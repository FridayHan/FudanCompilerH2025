#define DEBUG
// #undef DEBUG

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "quad.hh"
#include "flowinfo.hh"
#include "color.hh"
#include "quad2rpi.hh"

static string current_funcname = "";

//This is to convert the names of the functions to a format that is acceptable by the assembler
string normalizeName(string name) {
    DEBUG_PRINT2("[normalizeName] 开始处理函数名:", name);
    // Normalize the name by replacing special characters with underscores
    if (name == "_^main^_^main") { //speial case for main
        DEBUG_PRINT("[normalizeName] 检测到main函数，直接返回main");
        return "main";
    }
    for (char& c : name) {
        if (!isalnum(c)) {
            c = '$';
        }
    }
    DEBUG_PRINT2("[normalizeName] 规范化后的函数名:", name);
    return name;
}

bool rpi_isMachineReg(int n) {
    DEBUG_PRINT2("[rpi_isMachineReg] 检查寄存器编号:", n);
    // Check if a node is a machine register
    bool result = (n >= 0 && n <= 15);
    string answer = result ? "是" : "否";
    DEBUG_PRINT2("[rpi_isMachineReg] 是否为机器寄存器:", answer);
    return result;
}

string term2str(QuadTerm *term, Color *color) {
    DEBUG_PRINT2("[term2str] 开始处理项，类型:", (int)term->kind);
    string result;
    if (term->kind == QuadTermKind::TEMP) {
        Temp *t = term->get_temp()->temp;
        result = "r" + to_string(color->color_of(t->num));
        DEBUG_PRINT2("[term2str] 临时变量，寄存器分配:", result);
    } else if (term->kind == QuadTermKind::CONST) {
        result = "#" + to_string(term->get_const());
        DEBUG_PRINT2("[term2str] 常量值:", result);
    } else if (term->kind == QuadTermKind::MAME) {
        result = "@" + term->get_name();
        DEBUG_PRINT2("[term2str] 内存引用:", result);
    } else {
        std::cerr << "[term2str] 错误：未知的项类型" << std::endl;
        exit(EXIT_FAILURE);
    }
    return result;
}

//Always use function name to prefix a label
//Note that you should do the same for Jump and CJump labels
string convert(QuadLabel* label, Color *c, int indent) {
    DEBUG_PRINT2("[convert Label] 开始处理标签:", label->label->str());
    DEBUG_PRINT2("[convert Label] 当前函数名:", current_funcname);
    string result; 
    result = current_funcname + "$" + label->label->str() + ": \n";
    DEBUG_PRINT2("[convert Label] 生成的标签:", result);
    return result;
}

/*************************************************** */
/*************************************************** */
/*************************************************** */
/*************************************************** */
/*************************************************** */
/*************************************************** */

// Helper function declarations
string loadSpilledTemp(int temp_num, Color* color, int reg_num, int indent);
string storeSpilledTemp(int temp_num, Color* color, int reg_num, int indent);
string getTermStr(QuadTerm *term, Color *color, int temp_reg = -1);
string loadSpillIfNeeded(QuadTerm *term, Color *color, int temp_reg, int indent);
string convertQuadStm(QuadStm* stmt, Color* color, int indent, map<string, bool>& label_emitted);

