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
#ifdef DEBUG
    cout << "[normalizeName] 开始处理函数名: " << name << endl;
#endif
    // Normalize the name by replacing special characters with underscores
    if (name == "_^main^_^main") { //speial case for main
#ifdef DEBUG
        cout << "[normalizeName] 检测到main函数，直接返回main" << endl;
#endif
        return "main";
    }
    for (char& c : name) {
        if (!isalnum(c)) {
            c = '$';
        }
    }
#ifdef DEBUG
    cout << "[normalizeName] 规范化后的函数名: " << name << endl;
#endif
    return name;
}

bool rpi_isMachineReg(int n) {
#ifdef DEBUG
    cout << "[rpi_isMachineReg] 检查寄存器编号: " << n << endl;
#endif
    // Check if a node is a machine register
    bool result = (n >= 0 && n <= 15);
#ifdef DEBUG
    cout << "[rpi_isMachineReg] 是否为机器寄存器: " << (result ? "是" : "否") << endl;
#endif
    return result;
}

string term2str(QuadTerm *term, Color *color) {
#ifdef DEBUG
    cout << "[term2str] 开始处理项，类型: " << (int)term->kind << endl;
#endif
    string result;
    if (term->kind == QuadTermKind::TEMP) {
        Temp *t = term->get_temp()->temp;
        result = "r" + to_string(color->color_of(t->num));
#ifdef DEBUG
        cout << "[term2str] 临时变量，寄存器分配: " << result << endl;
#endif
    } else if (term->kind == QuadTermKind::CONST) {
        result = "#" + to_string(term->get_const());
#ifdef DEBUG
        cout << "[term2str] 常量值: " << result << endl;
#endif
    } else if (term->kind == QuadTermKind::MAME) {
        result = "@" + term->get_name();
#ifdef DEBUG
        cout << "[term2str] 内存引用: " << result << endl;
#endif
    } else {
        cerr << "[term2str] 错误：未知的项类型" << endl;
        exit(EXIT_FAILURE);
    }
    return result;
}

//Always use function name to prefix a label
//Note that you should do the same for Jump and CJump labels
string convert(QuadLabel* label, Color *c, int indent) {
#ifdef DEBUG
    cout << "[convert Label] 开始处理标签: " << label->label->str() << endl;
    cout << "[convert Label] 当前函数名: " << current_funcname << endl;
#endif
    string result; 
    result = current_funcname + "$" + label->label->str() + ": \n";
#ifdef DEBUG
    cout << "[convert Label] 生成的标签: " << result;
#endif
    return result;
}

/*************************************************** */
/*************************************************** */
/*************************************************** */
/*************************************************** */
/*************************************************** */
/*************************************************** */

