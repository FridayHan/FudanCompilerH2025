#define DEBUG
#undef DEBUG

#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include "namemaps.hh"
#include "semant.hh"

using namespace std;
using namespace fdmj;

AST_Semant_Map* semant_analyze(Program* node) {
    cerr << "Start Semantic Analysis" << endl;
    if (node == nullptr) {
        return nullptr;
    }
    Name_Maps* name_maps = makeNameMaps(node);
    AST_Semant_Visitor semant_visitor(name_maps);
    semant_visitor.visit(node);
    cerr << "Semantic Analysis Done" << endl;
    return semant_visitor.getSemantMap();
}

void AST_Semant_Visitor::visit(Program* node) {
    DEBUG_PRINT("Visiting Program");
    CHECK_NULLPTR(node);
    if (node->main) {
        node->main->accept(*this);
    }

    if (node->cdl) {
        for (auto cl : *(node->cdl)) {
            CHECK_NULLPTR(cl);
            cl->accept(*this);
        }
    }
}

void AST_Semant_Visitor::visit(MainMethod* node) {
    DEBUG_PRINT("Visiting MainMethod");
    CHECK_NULLPTR(node);
    current_class = "MainClass";
    current_method = "^_main";
    
    Type* main_type = new Type(node->getPos());
    name_maps->add_method_type(current_class, current_method, main_type);
    
    if (node->vdl) {
        set<string> declared_vars;
        
        // 检查变量是否重复定义
        for (auto vd : *(node->vdl)) {
            if (vd && vd->id) {
                string var_name = vd->id->id;
                if (declared_vars.find(var_name) != declared_vars.end()) {
                    cerr << "Error at line " << vd->getPos()->sline << ", column " << vd->getPos()->scolumn
                         << ": Variable '" << var_name << "' in main method is already declared" << endl;
                } else {
                    declared_vars.insert(var_name);
                }
            }
            vd->accept(*this);
        }
    }
    
    if (node->sl) {
        for (auto s : *(node->sl)) {
            s->accept(*this);
        }
    }
    
    current_class = "";
    current_method = "";
}

void AST_Semant_Visitor::visit(ClassDecl* node) {
    DEBUG_PRINT("Visiting ClassDecl: " << node->id->id);
    CHECK_NULLPTR(node);
    CHECK_NULLPTR(node->id);
    
    string class_name = node->id->id;
    string saved_class = current_class;
    current_class = class_name;
    
    if (node->vdl) {
        set<string> class_vars;
        for (auto vd : *(node->vdl)) {
            if (vd && vd->id) {
                string var_name = vd->id->id;
                if (class_vars.find(var_name) != class_vars.end()) {
                    cerr << "Error at line " << vd->getPos()->sline << ", column " << vd->getPos()->scolumn
                         << ": Duplicate variable '" << var_name << "' in class '" << class_name << "'" << endl;
                } else {
                    class_vars.insert(var_name);
                }

                VarDecl* parent_var = nullptr;
                string parent_class = "";

                set<string> ancestors = name_maps->get_ancestors(class_name);
                for (const auto& anc : ancestors) {
                    if (name_maps->is_class_var(anc, var_name)) {
                        parent_var = name_maps->get_class_var(anc, var_name);
                        parent_class = anc;
                        break;
                    }
                }

                if (parent_var) {
                    cerr << "Warning at line " << vd->getPos()->sline << ", column " << vd->getPos()->scolumn
                         << ": Variable '" << var_name << "' in class '" << class_name
                         << "' hides inherited variable from class '" << parent_class << "'" << endl;
                }
            }

            if (vd) {
                vd->accept(*this);
            }
        }
    }
    
    if (node->mdl) {
        set<string> class_methods;
        for (auto md : *(node->mdl)) {
            if (md && md->id) {
                string method_name = md->id->id;
                if (class_methods.find(method_name) != class_methods.end()) {
                    cerr << "Error at line " << md->getPos()->sline << ", column " << md->getPos()->scolumn
                         << ": Duplicate method '" << method_name << "' in class '" << class_name << "'" << endl;
                } else {
                    class_methods.insert(method_name);
                }
            }

            if (md) {
                md->accept(*this);
            }
        }
    }
    
    if (node->eid) {
        string parent_name = node->eid->id;
        if (!name_maps->is_class(parent_name)) {
            cerr << "Error at line " << node->eid->getPos()->sline << ", column " << node->eid->getPos()->scolumn
                 << ": Parent class '" << parent_name << "' not found" << endl;
        }
    }
    
    // do not visit the class name or the parent name here since they are
    // declarations rather than usages and should not carry semantic info
    
    current_class = saved_class;
}