string convert(QuadFuncDecl* func, DataFlowInfo *dfi, Color *color, int indent) {
    DEBUG_PRINT2("[convert Func] 处理函数:", func->funcname);
    string result; 
    current_funcname = func->funcname;
    string indent_str(indent, ' ');
    
    result += "@ Here's function: " + func->funcname + "\n\n";
    
    string func_label = normalizeName(func->funcname);
    result += ".balign 4\n";
    result += ".global " + func_label + "\n";
    result += ".section .text\n\n";
    result += func_label + ":\n";
    current_funcname = func_label;
    
    result += indent_str + "push {r4-r10, fp, lr}\n";
    result += indent_str + "add fp, sp, #32\n";
    
    // 收集所有跳转目标，确保生成所有需要的标签
    set<string> all_jump_targets;
    for (auto block : *func->quadblocklist) {
        for (auto stmt : *block->quadlist) {
            if (stmt->kind == QuadKind::JUMP) {
                QuadJump* jump = static_cast<QuadJump*>(stmt);
                all_jump_targets.insert(jump->label->str());
            } else if (stmt->kind == QuadKind::CJUMP) {
                QuadCJump* cjump = static_cast<QuadCJump*>(stmt);
                all_jump_targets.insert(cjump->t->str());
                all_jump_targets.insert(cjump->f->str());
            }
        }
    }
    
    // 跟踪已生成的标签
    map<string, bool> label_emitted;
    
    for (auto block : *func->quadblocklist) {
        string block_label = block->entry_label->str();
        DEBUG_PRINT2("[convert Func] 处理基本块:", block_label);
        // 生成基本块标签
        if (!label_emitted[block_label]) {
            result += current_funcname + "$" + block_label + ": \n";
            label_emitted[block_label] = true;
        }
        
        for (auto stmt : *block->quadlist) {
            DEBUG_PRINT2("[convert Func] 处理语句 kind:", (int)stmt->kind);
            // 在处理每个语句前，检查是否需要生成跳转目标标签
            if (stmt->kind == QuadKind::CJUMP) {
                QuadCJump* cjump = static_cast<QuadCJump*>(stmt);
                // 处理CJUMP指令
                result += convertQuadStm(stmt, color, indent, label_emitted);
                
                // CJUMP后立即生成false分支标签
                string false_label = cjump->f->str();
                if (!label_emitted[false_label]) {
                    result += current_funcname + "$" + false_label + ": \n";
                    label_emitted[false_label] = true;
                }
            } else {
                result += convertQuadStm(stmt, color, indent, label_emitted);
            }
        }
    }
    
    // 确保所有跳转目标标签都被生成
    for (const string& target : all_jump_targets) {
        if (!label_emitted[target]) {
            result += current_funcname + "$" + target + ": \n";
            label_emitted[target] = true;
        }
    }
    
    return result;
}

string loadSpilledTemp(int temp_num, Color* color, int reg_num, int indent) {
    if (!color->is_spill(temp_num)) return "";
    DEBUG_PRINT2("[loadSpilledTemp] 加载溢出变量:", temp_num);
    string indent_str(indent, ' ');
    int offset = color->get_spill_offset(temp_num);
    return indent_str + "ldr r" + to_string(reg_num) + ", [fp, #-" + to_string(offset) + "]\n";
}

string storeSpilledTemp(int temp_num, Color* color, int reg_num, int indent) {
    if (!color->is_spill(temp_num)) return "";
    DEBUG_PRINT2("[storeSpilledTemp] 存储溢出变量:", temp_num);
    string indent_str(indent, ' ');
    int offset = color->get_spill_offset(temp_num);
    return indent_str + "str r" + to_string(reg_num) + ", [fp, #-" + to_string(offset) + "]\n";
}

string getTermStr(QuadTerm *term, Color *color, int temp_reg) {
    DEBUG_PRINT2("[getTermStr] 处理项类型:", (int)term->kind);
    if (term->kind == QuadTermKind::TEMP) {
        Temp *t = term->get_temp()->temp;
        if (color->is_spill(t->num)) {
            return (temp_reg != -1) ? "r" + to_string(temp_reg) : "r9";
        } else {
            return "r" + to_string(color->color_of(t->num));
        }
    } else if (term->kind == QuadTermKind::CONST) {
        return "#" + to_string(term->get_const());
    } else if (term->kind == QuadTermKind::MAME) {
        return "=" + normalizeName(term->get_name());
    }
    return "";
}