string convert(QuadFuncDecl* func, DataFlowInfo *dfi, Color *color, int indent) {
#ifdef DEBUG
    cout << "\n[convert FuncDecl] 开始处理函数: " << func->funcname << endl;
    cout << "[convert FuncDecl] 函数包含 " << func->quadblocklist->size() << " 个基本块" << endl;
#endif
    string result; 
    current_funcname = func->funcname;
    string indent_str(indent, ' ');
    
    result += "@ Here's function: " + func->funcname + "\n\n";
    
    string func_label = normalizeName(func->funcname);
#ifdef DEBUG
    cout << "[convert FuncDecl] 规范化后的函数标签: " << func_label << endl;
#endif
    
    result += ".balign 4\n";
    result += ".global " + func_label + "\n";
    result += ".section .text\n\n";
    result += func_label + ":\n";
    current_funcname = func_label;
    
    result += indent_str + "push {r4-r10, fp, lr}\n";
    result += indent_str + "add fp, sp, #32\n";
    
    // 收集所有跳转目标
    set<string> all_jump_targets;
    for (auto block : *func->quadblocklist) {
        for (auto stmt : *block->quadlist) {
            if (stmt->kind == QuadKind::JUMP) {
                QuadJump* jump = static_cast<QuadJump*>(stmt);
                all_jump_targets.insert(jump->label->str());
#ifdef DEBUG
                cout << "[convert FuncDecl] 添加JUMP目标: " << jump->label->str() << endl;
#endif
            } else if (stmt->kind == QuadKind::CJUMP) {
                QuadCJump* cjump = static_cast<QuadCJump*>(stmt);
                all_jump_targets.insert(cjump->t->str());
                all_jump_targets.insert(cjump->f->str());
#ifdef DEBUG
                cout << "[convert FuncDecl] 添加CJUMP目标: " << cjump->t->str() << " 和 " << cjump->f->str() << endl;
#endif
            }
        }
    }
    
    map<string, bool> label_emitted;
    
    // 处理每个基本块
    for (auto block : *func->quadblocklist) {
        string block_label = block->entry_label->str();
        
        // 如果这个基本块是跳转目标，先生成标签
        if (all_jump_targets.count(block_label) && !label_emitted[block_label]) {
            result += current_funcname + "$" + block_label + ": \n";
            label_emitted[block_label] = true;
#ifdef DEBUG
            cout << "[convert FuncDecl] 插入基本块标签: " << block_label << endl;
#endif
        }
        
        // 处理基本块中的语句
        for (auto stmt : *block->quadlist) {
            // 如果是条件跳转，先生成条件跳转指令
            if (stmt->kind == QuadKind::CJUMP) {
                QuadCJump* cjump = static_cast<QuadCJump*>(stmt);
                string true_label = cjump->t->str();
                string false_label = cjump->f->str();
                
                // 生成条件跳转指令
                result += convertQuadStm(stmt, color, indent, label_emitted);
                
                // 生成 true 分支标签
                if (!label_emitted[true_label]) {
                    result += current_funcname + "$" + true_label + ": \n";
                    label_emitted[true_label] = true;
#ifdef DEBUG
                    cout << "[convert FuncDecl] 插入true分支标签: " << true_label << endl;
#endif
                }
                
                // 生成 false 分支标签
                if (!label_emitted[false_label]) {
                    result += current_funcname + "$" + false_label + ": \n";
                    label_emitted[false_label] = true;
#ifdef DEBUG
                    cout << "[convert FuncDecl] 插入false分支标签: " << false_label << endl;
#endif
                }
            } else {
                // 处理其他类型的语句
                result += convertQuadStm(stmt, color, indent, label_emitted);
            }
        }
    }
    
    // 确保所有跳转目标标签都被生成
    for (const string& target : all_jump_targets) {
        if (!label_emitted[target]) {
            result += current_funcname + "$" + target + ": \n";
            label_emitted[target] = true;
#ifdef DEBUG
            cout << "[convert FuncDecl] 插入遗漏的跳转目标标签: " << target << endl;
#endif
        }
    }
    
#ifdef DEBUG
    cout << "[convert FuncDecl] 函数 " << func->funcname << " 处理完成\n" << endl;
#endif
    return result;
}

string loadSpilledTemp(int temp_num, Color* color, int reg_num, int indent) {
#ifdef DEBUG
    cout << "[loadSpilledTemp] 检查临时变量 " << temp_num << " 是否需要加载到寄存器 " << reg_num << endl;
#endif
    if (!color->is_spill(temp_num)) {
#ifdef DEBUG
        cout << "[loadSpilledTemp] 临时变量 " << temp_num << " 不需要加载" << endl;
#endif
        return "";
    }
    
    string indent_str(indent, ' ');
    int offset = color->get_spill_offset(temp_num);
    string result = indent_str + "ldr r" + to_string(reg_num) + ", [fp, #-" + to_string(offset) + "]\n";
#ifdef DEBUG
    cout << "[loadSpilledTemp] 生成加载指令: " << result;
#endif
    return result;
}

string storeSpilledTemp(int temp_num, Color* color, int reg_num, int indent) {
#ifdef DEBUG
    cout << "[storeSpilledTemp] 检查临时变量 " << temp_num << " 是否需要存储到内存" << endl;
#endif
    if (!color->is_spill(temp_num)) {
#ifdef DEBUG
        cout << "[storeSpilledTemp] 临时变量 " << temp_num << " 不需要存储" << endl;
#endif
        return "";
    }
    
    string indent_str(indent, ' ');
    int offset = color->get_spill_offset(temp_num);
    string result = indent_str + "str r" + to_string(reg_num) + ", [fp, #-" + to_string(offset) + "]\n";
#ifdef DEBUG
    cout << "[storeSpilledTemp] 生成存储指令: " << result;
#endif
    return result;
}