void AST_Semant_Visitor::visit(MethodDecl* node) {
    DEBUG_PRINT("Visiting MethodDecl: " << node->id->id);
    CHECK_NULLPTR(node);
    
    current_method = node->id->id;
    name_maps->add_method(current_class, current_method);
    
    if (node->fl) {
        set<string> param_names;
        for (auto f : *(node->fl)) {
            if (f && f->id) {
                string param_name = f->id->id;
                if (param_names.find(param_name) != param_names.end()) {
                    cerr << "Error at line " << f->getPos()->sline << ", column " << f->getPos()->scolumn
                         << ": Duplicate parameter name '" << param_name << "' in method '" 
                         << node->id->id << "'" << endl;
                } else {
                    param_names.insert(param_name);
                }
            }
            f->accept(*this);
        }
    }
    
    if (node->type) {
        node->type->accept(*this);
    }
    
    if (node->vdl) {
        set<string> local_vars;
        for (auto vd : *(node->vdl)) {
            if (vd && vd->id) {
                string var_name = vd->id->id;
                if (name_maps->is_method_formal(current_class, current_method, var_name)) {
                    cerr << "Error at line " << vd->getPos()->sline << ", column " << vd->getPos()->scolumn
                         << ": Variable '" << var_name << "' in method '" << node->id->id 
                         << "' conflicts with a parameter" << endl;
                }
                if (local_vars.find(var_name) != local_vars.end()) {
                    cerr << "Error at line " << vd->getPos()->sline << ", column " << vd->getPos()->scolumn
                         << ": Variable '" << var_name << "' in method '" << node->id->id 
                         << "' is already declared" << endl;
                } else {
                    local_vars.insert(var_name);
                }
            }
            vd->accept(*this);
        }
    }
    
    if (node->sl) {
        for (auto stm : *(node->sl)) {
            stm->accept(*this);
        }
    }
    
    current_method = "";
    
    if (node->type && node->sl && !node->sl->empty()) {
        Stm* last_stm = node->sl->back();
        if (last_stm->getASTKind() == ASTKind::Return) {
            auto ret_stm = static_cast<Return*>(last_stm);
            if (ret_stm->exp) {
                ret_stm->exp->accept(*this);
                auto ret_semant = semant_map->getSemant(ret_stm->exp);
                if (ret_semant && ret_semant->get_type() != node->type->typeKind) {
                    cerr << "Error: Return type mismatch in method " << node->id->id << endl;
                }
            }
        }
    }
}

void AST_Semant_Visitor::visit(Type* node) {
    DEBUG_PRINT("Visiting Type");
    CHECK_NULLPTR(node);
}

void AST_Semant_Visitor::visit(VarDecl* node) {
    DEBUG_PRINT("Visiting VarDecl: " << node->id->id);
    CHECK_NULLPTR(node);
    CHECK_NULLPTR(node->id);

    string var_name = node->id->id;
    
    if (!current_class.empty() && current_method.empty()) {
        if (name_maps->is_class_var(current_class, var_name)) {
            VarDecl* existing_var = name_maps->get_class_var(current_class, var_name);
            if (existing_var && existing_var != node) {
                cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                     << ": Variable '" << var_name << "' in class '" << current_class 
                     << "' is already defined at line " << existing_var->getPos()->sline << endl;
            }
        }
    } 
    else if (!current_class.empty() && !current_method.empty()) {
        if (name_maps->is_method_var(current_class, current_method, var_name)) {
            VarDecl* existing_var = name_maps->get_method_var(current_class, current_method, var_name);
            if (existing_var && existing_var != node) {
                cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                     << ": Variable '" << var_name << "' in method '" << current_method 
                     << "' is already defined at line " << existing_var->getPos()->sline << endl;
            }
        }
        
        if (name_maps->is_method_formal(current_class, current_method, var_name)) {
            Formal* formal = name_maps->get_method_formal(current_class, current_method, var_name);
            if (formal) {
                cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                     << ": Variable '" << var_name << "' conflicts with method parameter"
                     << " defined at line " << formal->getPos()->sline << endl;
            }
        }
    }

    if (node->type) {
        node->type->accept(*this);
    }
}

void AST_Semant_Visitor::visit(Formal* node) {
    DEBUG_PRINT("Visiting Formal: " << node->id->id);
    CHECK_NULLPTR(node);
    CHECK_NULLPTR(node->id);

    string param_name = node->id->id;
    
    if (param_name == "^_method_return") {
        if (node->type) {
            node->type->accept(*this);
        }
        return;
    }
    
    if (!current_class.empty() && !current_method.empty()) {
        vector<string> param_list = name_maps->get_method_formal_list_names(current_class, current_method);
        
        int count = 0;
        for (auto p : param_list) {
            if (p == param_name) {
                count++;
            }
        }
        
        if (count > 1) {
            cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                 << ": Duplicate parameter name '" << param_name << "' in method '" 
                 << current_method << "'" << endl;
        }
    }

    if (node->type) {
        node->type->accept(*this);
    }
}

void AST_Semant_Visitor::visit(Assign* node) {
    DEBUG_PRINT("Visiting Assign");
    CHECK_NULLPTR(node);
    if (node->left) {
        node->left->accept(*this);
    }
    if (node->exp) {
        node->exp->accept(*this);
    }
    
    auto lhs_semant = semant_map->getSemant(node->left);
    if (lhs_semant && !lhs_semant->is_lvalue()) {
        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
             << ": Left side of assignment is not an lvalue" << endl;
        
        if (node->left->getASTKind() == ASTKind::IdExp) {
            IdExp* idExp = static_cast<IdExp*>(node->left);
            cerr << "  Identifier '" << idExp->id << "' cannot be assigned to" << endl;
        } else if (node->left->getASTKind() == ASTKind::CallExp) {
            cerr << "  Method call result cannot be assigned to" << endl;
        } else if (node->left->getASTKind() == ASTKind::BinaryOp) {
            cerr << "  Expression result cannot be assigned to" << endl;
        }
    }
    
    auto rhs_semant = semant_map->getSemant(node->exp);
    if (lhs_semant && rhs_semant) {
        if (lhs_semant->get_type() != rhs_semant->get_type()) {
            cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                 << ": Type mismatch in assignment - cannot assign " 
                 << type_kind_string(rhs_semant->get_type()) << " to " 
                 << type_kind_string(lhs_semant->get_type()) << endl;
        } else if (lhs_semant->get_type() == TypeKind::CLASS) {
            auto lhs_class = get<string>(lhs_semant->get_type_par());
            auto rhs_class = get<string>(rhs_semant->get_type_par());
            if (lhs_class != rhs_class) {
                auto ancestors = name_maps->get_ancestors(rhs_class);
                if (ancestors.find(lhs_class) == ancestors.end()) {
                    cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                         << ": Incompatible class types in assignment - cannot assign " 
                         << rhs_class << " to " << lhs_class << endl;
                }
            }
        }
    }
}

