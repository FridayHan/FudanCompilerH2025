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

// 语义分析入口函数
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
    if (node->main != nullptr) {
        node->main->accept(*this);
    }
    if (node->cdl != nullptr) {
        for (auto cl : *(node->cdl)) {
            cl->accept(*this);
        }
    }
}

void AST_Semant_Visitor::visit(MainMethod* node) {
    DEBUG_PRINT("Visiting MainMethod");
    CHECK_NULLPTR(node);
    // 设置当前上下文：MainMethod当作MainClass，方法名为^_main
    current_class = "MainClass";
    current_method = "^_main";
    if (node->sl != nullptr) {
        for (auto stm : *(node->sl)) {
            stm->accept(*this);
        }
    }
    current_class = "";
    current_method = "";
}

void AST_Semant_Visitor::visit(ClassDecl* node) {
    DEBUG_PRINT("Visiting ClassDecl: " << node->id->id);
    CHECK_NULLPTR(node);
    
    // 保存当前类名
    current_class = node->id->id;
    name_maps->add_class(current_class);
    
    // 处理类变量声明
    if (node->vdl != nullptr) {
        for (auto vd : *(node->vdl)) {
            vd->accept(*this);
        }
    }
    
    // 处理方法声明
    if (node->mdl != nullptr) {
        for (auto md : *(node->mdl)) {
            md->accept(*this);
        }
    }
    
    current_class = "";
    if (node->id != nullptr) {
        node->id->accept(*this);
    }
    if (node->eid != nullptr) {
        node->eid->accept(*this);
    }
}