string getTermStr(QuadTerm *term, Color *color, int temp_reg) {
#ifdef DEBUG
    cout << "[getTermStr] 开始处理项，类型: " << (int)term->kind << endl;
#endif
    if (term->kind == QuadTermKind::TEMP) {
        Temp *t = term->get_temp()->temp;
        if (color->is_spill(t->num)) {
            string result = (temp_reg != -1) ? "r" + to_string(temp_reg) : "r9";
#ifdef DEBUG
            cout << "[getTermStr] 临时变量溢出，使用寄存器: " << result << endl;
#endif
            return result;
        } else {
            string result = "r" + to_string(color->color_of(t->num));
#ifdef DEBUG
            cout << "[getTermStr] 临时变量分配寄存器: " << result << endl;
#endif
            return result;
        }
    } else if (term->kind == QuadTermKind::CONST) {
        string result = "#" + to_string(term->get_const());
#ifdef DEBUG
        cout << "[getTermStr] 常量值: " << result << endl;
#endif
        return result;
    } else if (term->kind == QuadTermKind::MAME) {
        string result = "=" + normalizeName(term->get_name());
#ifdef DEBUG
        cout << "[getTermStr] 内存引用: " << result << endl;
#endif
        return result;
    }
    return "";
}

string loadSpillIfNeeded(QuadTerm *term, Color *color, int temp_reg, int indent) {
#ifdef DEBUG
    cout << "[loadSpillIfNeeded] 检查是否需要加载溢出变量" << endl;
#endif
    if (term->kind == QuadTermKind::TEMP) {
        Temp *t = term->get_temp()->temp;
        if (color->is_spill(t->num)) {
#ifdef DEBUG
            cout << "[loadSpillIfNeeded] 需要加载溢出变量 " << t->num << " 到寄存器 " << temp_reg << endl;
#endif
            return loadSpilledTemp(t->num, color, temp_reg, indent);
        }
    }
#ifdef DEBUG
    cout << "[loadSpillIfNeeded] 不需要加载溢出变量" << endl;
#endif
    return "";
}