void AST_Semant_Visitor::visit(If* node) {
    DEBUG_PRINT("Visiting If");
    CHECK_NULLPTR(node);
    if (node->exp) {
        node->exp->accept(*this);
        auto cond_semant = semant_map->getSemant(node->exp);
        if (cond_semant && cond_semant->get_type() != TypeKind::INT) {
            cerr << "Error at line " << node->exp->getPos()->sline << ", column " << node->exp->getPos()->scolumn
                 << ": If condition must be of boolean type, found " 
                 << type_kind_string(cond_semant->get_type()) << endl;
        }
    }
    
    if (node->stm1) {
        node->stm1->accept(*this);
    }
    
    if (node->stm2) {
        node->stm2->accept(*this);
    }
}

void AST_Semant_Visitor::visit(While* node) {
    DEBUG_PRINT("Visiting While");
    CHECK_NULLPTR(node);
    if (node->exp) {
        node->exp->accept(*this);
        
        auto cond_semant = semant_map->getSemant(node->exp);
        if (cond_semant && cond_semant->get_type() != TypeKind::INT) {
            cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                 << ": While condition must be of boolean type, found " 
                 << type_kind_string(cond_semant->get_type()) << endl;
        }
    }
    
    loop_depth++;
    
    if (node->stm) {
        node->stm->accept(*this);
    }
    
    loop_depth--;
}