string loadSpillIfNeeded(QuadTerm *term, Color *color, int temp_reg, int indent) {
    if (term->kind == QuadTermKind::TEMP) {
        Temp *t = term->get_temp()->temp;
        if (color->is_spill(t->num)) {
            DEBUG_PRINT2("[loadSpillIfNeeded] 需要加载溢出变量:", t->num);
            return loadSpilledTemp(t->num, color, temp_reg, indent);
        }
    }
    return "";
}

string convertQuadStm(QuadStm* stmt, Color* color, int indent, map<string, bool>& label_emitted) {
    string result;
    string indent_str(indent, ' ');
    DEBUG_PRINT2("[convertQuadStm] 处理语句 kind:", (int)stmt->kind);
    switch (stmt->kind) {
        case QuadKind::MOVE: {
            DEBUG_PRINT("[convertQuadStm] 处理MOVE语句");
            QuadMove* move = static_cast<QuadMove*>(stmt);
            Temp* dst_temp = move->dst->temp;
            
            string dst_reg = "r" + to_string(color->color_of(dst_temp->num));
            string src_str = getTermStr(move->src, color);
            
            if (dst_reg != src_str) {
                result += indent_str + "mov " + dst_reg + ", " + src_str + "\n";
            }
            break;
        }
        
        case QuadKind::LOAD: {
            DEBUG_PRINT("[convertQuadStm] 处理LOAD语句");
            QuadLoad* load = static_cast<QuadLoad*>(stmt);
            Temp* dst_temp = load->dst->temp;
            
            string dst_reg = "r" + to_string(color->color_of(dst_temp->num));
            string src_str = getTermStr(load->src, color);
            
            result += indent_str + "ldr " + dst_reg + ", [" + src_str + "]\n";
            break;
        }
        
        case QuadKind::STORE: {
            DEBUG_PRINT("[convertQuadStm] 处理STORE语句");
            QuadStore* store = static_cast<QuadStore*>(stmt);
            
            string src_str = getTermStr(store->src, color);
            string dst_str = getTermStr(store->dst, color);
            
            result += indent_str + "str " + src_str + ", [" + dst_str + "]\n";
            break;
        }
        
        case QuadKind::MOVE_BINOP: {
            DEBUG_PRINT("[convertQuadStm] 处理MOVE_BINOP语句");
            QuadMoveBinop* binop = static_cast<QuadMoveBinop*>(stmt);
            Temp* dst_temp = binop->dst->temp;
            
            string dst_reg = "r" + to_string(color->color_of(dst_temp->num));
            string left_str = getTermStr(binop->left, color);
            string right_str = getTermStr(binop->right, color);
            
            string arm_op;
            if (binop->binop == "+") arm_op = "add";
            else if (binop->binop == "-") arm_op = "sub";
            else if (binop->binop == "*") arm_op = "mul";
            else if (binop->binop == "/") arm_op = "sdiv";
            else if (binop->binop == "%") {
                result += indent_str + "sdiv r11, " + left_str + ", " + right_str + "\n";
                result += indent_str + "mls " + dst_reg + ", r11, " + right_str + ", " + left_str + "\n";
                break;
            }
            else arm_op = "add";
            
            result += indent_str + arm_op + " " + dst_reg + ", " + left_str + ", " + right_str + "\n";
            break;
        }
        
        case QuadKind::JUMP: {
            DEBUG_PRINT("[convertQuadStm] 处理JUMP语句");
            QuadJump* jump = static_cast<QuadJump*>(stmt);
            result += indent_str + "b " + current_funcname + "$" + jump->label->str() + "\n";
            break;
        }
        
        case QuadKind::CJUMP: {
            DEBUG_PRINT("[convertQuadStm] 处理CJUMP语句");
            QuadCJump* cjump = static_cast<QuadCJump*>(stmt);
            
            string left_str, right_str;
            
            if (cjump->left->kind == QuadTermKind::TEMP && color->is_spill(cjump->left->get_temp()->temp->num)) {
                result += loadSpillIfNeeded(cjump->left, color, 9, indent);
                left_str = "r9";
            } else {
                left_str = getTermStr(cjump->left, color, -1);
            }
            
            if (cjump->right->kind == QuadTermKind::TEMP && color->is_spill(cjump->right->get_temp()->temp->num)) {
                result += loadSpillIfNeeded(cjump->right, color, 10, indent);
                right_str = "r10";
            } else {
                right_str = getTermStr(cjump->right, color, -1);
            }
            
            result += indent_str + "cmp " + left_str + ", " + right_str + "\n";
            
            string condition;
            if (cjump->relop == "==") condition = "eq";
            else if (cjump->relop == "!=") condition = "ne";
            else if (cjump->relop == "<") condition = "lt";
            else if (cjump->relop == "<=") condition = "le";
            else if (cjump->relop == ">") condition = "gt";
            else if (cjump->relop == ">=") condition = "ge";
            else condition = "eq";
            
            result += indent_str + "b" + condition + " " + current_funcname + "$" + cjump->t->str() + "\n";
            
            // 为false分支生成标签
            string false_label = cjump->f->str();
            if (!label_emitted[false_label]) {
                result += current_funcname + "$" + false_label + ": \n";
                label_emitted[false_label] = true;
            }
            break;
        }
        
        case QuadKind::RETURN: {
            DEBUG_PRINT("[convertQuadStm] 处理RETURN语句");
            QuadReturn* ret = static_cast<QuadReturn*>(stmt);
            
            if (ret->value) {
                string src_str = getTermStr(ret->value, color);
                if (src_str != "r0") {
                    result += indent_str + "mov r0, " + src_str + "\n";
                }
            }
            
            // 简化epilogue - 移除栈操作
            result += indent_str + "sub sp, fp, #32\n";
            result += indent_str + "pop {r4-r10, fp, pc}\n";
            break;
        }
        
        case QuadKind::EXTCALL: {
            DEBUG_PRINT("[convertQuadStm] 处理EXTCALL语句");
            QuadExtCall* extcall = static_cast<QuadExtCall*>(stmt);
            
            for (int i = 0; i < extcall->args->size() && i < 4; i++) {
                QuadTerm* arg = extcall->args->at(i);
                string dst_reg = "r" + to_string(i);
                string arg_str = getTermStr(arg, color);
                
                if (arg_str != dst_reg) {
                    result += indent_str + "mov " + dst_reg + ", " + arg_str + "\n";
                }
            }
            
            result += indent_str + "bl " + extcall->extfun + "\n";
            break;
        }
        
        case QuadKind::MOVE_EXTCALL: {
            DEBUG_PRINT("[convertQuadStm] 处理MOVE_EXTCALL语句");
            QuadMoveExtCall* moveExtcall = static_cast<QuadMoveExtCall*>(stmt);
            QuadExtCall* extcall = moveExtcall->extcall;
            Temp* dst_temp = moveExtcall->dst->temp;
            
            for (int i = 0; i < extcall->args->size() && i < 4; i++) {
                QuadTerm* arg = extcall->args->at(i);
                string dst_reg = "r" + to_string(i);
                string arg_str = getTermStr(arg, color);
                
                if (arg_str != dst_reg) {
                    result += indent_str + "mov " + dst_reg + ", " + arg_str + "\n";
                }
            }
            
            result += indent_str + "bl " + extcall->extfun + "\n";
            
            string dst_reg = "r" + to_string(color->color_of(dst_temp->num));
            if (dst_reg != "r0") {
                result += indent_str + "mov " + dst_reg + ", r0\n";
            }
            break;
        }
        
        case QuadKind::CALL: {
            DEBUG_PRINT("[convertQuadStm] 处理CALL语句");
            QuadCall* call = static_cast<QuadCall*>(stmt);
            
            for (int i = 0; i < call->args->size() && i < 4; i++) {
                QuadTerm* arg = call->args->at(i);
                string dst_reg = "r" + to_string(i);
                string arg_str = getTermStr(arg, color);
                
                if (arg_str != dst_reg) {
                    result += indent_str + "mov " + dst_reg + ", " + arg_str + "\n";
                }
            }
            
            if (call->obj_term) {
                string obj_str = getTermStr(call->obj_term, color);
                result += indent_str + "add r1, " + obj_str + ", #" + call->name + "\n";
                result += indent_str + "ldr r1, [r1]\n";
                result += indent_str + "blx r1\n";
            } else {
                result += indent_str + "bl " + normalizeName(call->name) + "\n";
            }
            break;
        }
        
        case QuadKind::MOVE_CALL: {
            DEBUG_PRINT("[convertQuadStm] 处理MOVE_CALL语句");
            QuadMoveCall* moveCall = static_cast<QuadMoveCall*>(stmt);
            QuadCall* call = moveCall->call;
            Temp* dst_temp = moveCall->dst->temp;
            
            for (int i = 0; i < call->args->size() && i < 4; i++) {
                QuadTerm* arg = call->args->at(i);
                string dst_reg = "r" + to_string(i);
                string arg_str = getTermStr(arg, color);
                
                if (arg_str != dst_reg) {
                    result += indent_str + "mov " + dst_reg + ", " + arg_str + "\n";
                }
            }
            
            if (call->obj_term) {
                string obj_str = getTermStr(call->obj_term, color);
                result += indent_str + "add r1, " + obj_str + ", #" + call->name + "\n";
                result += indent_str + "ldr r1, [r1]\n";
                result += indent_str + "blx r1\n";
            } else {
                result += indent_str + "bl " + normalizeName(call->name) + "\n";
            }
            
            string dst_reg = "r" + to_string(color->color_of(dst_temp->num));
            if (dst_reg != "r0") {
                result += indent_str + "mov " + dst_reg + ", r0\n";
            }
            break;
        }
        
        default:
            DEBUG_PRINT("[convertQuadStm] 未知语句类型");
            break;
    }
    return result;
}