string convertQuadStm(QuadStm* stmt, Color* color, int indent, map<string, bool>& label_emitted) {
#ifdef DEBUG
    cout << "\n[convertQuadStm] 开始处理语句，类型: " << (int)stmt->kind << endl;
#endif
    string result;
    string indent_str(indent, ' ');
    
    switch (stmt->kind) {
        case QuadKind::MOVE: {
            QuadMove* move = static_cast<QuadMove*>(stmt);
            Temp* dst_temp = move->dst->temp;
            
            string dst_reg = "r" + to_string(color->color_of(dst_temp->num));
            string src_str = getTermStr(move->src, color);
            
#ifdef DEBUG
            cout << "[convertQuadStm] MOVE指令: " << dst_reg << " <- " << src_str << endl;
#endif
            
            if (move->src->kind == QuadTermKind::MAME) {
                result += indent_str + "ldr r11, " + src_str + "\n";
                result += indent_str + "mov " + dst_reg + ", r11\n";
#ifdef DEBUG
                cout << "[convertQuadStm] 生成内存加载指令" << endl;
#endif
            } else if (dst_reg != src_str) {
                result += indent_str + "mov " + dst_reg + ", " + src_str + "\n";
#ifdef DEBUG
                cout << "[convertQuadStm] 生成寄存器移动指令" << endl;
#endif
            }
            break;
        }
        
        case QuadKind::LOAD: {
            QuadLoad* load = static_cast<QuadLoad*>(stmt);
            Temp* dst_temp = load->dst->temp;
            
            string dst_reg = "r" + to_string(color->color_of(dst_temp->num));
            string src_str = getTermStr(load->src, color);
            
#ifdef DEBUG
            cout << "[convertQuadStm] LOAD指令: " << dst_reg << " <- [" << src_str << "]" << endl;
#endif
            
            if (load->src->kind == QuadTermKind::MAME) {
                result += indent_str + "ldr r11, " + src_str + "\n";
                result += indent_str + "ldr " + dst_reg + ", [r11]\n";
#ifdef DEBUG
                cout << "[convertQuadStm] 生成间接内存加载指令" << endl;
#endif
            } else {
                result += indent_str + "ldr " + dst_reg + ", [" + src_str + "]\n";
#ifdef DEBUG
                cout << "[convertQuadStm] 生成直接内存加载指令" << endl;
#endif
            }
            break;
        }
        
        case QuadKind::STORE: {
            QuadStore* store = static_cast<QuadStore*>(stmt);
            
            string src_str = getTermStr(store->src, color);
            string dst_str = getTermStr(store->dst, color);
            
#ifdef DEBUG
            cout << "[convertQuadStm] STORE指令: [" << dst_str << "] <- " << src_str << endl;
#endif
            
            if (store->dst->kind == QuadTermKind::MAME) {
                result += indent_str + "ldr r11, " + dst_str + "\n";
                result += indent_str + "str " + src_str + ", [r11]\n";
#ifdef DEBUG
                cout << "[convertQuadStm] 生成间接内存存储指令" << endl;
#endif
            } else {
                result += indent_str + "str " + src_str + ", [" + dst_str + "]\n";
#ifdef DEBUG
                cout << "[convertQuadStm] 生成直接内存存储指令" << endl;
#endif
            }
            break;
        }
        
        case QuadKind::MOVE_BINOP: {
            QuadMoveBinop* binop = static_cast<QuadMoveBinop*>(stmt);
            Temp* dst_temp = binop->dst->temp;
            
            string dst_reg = "r" + to_string(color->color_of(dst_temp->num));
            string left_str = getTermStr(binop->left, color);
            string right_str = getTermStr(binop->right, color);
            
#ifdef DEBUG
            cout << "[convertQuadStm] MOVE_BINOP指令: " << dst_reg << " <- " << left_str << " " << binop->binop << " " << right_str << endl;
#endif
            
            string arm_op;
            if (binop->binop == "+") arm_op = "add";
            else if (binop->binop == "-") arm_op = "sub";
            else if (binop->binop == "*") arm_op = "mul";
            else if (binop->binop == "/") arm_op = "sdiv";
            else if (binop->binop == "%") {
                result += indent_str + "sdiv r11, " + left_str + ", " + right_str + "\n";
                result += indent_str + "mls " + dst_reg + ", r11, " + right_str + ", " + left_str + "\n";
#ifdef DEBUG
                cout << "[convertQuadStm] 生成取模运算指令" << endl;
#endif
                break;
            }
            else arm_op = "add";
            
            result += indent_str + arm_op + " " + dst_reg + ", " + left_str + ", " + right_str + "\n";
#ifdef DEBUG
            cout << "[convertQuadStm] 生成二元运算指令: " << arm_op << endl;
#endif
            break;
        }
        
        case QuadKind::JUMP: {
            QuadJump* jump = static_cast<QuadJump*>(stmt);
            result += indent_str + "b " + current_funcname + "$" + jump->label->str() + "\n";
#ifdef DEBUG
            cout << "[convertQuadStm] 生成无条件跳转指令，目标: " << jump->label->str() << endl;
#endif
            break;
        }
        
        case QuadKind::CJUMP: {
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
            
            // 生成比较指令
            result += indent_str + "cmp " + left_str + ", " + right_str + "\n";
            
            // 根据条件生成跳转指令
            string arm_cond;
            if (cjump->relop == "<") arm_cond = "lt";
            else if (cjump->relop == "<=") arm_cond = "le";
            else if (cjump->relop == "=") arm_cond = "eq";
            else if (cjump->relop == ">=") arm_cond = "ge";
            else if (cjump->relop == ">") arm_cond = "gt";
            else if (cjump->relop == "!=") arm_cond = "ne";
            else arm_cond = "eq";
            
            result += indent_str + "b" + arm_cond + " " + current_funcname + "$" + cjump->t->str() + "\n";
#ifdef DEBUG
            cout << "[convertQuadStm] 生成条件跳转指令，条件: " << arm_cond << ", true分支: " << cjump->t->str() << endl;
#endif
            break;
        }
        
        case QuadKind::RETURN: {
            QuadReturn* ret = static_cast<QuadReturn*>(stmt);
            
#ifdef DEBUG
            cout << "[convertQuadStm] 处理RETURN语句" << endl;
#endif
            
            if (ret->value) {
                string src_str = getTermStr(ret->value, color);
                if (src_str != "r0") {
                    result += indent_str + "mov r0, " + src_str + "\n";
#ifdef DEBUG
                    cout << "[convertQuadStm] 生成返回值移动指令" << endl;
#endif
                }
            }
            
            result += indent_str + "sub sp, fp, #32\n";
            result += indent_str + "pop {r4-r10, fp, pc}\n";
#ifdef DEBUG
            cout << "[convertQuadStm] 生成函数返回指令" << endl;
#endif
            break;
        }
        
        case QuadKind::EXTCALL: {
            QuadExtCall* extcall = static_cast<QuadExtCall*>(stmt);
            
#ifdef DEBUG
            cout << "[convertQuadStm] 处理外部调用: " << extcall->extfun << endl;
            cout << "[convertQuadStm] 参数数量: " << extcall->args->size() << endl;
#endif
            
            for (int i = 0; i < extcall->args->size() && i < 4; i++) {
                QuadTerm* arg = extcall->args->at(i);
                string dst_reg = "r" + to_string(i);
                string arg_str = getTermStr(arg, color);
                
                if (arg_str != dst_reg) {
                    result += indent_str + "mov " + dst_reg + ", " + arg_str + "\n";
#ifdef DEBUG
                    cout << "[convertQuadStm] 生成参数传递指令: " << dst_reg << " <- " << arg_str << endl;
#endif
                }
            }
            
            result += indent_str + "bl " + extcall->extfun + "\n";
#ifdef DEBUG
            cout << "[convertQuadStm] 生成外部调用指令" << endl;
#endif
            break;
        }
        
        case QuadKind::MOVE_EXTCALL: {
            QuadMoveExtCall* moveExtcall = static_cast<QuadMoveExtCall*>(stmt);
            QuadExtCall* extcall = moveExtcall->extcall;
            Temp* dst_temp = moveExtcall->dst->temp;
            
#ifdef DEBUG
            cout << "[convertQuadStm] 处理带返回值的内部调用: " << extcall->extfun << endl;
            cout << "[convertQuadStm] 参数数量: " << extcall->args->size() << endl;
#endif
            
            for (int i = 0; i < extcall->args->size() && i < 4; i++) {
                QuadTerm* arg = extcall->args->at(i);
                string dst_reg = "r" + to_string(i);
                string arg_str = getTermStr(arg, color);
                
                if (arg_str != dst_reg) {
                    result += indent_str + "mov " + dst_reg + ", " + arg_str + "\n";
#ifdef DEBUG
                    cout << "[convertQuadStm] 生成参数传递指令: " << dst_reg << " <- " << arg_str << endl;
#endif
                }
            }
            
            result += indent_str + "bl " + extcall->extfun + "\n";
            
            string dst_reg = "r" + to_string(color->color_of(dst_temp->num));
            if (dst_reg != "r0") {
                result += indent_str + "mov " + dst_reg + ", r0\n";
#ifdef DEBUG
                cout << "[convertQuadStm] 生成返回值移动指令: " << dst_reg << " <- r0" << endl;
#endif
            }
            break;
        }
        
        case QuadKind::CALL: {
            QuadCall* call = static_cast<QuadCall*>(stmt);
            
#ifdef DEBUG
            cout << "[convertQuadStm] 处理内部调用: " << call->name << endl;
            cout << "[convertQuadStm] 参数数量: " << call->args->size() << endl;
#endif
            
            for (int i = 0; i < call->args->size() && i < 4; i++) {
                QuadTerm* arg = call->args->at(i);
                string dst_reg = "r" + to_string(i);
                string arg_str = getTermStr(arg, color);
                
                if (arg_str != dst_reg) {
                    result += indent_str + "mov " + dst_reg + ", " + arg_str + "\n";
#ifdef DEBUG
                    cout << "[convertQuadStm] 生成参数传递指令: " << dst_reg << " <- " << arg_str << endl;
#endif
                }
            }
            
            if (call->obj_term) {
                string obj_str = getTermStr(call->obj_term, color);
                result += indent_str + "add r1, " + obj_str + ", #" + call->name + "\n";
                result += indent_str + "ldr r1, [r1]\n";
                result += indent_str + "blx r1\n";
#ifdef DEBUG
                cout << "[convertQuadStm] 生成对象方法调用指令" << endl;
#endif
            } else {
                result += indent_str + "bl " + normalizeName(call->name) + "\n";
#ifdef DEBUG
                cout << "[convertQuadStm] 生成普通函数调用指令" << endl;
#endif
            }
            break;
        }
        
        case QuadKind::MOVE_CALL: {
            QuadMoveCall* moveCall = static_cast<QuadMoveCall*>(stmt);
            QuadCall* call = moveCall->call;
            Temp* dst_temp = moveCall->dst->temp;
            
#ifdef DEBUG
            cout << "[convertQuadStm] 处理带返回值的内部调用: " << call->name << endl;
            cout << "[convertQuadStm] 参数数量: " << call->args->size() << endl;
#endif
            
            for (int i = 0; i < call->args->size() && i < 4; i++) {
                QuadTerm* arg = call->args->at(i);
                string dst_reg = "r" + to_string(i);
                string arg_str = getTermStr(arg, color);
                
                if (arg_str != dst_reg) {
                    result += indent_str + "mov " + dst_reg + ", " + arg_str + "\n";
#ifdef DEBUG
                    cout << "[convertQuadStm] 生成参数传递指令: " << dst_reg << " <- " << arg_str << endl;
#endif
                }
            }
            
            if (call->obj_term) {
                string obj_str = getTermStr(call->obj_term, color);
                result += indent_str + "add r1, " + obj_str + ", #" + call->name + "\n";
                result += indent_str + "ldr r1, [r1]\n";
                result += indent_str + "blx r1\n";
#ifdef DEBUG
                cout << "[convertQuadStm] 生成对象方法调用指令" << endl;
#endif
            } else {
                result += indent_str + "bl " + normalizeName(call->name) + "\n";
#ifdef DEBUG
                cout << "[convertQuadStm] 生成普通函数调用指令" << endl;
#endif
            }
            
            string dst_reg = "r" + to_string(color->color_of(dst_temp->num));
            if (dst_reg != "r0") {
                result += indent_str + "mov " + dst_reg + ", r0\n";
#ifdef DEBUG
                cout << "[convertQuadStm] 生成返回值移动指令: " << dst_reg << " <- r0" << endl;
#endif
            }
            break;
        }
        
        default:
#ifdef DEBUG
            cout << "[convertQuadStm] 未知的语句类型" << endl;
#endif
            break;
    }
    
    return result;
}