void AST_Semant_Visitor::visit(BinaryOp* node) {
    DEBUG_PRINT("Visiting BinaryOp");
    CHECK_NULLPTR(node);
    
    if (node->left) {
        node->left->accept(*this);
    }
    if (node->right) {
        node->right->accept(*this);
    }
    
    // do not force semantics for IdExp here; they will be resolved in their own visit
    
    AST_Semant* left_semant = semant_map->getSemant(node->left);
    AST_Semant* right_semant = semant_map->getSemant(node->right);
    
    TypeKind result_type = TypeKind::INT;
    AST_Semant::Kind s_kind = AST_Semant::Kind::Value;
    bool is_lvalue = false;
    variant<monostate, string, int> type_par{};
    
    if (node->op && !node->op->op.empty()) {
        string op_str = node->op->op;
        
        // 针对算术运算符的处理
        if (op_str == "+" || op_str == "-" || op_str == "*" || op_str == "/") {
            // 如果有任一操作数为数组，则必须都是数组
            bool left_is_array = (left_semant && left_semant->get_type() == TypeKind::ARRAY);
            bool right_is_array = (right_semant && right_semant->get_type() == TypeKind::ARRAY);
            if (left_is_array || right_is_array) {
                if (!(left_is_array && right_is_array)) {
                    cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                         << ": Mixed array and int in '" << op_str << "' operation" << endl;
                } else {
                    int larity = get<int>(left_semant->get_type_par());
                    int rarity = get<int>(right_semant->get_type_par());
                    if (larity != rarity) {
                        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                             << ": Array operands must have the same arity in '" << op_str << "' operation" << endl;
                    }
                    result_type = TypeKind::ARRAY;
                    type_par = variant<monostate, string, int>(larity);
                    s_kind = AST_Semant::Kind::Value;
                    is_lvalue = false;
                }
            }
            else {
                // 原 int 检查
                if (left_semant && left_semant->get_type() != TypeKind::INT) {
                    cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                         << ": Left operand of '" << op_str << "' must be int, found "
                         << type_kind_string(left_semant->get_type()) << endl;
                }
                if (right_semant && right_semant->get_type() != TypeKind::INT) {
                    cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                         << ": Right operand of '" << op_str << "' must be int, found "
                         << type_kind_string(right_semant->get_type()) << endl;
                }
                result_type = TypeKind::INT;
                s_kind = AST_Semant::Kind::Value;
                is_lvalue = false;
            }
        }
        // 针对比较运算符==和!=的处理
        else if (op_str == "==" || op_str == "!=") {
            bool left_is_array = (left_semant && left_semant->get_type() == TypeKind::ARRAY);
            bool right_is_array = (right_semant && right_semant->get_type() == TypeKind::ARRAY);
            if (left_is_array || right_is_array) {
                if (!(left_is_array && right_is_array)) {
                    cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                         << ": Mixed array and int in '" << op_str << "' comparison" << endl;
                } else {
                    int larity = get<int>(left_semant->get_type_par());
                    int rarity = get<int>(right_semant->get_type_par());
                    if (larity != rarity) {
                        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                             << ": Array operands must have the same arity in '" << op_str << "' comparison" << endl;
                    }
                }
            }
            result_type = TypeKind::INT;
            s_kind = AST_Semant::Kind::Value;
            is_lvalue = false;
            type_par = variant<monostate, string, int>(0);
        }
        // 保留其它运算符分支，如逻辑运算等
        else if (op_str == "&&" || op_str == "||") {
            if (left_semant && left_semant->get_type() != TypeKind::INT) {
                cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                     << ": Left operand of '" << op_str << "' must be boolean, found "
                     << type_kind_string(left_semant->get_type()) << endl;
            }
            if (right_semant && right_semant->get_type() != TypeKind::INT) {
                cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                     << ": Right operand of '" << op_str << "' must be boolean, found "
                     << type_kind_string(right_semant->get_type()) << endl;
            }
            result_type = TypeKind::INT;
            s_kind = AST_Semant::Kind::Value;
            is_lvalue = false;
            type_par = variant<monostate, string, int>(0);
        }
    }
    
    AST_Semant* semant = new AST_Semant(s_kind, result_type, type_par, is_lvalue);
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(Nested* node) {
    DEBUG_PRINT("Visiting Nested");
    CHECK_NULLPTR(node);
    if (node->sl) {
        for (auto s : *(node->sl)) {
            s->accept(*this);
        }
    }
}

void AST_Semant_Visitor::visit(CallStm* node) {
    DEBUG_PRINT("Visiting CallStm");
    CHECK_NULLPTR(node);

    string saved_class = current_class;
    string class_name = current_class;

    if (node->obj) {
        node->obj->accept(*this);
        AST_Semant* obj_semant = semant_map->getSemant(node->obj);
        if (obj_semant && obj_semant->get_type() == TypeKind::CLASS) {
            class_name = get<string>(obj_semant->get_type_par());
            if (!class_name.empty()) {
                current_class = class_name;
            }
        }
    }

    if (node->name) {
        node->name->accept(*this);
        if (name_maps->is_method(class_name, node->name->id)) {
            Type* mtype = name_maps->get_method_type(class_name, node->name->id);
            TypeKind mkind = mtype ? mtype->typeKind : TypeKind::INT;
            variant<monostate, string, int> tpar{};
            if (mtype) {
                if (mkind == TypeKind::CLASS && mtype->cid) tpar = mtype->cid->id;
                else if (mkind == TypeKind::ARRAY && mtype->arity) tpar = mtype->arity->val;
            }
            AST_Semant* msem = new AST_Semant(AST_Semant::Kind::MethodName, mkind, tpar, false);
            semant_map->setSemant(node->name, msem);
        }
    }

    // parameters are evaluated in the caller's context
    current_class = saved_class;

    if (node->par) {
        for (auto p : *(node->par)) {
            CHECK_NULLPTR(p);
            p->accept(*this);
        }
    }

    current_class = saved_class;
}

void AST_Semant_Visitor::visit(Continue* node) {
    DEBUG_PRINT("Visiting Continue");
    CHECK_NULLPTR(node);
    
    if (loop_depth <= 0) {
        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
             << ": 'continue' statement not within a loop" << endl;
    }
}

void AST_Semant_Visitor::visit(Break* node) {
    DEBUG_PRINT("Visiting Break");
    CHECK_NULLPTR(node);
    
    if (loop_depth <= 0) {
        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
             << ": 'break' statement not within a loop" << endl;
    }
}

void AST_Semant_Visitor::visit(Return* node) {
    DEBUG_PRINT("Visiting Return");
    CHECK_NULLPTR(node);
    
    if (current_method.empty()) {
        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
             << ": Return statement outside of method context" << endl;
        return;
    }
    
    Type* method_type = name_maps->get_method_type(current_class, current_method);
    if (!method_type) {
        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
             << ": Cannot determine return type for method '" << current_method << "'" << endl;
        return;
    }
    
    AST_Semant* exp_semant = nullptr;
    
    if (node->exp) {
        node->exp->accept(*this);
        // 如果 return 内表达式是 IntExp，则补充设置其语义信息
        if (node->exp->getASTKind() == ASTKind::IntExp) {
            IntExp* intExp = static_cast<IntExp*>(node->exp);
            AST_Semant* int_sem = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, variant<monostate, string, int>(intExp->val), false);
            semant_map->setSemant(node->exp, int_sem);
        }
        exp_semant = semant_map->getSemant(node->exp);
        
        if (exp_semant) {
            if (exp_semant->get_type() != method_type->typeKind) {
                cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                     << ": Return value type '" << type_kind_string(exp_semant->get_type()) 
                     << "' does not match method return type '" << type_kind_string(method_type->typeKind) << "'" << endl;
            }
            else if (exp_semant->get_type() == TypeKind::CLASS) {
                string return_class = get<string>(exp_semant->get_type_par());
                string method_class = method_type->cid ? method_type->cid->id : "";
                
                if (return_class != method_class) {
                    auto ancestors = name_maps->get_ancestors(return_class);
                    if (ancestors.find(method_class) == ancestors.end()) {
                        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                             << ": Return class type '" << return_class 
                             << "' is not compatible with method return class type '" << method_class << "'" << endl;
                    }
                }
            }
            else if (exp_semant->get_type() == TypeKind::ARRAY) {
                int return_arity = get<int>(exp_semant->get_type_par());
                int method_arity = method_type->arity ? method_type->arity->val : 0;
                
                if (return_arity != method_arity) {
                    cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                         << ": Return array dimension " << return_arity 
                         << " does not match method return array dimension " << method_arity << endl;
                }
            }
        }
    }
    else if (method_type->typeKind != TypeKind::INT) {
        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
             << ": Missing return value for non-void method" << endl;
    }
    
    // 补充设置 Return 节点自身的语义信息，例如认为其返回 INT 类型（可根据场景调整）
    AST_Semant* ret_sem = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, variant<monostate, string, int>{}, false);
    semant_map->setSemant(node, ret_sem);
}

