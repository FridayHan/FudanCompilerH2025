#define DEBUG
#undef DEBUG

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
    // Normalize the name by replacing special characters with underscores
    if (name == "_^main^_^main") { //speial case for main
        return "main";
    }
    for (char& c : name) {
        if (!isalnum(c)) {
            c = '$';
        }
    }
    return name;
}

bool rpi_isMachineReg(int n) {
    // Check if a node is a machine register
    return (n >= 0 && n <= 15);
}

string term2str(QuadTerm *term, Color *color) {
    string result;
    if (term->kind == QuadTermKind::TEMP) {
        Temp *t = term->get_temp()->temp;
        result = "r" + to_string(color->color_of(t->num));
    } else if (term->kind == QuadTermKind::CONST) {
        result = "#" + to_string(term->get_const());
    } else if (term->kind == QuadTermKind::MAME) {
        result = "@" + term->get_name();
    } else {
        cerr << "Error: Unknown term kind" << endl;
        exit(EXIT_FAILURE);
    }
    return result;
}

//Always use function name to prefix a label
//Note that you should do the same for Jump and CJump labels
string convert(QuadLabel* label, Color *c, int indent) {
#ifdef DEBUG
    cout << "In convert Label" << endl;
#endif
    string result; 
    result = current_funcname + "$" + label->label->str() + ": \n";
    return result;
}

/*************************************************** */
/*************************************************** */
/*************************************************** */
/*************************************************** */
/*************************************************** */
/*************************************************** */

string convert(QuadFuncDecl* func, DataFlowInfo *dfi, Color *color, int indent) {
    string result; 
    current_funcname = func->funcname; //set the global variable
    // Iterate through Quads in the function and convert
    // one statement at a time (sometimes two statemetns can be combined into one, try that!)
    // 1) Set up the frame stack correctly at beginning of the function, and return
    // 2) Replace temp with register using the color map
    // 3) Take care of the spilled temps
    // 4) and other details
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
        
        // 生成基本块标签
        if (!label_emitted[block_label]) {
            result += current_funcname + "$" + block_label + ": \n";
            label_emitted[block_label] = true;
        }
        
        for (auto stmt : *block->quadlist) {
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
    
    string indent_str(indent, ' ');
    int offset = color->get_spill_offset(temp_num);
    return indent_str + "ldr r" + to_string(reg_num) + ", [fp, #-" + to_string(offset) + "]\n";
}

string storeSpilledTemp(int temp_num, Color* color, int reg_num, int indent) {
    if (!color->is_spill(temp_num)) return "";
    
    string indent_str(indent, ' ');
    int offset = color->get_spill_offset(temp_num);
    return indent_str + "str r" + to_string(reg_num) + ", [fp, #-" + to_string(offset) + "]\n";
}

string getTermStr(QuadTerm *term, Color *color, int temp_reg) {
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
            return loadSpilledTemp(t->num, color, temp_reg, indent);
        }
    }
    return "";
}

string convertQuadStm(QuadStm* stmt, Color* color, int indent, map<string, bool>& label_emitted) {
    string result;
    string indent_str(indent, ' ');
    
    switch (stmt->kind) {
        case QuadKind::MOVE: {
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
            QuadLoad* load = static_cast<QuadLoad*>(stmt);
            Temp* dst_temp = load->dst->temp;
            
            string dst_reg = "r" + to_string(color->color_of(dst_temp->num));
            string src_str = getTermStr(load->src, color);
            
            result += indent_str + "ldr " + dst_reg + ", [" + src_str + "]\n";
            break;
        }
        
        case QuadKind::STORE: {
            QuadStore* store = static_cast<QuadStore*>(stmt);
            
            string src_str = getTermStr(store->src, color);
            string dst_str = getTermStr(store->dst, color);
            
            result += indent_str + "str " + src_str + ", [" + dst_str + "]\n";
            break;
        }
        
        case QuadKind::MOVE_BINOP: {
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
            QuadJump* jump = static_cast<QuadJump*>(stmt);
            result += indent_str + "b " + current_funcname + "$" + jump->label->str() + "\n";
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
            break;
    }
    
    return result;
}

string quad2rpi(QuadProgram* quadProgram, ColorMap *cm) {// Convert a QuadProgram to RPI format with k registers
    string result; result.reserve(10000);
    // Iterate through the function declarations in the Quad program
    result = ".section .note.GNU-stack\n\n@ Here is the RPI code\n\n";
    for (QuadFuncDecl* func : *quadProgram->quadFuncDeclList) {
        //get the data flow info for the function
        DataFlowInfo *dfi = new DataFlowInfo(func);
        dfi->computeLiveness(); //liveness useful in some cases. Has to be done before trace otherwise this func code won't work!
        trace(func); //trace it (merge all blocks into one)
        current_funcname = func->funcname; //set the global variable
        //get the color for the function
        Color *c = cm->color_map[func->funcname]; 
        int indent = 9;
        result += convert(func, dfi, c, indent) + "\n";
    }
    //put the global functions at the end
    result += ".global malloc\n";
    result +=".global getint\n";
    result += ".global putint\n";
    result += ".global putch\n";
    result += ".global putarray\n";
    result += ".global getch\n";
    result += ".global getarray\n";
    result += ".global starttime\n";
    result += ".global stoptime\n";
    return result;
}

// Print the RPI code to the output file
void quad2rpi(QuadProgram* quadProgram, ColorMap *cm, string filename) {
    ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << quad2rpi(quadProgram, cm);
        outfile.flush();
        outfile.close();
    } else {
        cerr << "Error: Unable to open file " << filename << endl;
    }
}