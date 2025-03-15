#define DEBUG
//#undef DEBUG

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include "config.hh"
#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "treep.hh"
#include "temp.hh"
#include "ast2tree.hh"

using namespace std;
//using namespace fdmj;
//using namespace tree;

#define VarDeclList vector<fdmj::VarDecl*>
#define ClassDeclList vector<fdmj::ClassDecl*>
#define MethodDeclList vector<fdmj::MethodDecl*>
#define FormalList vector<fdmj::Formal*>

#define AST_Stmlist vector<fdmj::Stm*>
#define AST_Explist vector<fdmj::Exp*>

#define FuncDeclList vector<tree::FuncDecl*>
#define Blocklist vector<tree::Block*>
#define T_Stmlist vector<tree::Stm*>
#define T_Explist vector<tree::Exp*>

tree::Program* ast2tree(fdmj::Program* prog, AST_Semant_Map* semant_map) {
    if (prog == nullptr) {
        return nullptr;
    }
#ifdef DEBUG
    cout << "ast2tree::Converting AST to Tree" << endl;
#endif
    ASTToTreeVisitor visitor;
    visitor.visit(prog);
    return static_cast<tree::Program*>(visitor.getTree());
}

Class_table* generate_class_table(ClassDeclList* cdl) {
#ifdef DEBUG
    cout << "Generating class table" << endl;
#endif
    if (cdl == nullptr) {
        return nullptr;
    }
    map<string, bool> class_var_names; //collecting the class variable names from all classes
    map<string, bool> class_method_names; //collecting the method names from all classes
    for (auto cd : *cdl) {
        VarDeclList* vdl = cd->vdl;
        if (vdl != nullptr) {
            for (auto vd : *vdl) 
                class_var_names[vd->id->id] = true;
        }
        MethodDeclList *mdl = cd->mdl;
        if (mdl != nullptr) {
            for (auto md : *mdl) 
                class_method_names[md->id->id] = true;
        }
    }
    Class_table *ct = new Class_table();
    const int address_size = Compiler_Config::get("address_size");
    int var_pos = 0;
    for (auto pair: class_var_names) {
        ct->var_pos_map[pair.first]=var_pos;
        var_pos = (var_pos + 1)*address_size;
    }
    int method_pos = 0;
    for (auto pair: class_method_names) {
        ct->method_pos_map[pair.first]=method_pos;
        method_pos = (method_pos + 1)*address_size;
    }
    return ct;
}

Var_table* generate_var_table(VarDeclList* vdl) {
    return nullptr;
}

void ASTToTreeVisitor::visit(fdmj::Program* node) {
#ifdef DEBUG
    cout << "Visiting Program" << endl;
#endif
    if (node==nullptr) {
        visit_result = nullptr;
        return;
    }

    Class_table *ct = generate_class_table(node->cdl);
    node->main->accept(*this);
    tree::FuncDecl *fd =  static_cast<tree::FuncDecl*>(visit_result);
#ifdef DEBUG
    cout<< "Main method converted: name=" << fd->name << endl;
    tree::Block *b = fd->blocks->at(0);
    cout << "Main method block: entry label=" << b->entry_label->name() << endl;
#endif
    visit_result = new tree::Program(new FuncDeclList(1, 
                        static_cast<tree::FuncDecl*>(visit_result)));
#ifdef DEBUG
    cout << "Program converted" << endl;
#endif
}

void ASTToTreeVisitor::visit(fdmj::MainMethod* node) {
#ifdef DEBUG
    cout << "Visiting MainMethod" << endl;
#endif
    if (node==nullptr) {
        visit_result = nullptr;
        return;
    }
    //reset the temp map for each method
    delete visitor_temp_map;
    visitor_temp_map = new Temp_map();

    T_Stmlist *sl = new T_Stmlist();
    for (auto stm : *node->sl) {
        stm->accept(*this);
        sl->push_back(static_cast<tree::Stm*>(visit_result));
    }
    if (sl->size() == 0 || sl->back()->getTreeKind() != Kind::RETURN)
        sl->push_back(new tree::Return(new tree::Const(0))); //add a return if last statement is not a return
    //create a label for the entry block
    tree::Label *entry_label = visitor_temp_map->newlabel();
    cout << "Entry label: " << entry_label->name() << endl;
    sl->push_back(new tree::LabelStm(entry_label)); //add the entry label at the end
    rotate(sl->rbegin(), sl->rbegin()+1, sl->rend()); //move the entry label beginning
    //setting up the visit result which is a function declaration
    visit_result = new tree::FuncDecl("main", nullptr,  //no arguments
        new Blocklist(1, new tree::Block(entry_label, nullptr, sl)),
        tree::Type::INT, //return type is int
        visitor_temp_map->next_temp-1, //remember the last temp and label numbers used
        visitor_temp_map->next_label-1);
}