void AST_Semant_Visitor::visit(PutInt* node) {
    DEBUG_PRINT("Visiting PutInt");
    CHECK_NULLPTR(node);
    if (node->exp)
        node->exp->accept(*this);
}

void AST_Semant_Visitor::visit(PutCh* node) {
    DEBUG_PRINT("Visiting PutCh");
    CHECK_NULLPTR(node);
    if (node->exp)
        node->exp->accept(*this);
}

void AST_Semant_Visitor::visit(PutArray* node) {
    DEBUG_PRINT("Visiting PutArray");
    CHECK_NULLPTR(node);
    if (node->n)
        node->n->accept(*this);
    if (node->arr)
        node->arr->accept(*this);
}

void AST_Semant_Visitor::visit(Starttime* node) {
    DEBUG_PRINT("Visiting Starttime");
    CHECK_NULLPTR(node);
}

void AST_Semant_Visitor::visit(Stoptime* node) {
    DEBUG_PRINT("Visiting Stoptime");
    CHECK_NULLPTR(node);
}

void AST_Semant_Visitor::visit(UnaryOp* node) {
    DEBUG_PRINT("Visiting UnaryOp");
    CHECK_NULLPTR(node);
    if (node->exp) {
        node->exp->accept(*this);
    }
    AST_Semant* semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT,
                                        variant<monostate, string, int>{}, false);
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(ArrayExp* node) {
    DEBUG_PRINT("Visiting ArrayExp");
    CHECK_NULLPTR(node);
    
    if (node->arr)
        node->arr->accept(*this);
    if (node->index)
        node->index->accept(*this);

    AST_Semant* arr_semant = semant_map->getSemant(node->arr);
    AST_Semant* index_semant = semant_map->getSemant(node->index);
    
    if (arr_semant == nullptr || arr_semant->get_type() != TypeKind::ARRAY) {
        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
             << ": Expression used as array is not of array type" << endl;
        if (arr_semant) {
            cerr << "  Found type: " << type_kind_string(arr_semant->get_type()) << endl;
        }
    }
    
    if (index_semant == nullptr || index_semant->get_type() != TypeKind::INT) {
        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
             << ": Array index expression is not of integer type" << endl;
        if (index_semant) {
            cerr << "  Found type: " << type_kind_string(index_semant->get_type()) << endl;
        }
    }
    
    // 设定数组表达式将返回其元素类型，这里默认元素类型为 INT，lvalue 为 true
    int arity = 0;
    if (node->arr) {
        AST_Semant* arr_sem = semant_map->getSemant(node->arr);
        try {
            arity = get<int>(arr_sem->get_type_par());
        } catch (...) {
            arity = 0;
        }
    }
    // 此处将 s_kind 设为 Value，typeKind 根据需要设置（示例中设为 INT），并保存 arity 信息
    AST_Semant* array_sem = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, variant<monostate, string, int>(arity), true);
    semant_map->setSemant(node, array_sem);
}