string quad2rpi(QuadProgram* quadProgram, ColorMap *cm) {
#ifdef DEBUG
    cout << "\n[quad2rpi] 开始转换四元式程序到RPI格式" << endl;
    cout << "[quad2rpi] 函数数量: " << quadProgram->quadFuncDeclList->size() << endl;
#endif
    
    string result; result.reserve(10000);
    result = ".section .note.GNU-stack\n\n@ Here is the RPI code\n\n";
    
    for (QuadFuncDecl* func : *quadProgram->quadFuncDeclList) {
#ifdef DEBUG
        cout << "\n[quad2rpi] 处理函数: " << func->funcname << endl;
#endif
        
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
    
#ifdef DEBUG
    cout << "[quad2rpi] RPI代码生成完成" << endl;
#endif
    
    return result;
}

void quad2rpi(QuadProgram* quadProgram, ColorMap *cm, string filename) {
#ifdef DEBUG
    cout << "\n[quad2rpi] 开始将RPI代码写入文件: " << filename << endl;
#endif
    
    ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << quad2rpi(quadProgram, cm);
        outfile.flush();
        outfile.close();
#ifdef DEBUG
        cout << "[quad2rpi] 文件写入成功" << endl;
#endif
    } else {
        cerr << "[quad2rpi] 错误：无法打开文件 " << filename << endl;
    }
}