void ASTToTreeVisitor::visit(fdmj::ClassDecl *node) {
#ifdef DEBUG
    cout << "Visiting ClassDecl" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::Type* node) {
#ifdef DEBUG
    cout << "Visiting Type" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::VarDecl* node) {
#ifdef DEBUG
    cout << "Visiting VarDecl" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::MethodDecl* node) {
#ifdef DEBUG
    cout << "Visiting MethodDecl" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::Formal* node) {
#ifdef DEBUG
    cout << "Visiting Formal" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::Nested* node) {
#ifdef DEBUG
    cout << "Visiting Nested" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::If* node) {
#ifdef DEBUG
    cout << "Visiting If" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::While* node) {
#ifdef DEBUG
    cout << "Visiting While" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::Assign* node) {
#ifdef DEBUG
    cout << "Visiting Assign" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    node->left->accept(*this);
    tree::TempExp *id = static_cast<tree::TempExp*>(visit_result);
    node->exp->accept(*this);
    tree::Exp *exp = static_cast<tree::Exp*>(visit_result);
    visit_result = new tree::Move(id, exp);
}

void ASTToTreeVisitor::visit(fdmj::CallStm* node) {
#ifdef DEBUG
    cout << "Visiting CallStm" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::Continue* node) {
#ifdef DEBUG
    cout << "Visiting Continue" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::Break* node) {
#ifdef DEBUG
    cout << "Visiting Break" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::Return* node) {
#ifdef DEBUG
    cout << "Visiting Return" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    node->exp->accept(*this);
    visit_result = new tree::Return(static_cast<tree::Exp*>(visit_result));
}

void ASTToTreeVisitor::visit(fdmj::PutInt* node) {
#ifdef DEBUG
    cout << "Visiting PutInt" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::PutCh* node) {
#ifdef DEBUG
    cout << "Visiting PutCh" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::PutArray* node) {
#ifdef DEBUG
    cout << "Visiting PutArray" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::Starttime* node) {
#ifdef DEBUG
    cout << "Visiting Starttime" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::Stoptime* node) {
#ifdef DEBUG
    cout << "Visiting Stoptime" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::BinaryOp* node) {
#ifdef DEBUG
    cout << "Visiting BinaryOp" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    node->left->accept(*this);
    tree::Exp *l = static_cast<tree::Exp*>(visit_result);
    node->right->accept(*this);
    tree::Exp *r = static_cast<tree::Exp*>(visit_result);
    visit_result = new tree::Binop(tree::Type::INT, node->op->op, l, r);
}

void ASTToTreeVisitor::visit(fdmj::UnaryOp* node) {
#ifdef DEBUG
    cout << "Visiting UnaryOp" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    node->exp->accept(*this);
    if (node->op->op != "-") { //only support negation
        cerr << "Unary operator not supported: " << node->op->op << endl;
        visit_result = nullptr;
        return;
    }
    //using 0-exp to represent negation
    visit_result = new tree::Binop(tree::Type::INT, "-", new tree::Const(0), static_cast<tree::Exp*>(visit_result));
}

void ASTToTreeVisitor::visit(fdmj::ArrayExp* node) {
#ifdef DEBUG
    cout << "Visiting ArrayExp" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::CallExp* node) {
#ifdef DEBUG
    cout << "Visiting CallExp" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::ClassVar* node) {
#ifdef DEBUG
    cout << "Visiting ClassVar" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::BoolExp* node) {
#ifdef DEBUG
    cout << "Visiting BoolExp" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::This* node) {
#ifdef DEBUG
    cout << "Visiting This" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::Length* node) {
#ifdef DEBUG
    cout << "Visiting Length" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::Esc* node) {
#ifdef DEBUG
    cout << "Visiting Esc" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    T_Stmlist *sl = new T_Stmlist();
    for (auto stm : *node->sl) {
        stm->accept(*this);
        sl->push_back(static_cast<tree::Stm*>(visit_result));
    }
    node->exp->accept(*this);
    visit_result = new tree::Eseq(tree::Type::INT, new tree::Seq(sl), static_cast<tree::Exp*>(visit_result));
}

void ASTToTreeVisitor::visit(fdmj::GetInt* node) {
#ifdef DEBUG
    cout << "Visiting GetInt" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::GetCh* node) {
#ifdef DEBUG
    cout << "Visiting GetCh" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::GetArray* node) {
#ifdef DEBUG
    cout << "Visiting GetArray" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::IdExp* node) {
#ifdef DEBUG
    cout << "Visiting IdExp" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    tree::Temp *t;
    /*
    if (current_variable_map->find(node->id) == current_variable_map->end()) {
        t = visitor_temp_map->Temp_newtemp();
        current_variable_map->insert({node->id, t});
    } else {
        t = current_variable_map->at(node->id);
    }*/
    visit_result = new tree::TempExp(tree::Type::INT, t);
}

//won't be called for this visitor
void ASTToTreeVisitor::visit(fdmj::OpExp* node) {
#ifdef DEBUG
    cout << "Visiting OpExp" << endl;
#endif
    visit_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::IntExp* node) {
#ifdef DEBUG
    cout << "Visiting IntExp" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    visit_result = new tree::Const(node->val);
}