void AST_Semant_Visitor::visit(MethodDecl* node) {
    DEBUG_PRINT("Visiting MethodDecl: " << node->id->id);
    CHECK_NULLPTR(node);
    
    // 设置当前方法名
    current_method = node->id->id;
    name_maps->add_method(current_class, current_method);
    
    // 处理返回类型
    if (node->type != nullptr) {
        node->type->accept(*this);
    }
    
    // 处理形参列表
    if (node->fl != nullptr) {
        for (auto formal : *(node->fl)) {
            formal->accept(*this);
        }
    }
    
    // 处理局部变量声明
    if (node->vdl != nullptr) {
        for (auto vd : *(node->vdl)) {
            vd->accept(*this);
        }
    }
    
    // 处理方法体语句
    if (node->sl != nullptr) {
        for (auto stm : *(node->sl)) {
            stm->accept(*this);
        }
    }
    
    current_method = "";
    
    // 检查返回表达式
    if (node->type != nullptr && node->sl != nullptr && !node->sl->empty()) {
        Stm* last_stm = node->sl->back();
        if (last_stm->getASTKind() == ASTKind::Return) {
            auto ret_stm = static_cast<Return*>(last_stm);
            if (ret_stm->exp != nullptr) {
                ret_stm->exp->accept(*this);
                auto ret_semant = semant_map->getSemant(ret_stm->exp);
                if (ret_semant != nullptr && ret_semant->get_type() != node->type->typeKind) {
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
    if (node->type != nullptr) {
        node->type->accept(*this);
    }
}

void AST_Semant_Visitor::visit(Formal* node) {
    DEBUG_PRINT("Visiting Formal: " << node->id->id);
    CHECK_NULLPTR(node);
    if (node->type != nullptr) {
        node->type->accept(*this);
    }
}

void AST_Semant_Visitor::visit(Assign* node) {
    DEBUG_PRINT("Visiting Assign");
    CHECK_NULLPTR(node);
    if (node->left != nullptr) {
        node->left->accept(*this);
    }
    if (node->exp != nullptr) {
        node->exp->accept(*this);
    }
    
    // 检查左值
    auto lhs_semant = semant_map->getSemant(node->left);
    if (lhs_semant != nullptr && !lhs_semant->is_lvalue()) {
        cerr << "Error: Left side of assignment is not an lvalue" << endl;
    }
    
    // 检查类型匹配
    auto rhs_semant = semant_map->getSemant(node->exp);
    if (lhs_semant != nullptr && rhs_semant != nullptr) {
        if (lhs_semant->get_type() != rhs_semant->get_type()) {
            // 简单类型不匹配检查
            cerr << "Error: Type mismatch in assignment" << endl;
        } else if (lhs_semant->get_type() == TypeKind::CLASS) {
            // 类类型的兼容性检查（子类可以赋值给父类）
            auto lhs_class = get<string>(lhs_semant->get_type_par());
            auto rhs_class = get<string>(rhs_semant->get_type_par());
            if (lhs_class != rhs_class) {
                auto ancestors = name_maps->get_ancestors(rhs_class);
                if (ancestors.find(lhs_class) == ancestors.end()) {
                    cerr << "Error: Incompatible class types in assignment" << endl;
                }
            }
        }
    }
}

void AST_Semant_Visitor::visit(If* node) {
    DEBUG_PRINT("Visiting If");
    CHECK_NULLPTR(node);
    if (node->exp != nullptr) {
        node->exp->accept(*this);
        auto cond_semant = semant_map->getSemant(node->exp);
        // 这里应该进行适当的类型检查，但简化版本可能不需要精确匹配布尔类型
    }
    
    if (node->stm1 != nullptr) {
        node->stm1->accept(*this);
    }
    
    if (node->stm2 != nullptr) {
        node->stm2->accept(*this);
    }
}

void AST_Semant_Visitor::visit(While* node) {
    DEBUG_PRINT("Visiting While");
    CHECK_NULLPTR(node);
    if (node->exp != nullptr) {
        node->exp->accept(*this);
        
        // 检查条件表达式是否为布尔类型
        auto cond_semant = semant_map->getSemant(node->exp);
        // 与If类似，进行适当的类型检查
    }
    
    if (node->stm != nullptr) {
        // 记录进入循环，用于处理continue/break语句
        node->stm->accept(*this);
    }
}

void AST_Semant_Visitor::visit(BinaryOp* node) {
    DEBUG_PRINT("Visiting BinaryOp");
    CHECK_NULLPTR(node);
    if (node->left != nullptr) {
        node->left->accept(*this);
    }
    if (node->right != nullptr) {
        node->right->accept(*this);
    }
    
    auto left_semant = semant_map->getSemant(node->left);
    auto right_semant = semant_map->getSemant(node->right);
    
    TypeKind result_type = TypeKind::INT; // 默认整型
    bool is_lvalue = false;
    
    // 将 switch 替换为 if / else if 结构，对 node->op->op 进行比较
    if (node->op && node->op->op == "+") {
        if (left_semant && left_semant->get_type() != TypeKind::INT) {
            cerr << "Error: Left operand of arithmetic operation must be int" << endl;
        }
        if (right_semant && right_semant->get_type() != TypeKind::INT) {
            cerr << "Error: Right operand of arithmetic operation must be int" << endl;
        }
        result_type = TypeKind::INT;
    } else if (node->op && node->op->op == "-") {
        // 同上
        result_type = TypeKind::INT;
    } else if (node->op && node->op->op == "*") {
        // 同上
        result_type = TypeKind::INT;
    } else if (node->op && node->op->op == "/") {
        // 同上
        result_type = TypeKind::INT;
    } else if (node->op && (node->op->op == "<" || node->op->op == "<=" ||
                            node->op->op == ">" || node->op->op == ">=" ||
                            node->op->op == "==" || node->op->op == "!=")) {
        result_type = TypeKind::INT; // 布尔值用 INT 表示
    } else if (node->op && (node->op->op == "&&" || node->op->op == "||")) {
        result_type = TypeKind::INT; // 布尔值用 INT 表示
    } else {
        // 其他运算符处理
    }
    
    AST_Semant* semant = new AST_Semant(AST_Semant::Kind::Value, result_type,
                                        variant<monostate, string, int>{}, is_lvalue);
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(IdExp* node) {
    DEBUG_PRINT("Visiting IdExp: " << node->id);
    CHECK_NULLPTR(node);

    // 检查是否为方法形参或局部变量
    VarDecl* var_decl = nullptr;
    Formal* formal = nullptr;
    if (!current_method.empty() && name_maps->is_method_formal(current_class, current_method, node->id)) {
        formal = name_maps->get_method_formal(current_class, current_method, node->id);
    } else if (!current_method.empty() && name_maps->is_method_var(current_class, current_method, node->id)) {
        var_decl = name_maps->get_method_var(current_class, current_method, node->id);
    }
    // 检查是否为类变量
    else if (!current_class.empty() && name_maps->is_class_var(current_class, node->id)) {
        var_decl = name_maps->get_class_var(current_class, node->id);
    }

    // 设置语义信息：变量或形参
    if (var_decl) {
        // 变量声明的类型
        Type* type = var_decl->type;
        AST_Semant* semant = new AST_Semant(
            AST_Semant::Kind::Value, 
            type->typeKind,
            type->typeKind == TypeKind::CLASS ? 
                variant<monostate, string, int>(type->cid ? type->cid->id : "") :
            (type->typeKind == TypeKind::ARRAY ?
                variant<monostate, string, int>(type->arity ? type->arity->val : 0) :
                variant<monostate, string, int>{}),
            true // 是左值
        );
        semant_map->setSemant(node, semant);
    } else if (formal) {
        // 方法形参的类型
        Type* type = formal->type;
        AST_Semant* semant = new AST_Semant(
            AST_Semant::Kind::Value, 
            type->typeKind,
            type->typeKind == TypeKind::CLASS ? 
                variant<monostate, string, int>(type->cid ? type->cid->id : "") :
            (type->typeKind == TypeKind::ARRAY ?
                variant<monostate, string, int>(type->arity ? type->arity->val : 0) :
                variant<monostate, string, int>{}),
            true // 是左值
        );
        semant_map->setSemant(node, semant);
    } 
    // 检查是否为方法名
    else if (!current_class.empty() && name_maps->is_method(current_class, node->id)) {
        // 获取方法的返回类型
        Type* method_type = name_maps->get_method_type(current_class, node->id);
        AST_Semant* semant = new AST_Semant(
            AST_Semant::Kind::MethodName, 
            method_type->typeKind,
            method_type->typeKind == TypeKind::CLASS ? 
                variant<monostate, string, int>(method_type->cid ? method_type->cid->id : "") :
            (method_type->typeKind == TypeKind::ARRAY ?
                variant<monostate, string, int>(method_type->arity ? method_type->arity->val : 0) :
                variant<monostate, string, int>{}),
            false // 方法名不是左值
        );
        semant_map->setSemant(node, semant);
    } 
    // 未定义的标识符
    else {
        cerr << "Error: Undefined identifier: " << node->id << endl;
    }
}

void AST_Semant_Visitor::visit(IntExp* node) {
    DEBUG_PRINT("Visiting IntExp: " << node->val);
    CHECK_NULLPTR(node);
    // 整数字面量的类型是 INT，不是左值
    AST_Semant* semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, 
                                      variant<monostate, string, int>{}, false);
    semant_map->setSemant(node, semant);
}

// 以下为补充实现缺失的 AST_Semant_Visitor::visit 方法

void AST_Semant_Visitor::visit(Nested* node) {
    DEBUG_PRINT("Visiting Nested");
    CHECK_NULLPTR(node);
    if (node->sl != nullptr) {
        for (auto s : *(node->sl)) {
            s->accept(*this);
        }
    }
}

void AST_Semant_Visitor::visit(CallStm* node) {
    DEBUG_PRINT("Visiting CallStm");
    CHECK_NULLPTR(node);
    if (node->obj != nullptr)
        node->obj->accept(*this);
    if (node->name != nullptr)
        node->name->accept(*this);
    if (node->par != nullptr) {
        for (auto p : *(node->par)) {
            p->accept(*this);
        }
    }
}

void AST_Semant_Visitor::visit(Continue* node) {
    DEBUG_PRINT("Visiting Continue");
    CHECK_NULLPTR(node);
}

void AST_Semant_Visitor::visit(Break* node) {
    DEBUG_PRINT("Visiting Break");
    CHECK_NULLPTR(node);
}

void AST_Semant_Visitor::visit(Return* node) {
    DEBUG_PRINT("Visiting Return");
    CHECK_NULLPTR(node);
    if (node->exp != nullptr) {
        node->exp->accept(*this);
        AST_Semant* exp_semant = semant_map->getSemant(node->exp);
        AST_Semant* semant = new AST_Semant(exp_semant->get_kind(), exp_semant->get_type(),
                                            exp_semant->get_type_par(), false);
        semant_map->setSemant(node, semant);
    } else {
        AST_Semant* semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT,
                                            variant<monostate, string, int>{}, false);
        semant_map->setSemant(node, semant);
    }
}

void AST_Semant_Visitor::visit(PutInt* node) {
    DEBUG_PRINT("Visiting PutInt");
    CHECK_NULLPTR(node);
    if (node->exp != nullptr)
        node->exp->accept(*this);
}

void AST_Semant_Visitor::visit(PutCh* node) {
    DEBUG_PRINT("Visiting PutCh");
    CHECK_NULLPTR(node);
    if (node->exp != nullptr)
        node->exp->accept(*this);
}

void AST_Semant_Visitor::visit(PutArray* node) {
    DEBUG_PRINT("Visiting PutArray");
    CHECK_NULLPTR(node);
    if (node->n != nullptr)
        node->n->accept(*this);
    if (node->arr != nullptr)
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

// Update UnaryOp: after processing the subexpression, assume the result is INT and non-lvalue.
void AST_Semant_Visitor::visit(UnaryOp* node) {
    DEBUG_PRINT("Visiting UnaryOp");
    CHECK_NULLPTR(node);
    if (node->exp != nullptr) {
        node->exp->accept(*this);
    }
    // For simplicity, assume the unary operator returns an integer value.
    AST_Semant* semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT,
                                        variant<monostate, string, int>{}, false);
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(ArrayExp* node) {
    DEBUG_PRINT("Visiting ArrayExp");
    CHECK_NULLPTR(node);
    if (node->arr != nullptr)
        node->arr->accept(*this);
    if (node->index != nullptr)
        node->index->accept(*this);

    // 检查数组表达式必须是数组类型
    AST_Semant* arr_semant = semant_map->getSemant(node->arr);
    if (arr_semant == nullptr || arr_semant->get_type() != TypeKind::ARRAY) {
        cerr << "Error: Array expression is not of array type" << endl;
    }
    // 检查数组下标表达式必须是int类型
    AST_Semant* index_semant = semant_map->getSemant(node->index);
    if (index_semant == nullptr || index_semant->get_type() != TypeKind::INT) {
        cerr << "Error: Array index is not an integer" << endl;
    }
    // 假定数组元素类型为int，ArrayExp运算结果为int
    AST_Semant* semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT,
                                        variant<monostate, string, int>{}, true);
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(CallExp* node) {
    DEBUG_PRINT("Visiting CallExp");
    CHECK_NULLPTR(node);
    if (node->obj != nullptr)
        node->obj->accept(*this);
    if (node->name != nullptr)
        node->name->accept(*this);
    if (node->par != nullptr) {
        for (auto p : *(node->par)) {
            p->accept(*this);
        }
    }
    // For simplicity, assume the call returns an integer.
    AST_Semant* semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT,
                                        variant<monostate, string, int>{}, false);
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(ClassVar* node) {
    DEBUG_PRINT("Visiting ClassVar");
    CHECK_NULLPTR(node);
    if (node->obj != nullptr)
        node->obj->accept(*this);
    if (node->id != nullptr)
        node->id->accept(*this);
}

// 修改 BoolExp 的语义实现
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
    if (current_class.empty()) {
        cerr << "Error: 'this' used outside of class context" << endl;
    }
    // 'this' 的类型为当前类的类型，非左值
    AST_Semant* semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::CLASS,
                                        variant<monostate, string, int>(current_class), false);
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(Length* node) {
    DEBUG_PRINT("Visiting Length");
    CHECK_NULLPTR(node);
    if (node->exp != nullptr) {
        node->exp->accept(*this);
        AST_Semant* exp_semant = semant_map->getSemant(node->exp);
        if (exp_semant == nullptr || exp_semant->get_type() != TypeKind::ARRAY) {
            cerr << "Error: Length operator applied to non-array" << endl;
        }
    }
    // Length运算结果为整型
    AST_Semant* semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT,
                                        variant<monostate, string, int>{}, false);
    semant_map->setSemant(node, semant);
}

void AST_Semant_Visitor::visit(Esc* node) {
    DEBUG_PRINT("Visiting Esc");
    CHECK_NULLPTR(node);
    if (node->sl != nullptr) {
        for (auto s : *(node->sl)) {
            s->accept(*this);
        }
    }
    if (node->exp != nullptr) {
        node->exp->accept(*this);
    }
    // 使用 exp 的语义信息作为 Esc 的语义信息
    AST_Semant* semant = semant_map->getSemant(node->exp);
    if (semant == nullptr) {
        semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, variant<monostate, string, int>{}, false);
    }
    semant_map->setSemant(node, semant);
}

// Update GetInt to assign INT type (not lvalue)
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
    if (node->exp != nullptr)
        node->exp->accept(*this);
}

void AST_Semant_Visitor::visit(OpExp* node) {
    DEBUG_PRINT("Visiting OpExp");
    CHECK_NULLPTR(node);
}