void AST_Semant_Visitor::visit(CallExp* node) {
    DEBUG_PRINT("Visiting CallExp");
    CHECK_NULLPTR(node);
    
    string saved_class = current_class;
    string class_name = current_class;
    string method_name = "";
    
    if (node->obj) {
        node->obj->accept(*this);
        
        AST_Semant* obj_semant = semant_map->getSemant(node->obj);
        
        if (obj_semant && obj_semant->get_type() == TypeKind::CLASS) {
            class_name = get<string>(obj_semant->get_type_par());
            if (!class_name.empty()) {
                DEBUG_PRINT("Setting context to class: " << class_name);
                current_class = class_name;
            }
        } else {
            cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                 << ": Method call on non-object expression" << endl;
            if (obj_semant) {
                cerr << "  Object has type: " << type_kind_string(obj_semant->get_type()) << endl;
            }
        }
    }
    
    if (node->name) {
        method_name = node->name->id;

        if (!name_maps->is_method(class_name, method_name)) {
            cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                 << ": Method '" << method_name << "' not found in class '" << class_name << "'" << endl;
        } else {
            // record semantic info for the method identifier itself
            Type* mtype = name_maps->get_method_type(class_name, method_name);
            TypeKind mkind = mtype ? mtype->typeKind : TypeKind::INT;
            variant<monostate, string, int> tpar{};
            if (mtype) {
                if (mkind == TypeKind::CLASS && mtype->cid) {
                    tpar = mtype->cid->id;
                } else if (mkind == TypeKind::ARRAY && mtype->arity) {
                    tpar = mtype->arity->val;
                }
            }
            AST_Semant* msem = new AST_Semant(AST_Semant::Kind::MethodName, mkind, tpar, false);
            semant_map->setSemant(node->name, msem);
        }
    }

    // parameters are evaluated in the caller's context
    current_class = saved_class;

    vector<AST_Semant*> param_semants;
    if (node->par) {
        for (auto p : *(node->par)) {
            CHECK_NULLPTR(p);
            p->accept(*this);
            
            AST_Semant* param_semant = semant_map->getSemant(p);
            if (param_semant) {
                param_semants.push_back(param_semant);
            }
        }
    }
    
    Type* method_type = name_maps->get_method_type(class_name, method_name);
    AST_Semant::Kind s_kind = AST_Semant::Kind::Value;
    TypeKind return_type = TypeKind::INT;
    variant<monostate, string, int> type_par{};
    
    if (method_type) {
        return_type = method_type->typeKind;
        
        if (return_type == TypeKind::CLASS && method_type->cid) {
            type_par = method_type->cid->id;
        } else if (return_type == TypeKind::ARRAY && method_type->arity) {
            type_par = method_type->arity->val;
        }
        
        vector<string> formal_names = name_maps->get_method_formal_list_names(class_name, method_name);
        
        int expected_params = formal_names.empty() ? 0 : formal_names.size() - 1;
        
        if (param_semants.size() != expected_params) {
            cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                 << ": Method '" << method_name << "' expects " << expected_params 
                 << " parameters, but " << param_semants.size() << " were provided" << endl;
        } else {
            for (int i = 0; i < expected_params && i < param_semants.size(); i++) {
                Formal* formal = name_maps->get_method_formal(class_name, method_name, formal_names[i]);
                if (formal && formal->type && param_semants[i]) {
                    TypeKind formal_type = formal->type->typeKind;
                    TypeKind actual_type = param_semants[i]->get_type();
                    
                    if (formal_type != actual_type) {
                        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                             << ": Parameter " << (i+1) << " of method '" << method_name
                             << "' expects type '" << type_kind_string(formal_type)
                             << "', but got '" << type_kind_string(actual_type) << "'" << endl;
                        
                        if (formal_type == TypeKind::CLASS && formal->type->cid) {
                            cerr << "  Expected class type: " << formal->type->cid->id << endl;
                        }
                        if (actual_type == TypeKind::CLASS && !param_semants[i]->get_type_par().valueless_by_exception()) {
                            cerr << "  Provided class type: " << get<string>(param_semants[i]->get_type_par()) << endl;
                        }
                    }
                }
            }
        }
    } else if (!method_name.empty()) {
        if (node->getPos()) {
            cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                 << ": Cannot determine return type for method '" << method_name << "'" << endl;
        } else {
            cerr << "Error: Cannot determine return type for method '" << method_name << "'" << endl;
        }
    }
    
    AST_Semant* semant = new AST_Semant(
        s_kind,
        return_type,
        type_par,
        false
    );
    semant_map->setSemant(node, semant);
    
    current_class = saved_class;
}

void AST_Semant_Visitor::visit(ClassVar* node) {
    DEBUG_PRINT("Visiting ClassVar");
    CHECK_NULLPTR(node);
    
    string saved_class = current_class;
    
    if (node->obj) {
        node->obj->accept(*this);
        
        AST_Semant* obj_semant = semant_map->getSemant(node->obj);
        
        if (obj_semant && obj_semant->get_type() == TypeKind::CLASS) {
            auto class_name = get<string>(obj_semant->get_type_par());
            if (!class_name.empty() && node->id) {
                DEBUG_PRINT("Setting context to class: " << class_name);
                current_class = class_name;
                
                string var_name = node->id->id;
                VarDecl* var = find_var_in_class_hierarchy(class_name, var_name);
                
                if (var && var->type) {
                    AST_Semant* semant = new AST_Semant(
                        AST_Semant::Kind::Value, 
                        var->type->typeKind,
                        var->type->typeKind == TypeKind::CLASS ? 
                            variant<monostate, string, int>(var->type->cid ? var->type->cid->id : "") :
                            (var->type->typeKind == TypeKind::ARRAY ?
                                variant<monostate, string, int>(var->type->arity ? var->type->arity->val : 0) :
                                variant<monostate, string, int>{}),
                        true
                    );
                    semant_map->setSemant(node, semant);
                }
            }
        }
    }
    
    if (node->id)
        node->id->accept(*this);
    
    current_class = saved_class;
}