string quad2rpi(QuadProgram* quadProgram, ColorMap *cm) {
    DEBUG_PRINT("[quad2rpi] 开始转换四元式程序到RPI格式");
    DEBUG_PRINT2("[quad2rpi] 函数数量:", quadProgram->quadFuncDeclList->size());
    string result; result.reserve(10000);
    result = ".section .note.GNU-stack\n\n@ Here is the RPI code\n\n";
    
    for (QuadFuncDecl* func : *quadProgram->quadFuncDeclList) {
        DEBUG_PRINT2("[quad2rpi] 处理函数:", func->funcname);
        DataFlowInfo *dfi = new DataFlowInfo(func);
        dfi->computeLiveness();
        trace(func);
        current_funcname = func->funcname;
        Color *c = cm->color_map[func->funcname];
        int indent = 9;
        result += convert(func, dfi, c, indent) + "\n";
    }
    
    result += ".global malloc\n";
    result +=".global getint\n";
    result += ".global putint\n";
    result += ".global putch\n";
    result += ".global putarray\n";
    result += ".global getch\n";
    result += ".global getarray\n";
    result += ".global starttime\n";
    result += ".global stoptime\n";
    
    DEBUG_PRINT("[quad2rpi] RPI代码生成完成");
    return result;
}

void quad2rpi(QuadProgram* quadProgram, ColorMap *cm, string filename) {
    DEBUG_PRINT2("[quad2rpi] 开始将RPI代码写入文件:", filename);
    ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << quad2rpi(quadProgram, cm);
        outfile.flush();
        outfile.close();
        DEBUG_PRINT("[quad2rpi] 文件写入成功");
    } else {
        std::cerr << "[quad2rpi] 错误：无法打开文件 " << filename << std::endl;
    }
}