void AST_Semant_Visitor::visit(BoolExp* node) {
    DEBUG_PRINT("Visiting BoolExp");
    CHECK_NULLPTR(node);
    int val = node->val ? 1 : 0;
    AST_Semant* semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, variant<monostate, string, int>(val), false);
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(This* node) {
    DEBUG_PRINT("Visiting This");
    CHECK_NULLPTR(node);
    
    if (current_class.empty() || current_class == "MainClass") {
        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
             << ": 'this' keyword used outside of a class context" << endl;
        if (current_class == "MainClass") {
            cerr << "  'this' cannot be used in the main method" << endl;
        }
    }
    
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        TypeKind::CLASS,
        variant<monostate, string, int>(current_class),
        false
    );
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(Length* node) {
    DEBUG_PRINT("Visiting Length");
    CHECK_NULLPTR(node);
    
    if (node->exp) {
        if (node->exp->getPos()) {
            cerr << "Debug: Length exp position - sline:" << node->exp->getPos()->sline 
                 << ", scolumn:" << node->exp->getPos()->scolumn << endl;
        } else {
            cerr << "Debug: Length exp position is NULL!" << endl;
        }
        
        node->exp->accept(*this);
        
        if (node->exp->getASTKind() == ASTKind::IdExp && semant_map->getSemant(node->exp) == nullptr) {
            IdExp* idExp = static_cast<IdExp*>(node->exp);
            AST_Semant* array_semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::ARRAY,
                                                   variant<monostate, string, int>(0), true);
            semant_map->setSemant(idExp, array_semant);
        }
        
        AST_Semant* exp_semant = semant_map->getSemant(node->exp);
        
        if (exp_semant == nullptr || exp_semant->get_type() != TypeKind::ARRAY) {
            cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
                 << ": Length operator applied to non-array expression";
            if (exp_semant) {
                cerr << ", found type: " << type_kind_string(exp_semant->get_type());
                if (node->exp->getASTKind() == ASTKind::IdExp) {
                    IdExp* idExp = static_cast<IdExp*>(node->exp);
                    cerr << " for variable '" << idExp->id << "'";
                }
            } else {
                cerr << ", unable to determine expression type";
            }
            cerr << endl;
        }
    }
    
    // Length 总是返回 INT 类型
    AST_Semant* semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT,
                                        variant<monostate, string, int>{}, false);
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(Esc* node) {
    DEBUG_PRINT("Visiting Esc");
    CHECK_NULLPTR(node);
    if (node->sl) {
        for (auto s : *(node->sl)) {
            s->accept(*this);
        }
    }
    if (node->exp) {
        node->exp->accept(*this);
    }
    AST_Semant* semant = semant_map->getSemant(node->exp);
    if (semant == nullptr) {
        semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, variant<monostate, string, int>{}, false);
    }
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(GetInt* node) {
    DEBUG_PRINT("Visiting GetInt");
    CHECK_NULLPTR(node);
    AST_Semant* semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT,
                                        variant<monostate, string, int>{}, false);
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(GetCh* node) {
    DEBUG_PRINT("Visiting GetCh");
    CHECK_NULLPTR(node);
}

void AST_Semant_Visitor::visit(GetArray* node) {
    DEBUG_PRINT("Visiting GetArray");
    CHECK_NULLPTR(node);
    if (node->exp)
        node->exp->accept(*this);
}

void AST_Semant_Visitor::visit(OpExp* node) {
    DEBUG_PRINT("Visiting OpExp");
    CHECK_NULLPTR(node);
}

VarDecl* AST_Semant_Visitor::find_var_in_class_hierarchy(const string& class_name, const string& var_name) {
    if (name_maps->is_class_var(class_name, var_name)) {
        return name_maps->get_class_var(class_name, var_name);
    }
    
    set<string> ancestors = name_maps->get_ancestors(class_name);
    for (auto ancestor : ancestors) {
        if (name_maps->is_class_var(ancestor, var_name)) {
            return name_maps->get_class_var(ancestor, var_name);
        }
    }
    
    if (name_maps->is_class_var("MainClass", var_name)) {
        return name_maps->get_class_var("MainClass", var_name);
    }
    return nullptr;
}

void AST_Semant_Visitor::visit(IdExp* node) {
    DEBUG_PRINT("Visiting IdExp: " << node->id);
    CHECK_NULLPTR(node);

    // 如果该IdExp已经有语义信息，则不需要再次设置
    if (semant_map->getSemant(node)) {
        return;
    }

    AST_Semant::Kind s_kind = AST_Semant::Kind::Value;
    TypeKind type_kind = TypeKind::INT; // 默认类型
    variant<monostate, string, int> type_par{};
    bool is_lvalue = true; // 默认为左值（大多数情况）
    bool found = false;

    DEBUG_PRINT("Searching for identifier: " << node->id 
               << " in class: " << (current_class.empty() ? "none" : current_class)
               << ", method: " << (current_method.empty() ? "none" : current_method));

    // 1. 最高优先级：方法形参（参数）
    if (!current_method.empty() && name_maps->is_method_formal(current_class, current_method, node->id)) {
        Formal* formal = name_maps->get_method_formal(current_class, current_method, node->id);
        if (formal && formal->type) {
            type_kind = formal->type->typeKind;
            
            if (type_kind == TypeKind::CLASS && formal->type->cid) {
                type_par = formal->type->cid->id;
            } else if (type_kind == TypeKind::ARRAY && formal->type->arity) {
                type_par = formal->type->arity->val;
            }
            found = true;
            DEBUG_PRINT("Found as method formal with type: " << type_kind_string(type_kind));
        }
    }
    
    // 2. 局部变量
    else if (!current_method.empty() && name_maps->is_method_var(current_class, current_method, node->id)) {
        VarDecl* var_decl = name_maps->get_method_var(current_class, current_method, node->id);
        if (var_decl && var_decl->type) {
            type_kind = var_decl->type->typeKind;
            
            if (type_kind == TypeKind::CLASS && var_decl->type->cid) {
                type_par = var_decl->type->cid->id;
            } else if (type_kind == TypeKind::ARRAY && var_decl->type->arity) {
                type_par = var_decl->type->arity->val;
            }
            found = true;
            DEBUG_PRINT("Found as method variable with type: " << type_kind_string(type_kind));
        }
    }
    
    // 3. 类成员变量（包括继承的）
    else if (!current_class.empty()) {
        // 当前类的成员变量
        if (name_maps->is_class_var(current_class, node->id)) {
            VarDecl* var_decl = name_maps->get_class_var(current_class, node->id);
            if (var_decl && var_decl->type) {
                type_kind = var_decl->type->typeKind;
                
                if (type_kind == TypeKind::CLASS && var_decl->type->cid) {
                    type_par = var_decl->type->cid->id;
                } else if (type_kind == TypeKind::ARRAY && var_decl->type->arity) {
                    type_par = var_decl->type->arity->val;
                }
                found = true;
                DEBUG_PRINT("Found as current class variable with type: " << type_kind_string(type_kind));
            }
        }
        // 父类的成员变量
        else {
            set<string> ancestors = name_maps->get_ancestors(current_class);
            for (const auto& ancestor : ancestors) {
                if (name_maps->is_class_var(ancestor, node->id)) {
                    VarDecl* var_decl = name_maps->get_class_var(ancestor, node->id);
                    if (var_decl && var_decl->type) {
                        type_kind = var_decl->type->typeKind;
                        
                        if (type_kind == TypeKind::CLASS && var_decl->type->cid) {
                            type_par = var_decl->type->cid->id;
                        } else if (type_kind == TypeKind::ARRAY && var_decl->type->arity) {
                            type_par = var_decl->type->arity->val;
                        }
                        found = true;
                        DEBUG_PRINT("Found as ancestor class variable with type: " << type_kind_string(type_kind));
                        break;
                    }
                }
            }
        }

        // 4. 方法名（当前类的方法）
        if (!found && name_maps->is_method(current_class, node->id)) {
            s_kind = AST_Semant::Kind::MethodName;
            Type* method_type = name_maps->get_method_type(current_class, node->id);
            
            if (method_type) {
                type_kind = method_type->typeKind;
                
                if (type_kind == TypeKind::CLASS && method_type->cid) {
                    type_par = method_type->cid->id;
                } else if (type_kind == TypeKind::ARRAY && method_type->arity) {
                    type_par = method_type->arity->val;
                }
            }
            is_lvalue = false;
            found = true;
            DEBUG_PRINT("Found as current class method with return type: " << type_kind_string(type_kind));
        }
        
        // 5. 方法名（父类的方法）
        if (!found) {
            set<string> ancestors = name_maps->get_ancestors(current_class);
            for (const auto& ancestor : ancestors) {
                if (name_maps->is_method(ancestor, node->id)) {
                    s_kind = AST_Semant::Kind::MethodName;
                    Type* method_type = name_maps->get_method_type(ancestor, node->id);
                    
                    if (method_type) {
                        type_kind = method_type->typeKind;
                        
                        if (type_kind == TypeKind::CLASS && method_type->cid) {
                            type_par = method_type->cid->id;
                        } else if (type_kind == TypeKind::ARRAY && method_type->arity) {
                            type_par = method_type->arity->val;
                        }
                    }
                    is_lvalue = false;
                    found = true;
                    DEBUG_PRINT("Found as ancestor class method with return type: " << type_kind_string(type_kind));
                    break;
                }
            }
        }
    }
    
    // 6. 检查是否为类名
    if (!found && name_maps->is_class(node->id)) {
        s_kind = AST_Semant::Kind::ClassName;
        type_kind = TypeKind::CLASS;
        type_par = node->id;  // 类型参数为类名本身
        is_lvalue = false;    // 类名不能作为左值
        found = true;
        DEBUG_PRINT("Found as class name: " << node->id);
    }
    
    // 7. 最后检查MainClass的变量（全局变量）
    if (!found && name_maps->is_class_var("MainClass", node->id)) {
        VarDecl* var_decl = name_maps->get_class_var("MainClass", node->id);
        if (var_decl && var_decl->type) {
            type_kind = var_decl->type->typeKind;
            
            if (type_kind == TypeKind::CLASS && var_decl->type->cid) {
                type_par = var_decl->type->cid->id;
            } else if (type_kind == TypeKind::ARRAY && var_decl->type->arity) {
                type_par = var_decl->type->arity->val;
            }
            found = true;
            DEBUG_PRINT("Found as MainClass variable with type: " << type_kind_string(type_kind));
        }
    }

    if (!found) {
        cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
             << ": Undefined identifier: '" << node->id << "'" << endl;
        cerr << "  Current context - Class: " << (current_class.empty() ? "none" : current_class)
             << ", Method: " << (current_method.empty() ? "none" : current_method) << endl;
        return;
    }

    AST_Semant* semant = new AST_Semant(s_kind, type_kind, type_par, is_lvalue);
    DEBUG_PRINT("Setting semantic info for IdExp " << node->id << " with kind: " << AST_Semant::s_kind_string(s_kind)
               << ", type: " << type_kind_string(type_kind));
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(IntExp* node) {
    DEBUG_PRINT("Visiting IntExp: " << node->val);
    CHECK_NULLPTR(node);
    
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        TypeKind::INT,
        variant<monostate, string, int>(node->val),
        false
    );
    semant_map->setSemant(node, semant);
}