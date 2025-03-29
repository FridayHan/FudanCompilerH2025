#define DEBUG
#undef DEBUG

#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "namemaps.hh"
#include <algorithm>
#include <iostream>
#include <map>
#include <variant>
#include <vector>

using namespace std;
using namespace fdmj;

void AST_Name_Map_Visitor::visit(Program *node) {
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

void AST_Name_Map_Visitor::visit(MainMethod *node) {
  DEBUG_PRINT("Visiting MainMethod");
  CHECK_NULLPTR(node);
  
  // 将MainMethod当成一个特殊的class，命名为"MainClass"
  string class_name = "MainClass";
  string method_name = "^_main";
  
  // 设置当前上下文
  current_class = class_name;
  current_method = method_name;
  
  // 添加类和方法到名称映射
  name_maps->add_class(class_name);
  name_maps->add_method(class_name, method_name);
  
  // 初始化形参列表
  current_formal_list.clear();
  
  // 为主方法添加一个返回值形参（视为void类型）
  string return_var_name = "^_method_return";
  Type* void_type = new Type(node->getPos());  // 默认类型为INT，这里用作void
  Formal* return_formal = new Formal(node->getPos(), void_type, new IdExp(node->getPos(), return_var_name));
  
  // 添加到方法形参中
  name_maps->add_method_formal(class_name, method_name, return_var_name, return_formal);
  current_formal_list.push_back(return_var_name);
  
  // 添加方法形参列表
  name_maps->add_method_formal_list(class_name, method_name, current_formal_list);
  
  if (node->vdl != nullptr) {
    for (auto vd : *(node->vdl)) {
      vd->accept(*this);
    }
  }
  if (node->sl != nullptr) {
    for (auto s : *(node->sl)) {
      s->accept(*this);
    }
  }
  
  // 访问完毕后重置上下文
  current_class = "";
  current_method = "";
}

void AST_Name_Map_Visitor::visit(ClassDecl *node) {
  DEBUG_PRINT("Visiting ClassDecl");
  CHECK_NULLPTR(node);
  
  // 记录类名和继承关系
  string class_name = node->id->id;
  current_class = class_name;
  name_maps->add_class(class_name);
  
  // 如果有继承关系，添加到类层次结构中
  if (node->eid != nullptr) {
    string parent_name = node->eid->id;
    name_maps->add_class_hiearchy(class_name, parent_name);
  }
  
  if (node->vdl != nullptr) {
    for (auto vd : *(node->vdl)) {
      vd->accept(*this);
    }
  }
  if (node->mdl != nullptr) {
    for (auto md : *(node->mdl)) {
      md->accept(*this);
    }
  }
  
  // 访问完类后重置当前类名
  current_class = "";
  
  if (node->id != nullptr) {
    node->id->accept(*this);
  }
  if (node->eid != nullptr) {
    node->eid->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(Type *node) {
  DEBUG_PRINT("Visiting Type");
  CHECK_NULLPTR(node);
  if (node->typeKind == TypeKind::CLASS) {
    if (node->cid != nullptr) {
      node->cid->accept(*this);
    }
  }
  if (node->typeKind == TypeKind::ARRAY) {
    if (node->arity != nullptr) {
      node->arity->accept(*this);
    }
  }
}

void AST_Name_Map_Visitor::visit(VarDecl *node) {
  DEBUG_PRINT("Visiting VarDecl");
  CHECK_NULLPTR(node);
  
  // 根据上下文确定是类变量还是方法变量
  if (!current_class.empty()) {
    if (!current_method.empty() && !in_formal) {
      // 方法内的局部变量
      name_maps->add_method_var(current_class, current_method, node->id->id, node);
    } else if (current_method.empty()) {
      // 类变量
      name_maps->add_class_var(current_class, node->id->id, node);
    }
  }
  
  if (node->type != nullptr) {
    node->type->accept(*this);
  }
  if (node->id != nullptr) {
    node->id->accept(*this);
  }
  if (node->init.index() == 1) {
    get<IntExp *>(node->init)->accept(*this);
  } else if (node->init.index() == 2) {
    for (auto e : *(get<vector<IntExp *> *>(node->init))) {
      e->accept(*this);
    }
  }
}

void AST_Name_Map_Visitor::visit(MethodDecl *node) {
  DEBUG_PRINT("Visiting MethodDecl");
  CHECK_NULLPTR(node);
  
  // 记录方法名
  string method_name = node->id->id;
  current_method = method_name;
  name_maps->add_method(current_class, method_name);
  
  // 准备收集形参列表
  current_formal_list.clear();
  in_formal = true;
  
  if (node->fl != nullptr) {
    for (auto f : *(node->fl)) {
      f->accept(*this);
    }
  }
  
  // 添加方法返回值作为特殊形参
  string return_var_name = "^_method_return";
  if (node->type != nullptr) {
    // 创建代表返回值的形参
    Formal* return_formal = new Formal(node->getPos(), node->type->clone(), new IdExp(node->getPos(), return_var_name));
    // 添加到方法形参映射
    name_maps->add_method_formal(current_class, method_name, return_var_name, return_formal);
    current_formal_list.push_back(return_var_name);
  }
  
  // 添加方法形参列表
  name_maps->add_method_formal_list(current_class, method_name, current_formal_list);
  in_formal = false;
  
  if (node->type != nullptr) {
    node->type->accept(*this);
  }
  if (node->id != nullptr) {
    node->id->accept(*this);
  }
  
  if (node->vdl != nullptr) {
    for (auto vd : *(node->vdl)) {
      vd->accept(*this);
    }
  }
  if (node->sl != nullptr) {
    for (auto s : *(node->sl)) {
      s->accept(*this);
    }
  }
  
  // 访问完方法后重置当前方法名
  current_method = "";
}

void AST_Name_Map_Visitor::visit(Formal *node) {
  DEBUG_PRINT("Visiting Formal");
  CHECK_NULLPTR(node);
  
  // 记录方法形参
  if (!current_class.empty() && !current_method.empty() && in_formal) {
    string var_name = node->id->id;
    name_maps->add_method_formal(current_class, current_method, var_name, node);
    current_formal_list.push_back(var_name);
  }
  
  if (node->type != nullptr) {
    node->type->accept(*this);
  }
  if (node->id != nullptr) {
    node->id->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(Nested *node) {
  DEBUG_PRINT("Visiting Nested");
  CHECK_NULLPTR(node);
  if (node->sl != nullptr) {
    for (auto s : *(node->sl)) {
      s->accept(*this);
    }
  }
}

void AST_Name_Map_Visitor::visit(If *node) {
  DEBUG_PRINT("Visiting If");
  CHECK_NULLPTR(node);
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
  if (node->stm1 != nullptr) {
    node->stm1->accept(*this);
  }
  if (node->stm2 != nullptr) {
    node->stm2->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(While *node) {
  DEBUG_PRINT("Visiting While");
  CHECK_NULLPTR(node);
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
  if (node->stm != nullptr) {
    node->stm->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(Assign *node) {
  DEBUG_PRINT("Visiting Assign");
  CHECK_NULLPTR(node);
  if (node->left != nullptr) {
    node->left->accept(*this);
  }
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(CallStm *node) {
  DEBUG_PRINT("Visiting CallStm");
  CHECK_NULLPTR(node);
  if (node->obj != nullptr) {
    node->obj->accept(*this);
  }
  if (node->name != nullptr) {
    node->name->accept(*this);
  }
  if (node->par != nullptr) {
    for (auto p : *(node->par)) {
      p->accept(*this);
    }
  }
}

void AST_Name_Map_Visitor::visit(Continue *node) {
  DEBUG_PRINT("Visiting Continue");
  CHECK_NULLPTR(node);
}

void AST_Name_Map_Visitor::visit(Break *node) {
  DEBUG_PRINT("Visiting Break");
  CHECK_NULLPTR(node);
}

void AST_Name_Map_Visitor::visit(Return *node) {
  DEBUG_PRINT("Visiting Return");
  CHECK_NULLPTR(node);
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(PutInt *node) {
  DEBUG_PRINT("Visiting PutInt");
  CHECK_NULLPTR(node);
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(PutCh *node) {
  DEBUG_PRINT("Visiting PutCh");
  CHECK_NULLPTR(node);
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(PutArray *node) {
  DEBUG_PRINT("Visiting PutArray");
  CHECK_NULLPTR(node);
  if (node->n != nullptr) {
    node->n->accept(*this);
  }
  if (node->arr != nullptr) {
    node->arr->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(Starttime *node) {
  DEBUG_PRINT("Visiting Starttime");
  CHECK_NULLPTR(node);
}

void AST_Name_Map_Visitor::visit(Stoptime *node) {
  DEBUG_PRINT("Visiting Stoptime");
  CHECK_NULLPTR(node);
}

void AST_Name_Map_Visitor::visit(BinaryOp *node) {
  DEBUG_PRINT("Visiting BinaryOp");
  CHECK_NULLPTR(node);
  if (node->left != nullptr) {
    node->left->accept(*this);
  }
  if (node->right != nullptr) {
    node->right->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(UnaryOp *node) {
  DEBUG_PRINT("Visiting UnaryOp");
  CHECK_NULLPTR(node);
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(ArrayExp *node) {
  DEBUG_PRINT("Visiting ArrayExp");
  CHECK_NULLPTR(node);
  if (node->arr != nullptr) {
    node->arr->accept(*this);
  }
  if (node->index != nullptr) {
    node->index->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(CallExp *node) {
  DEBUG_PRINT("Visiting CallExp");
  CHECK_NULLPTR(node);
  if (node->obj != nullptr) {
    node->obj->accept(*this);
  }
  if (node->name != nullptr) {
    node->name->accept(*this);
  }
  if (node->par != nullptr) {
    for (auto p : *(node->par)) {
      p->accept(*this);
    }
  }
}

void AST_Name_Map_Visitor::visit(ClassVar *node) {
  DEBUG_PRINT("Visiting ClassVar");
  CHECK_NULLPTR(node);
  if (node->obj != nullptr) {
    node->obj->accept(*this);
  }
  if (node->id != nullptr) {
    node->id->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(BoolExp *node) {
  DEBUG_PRINT("Visiting BoolExp");
  CHECK_NULLPTR(node);
}

void AST_Name_Map_Visitor::visit(This *node) {
  DEBUG_PRINT("Visiting This");
  CHECK_NULLPTR(node);
}

void AST_Name_Map_Visitor::visit(Length *node) {
  DEBUG_PRINT("Visiting Length");
  CHECK_NULLPTR(node);
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(Esc *node) {
  DEBUG_PRINT("Visiting Esc");
  CHECK_NULLPTR(node);
}

void AST_Name_Map_Visitor::visit(GetInt *node) {
  DEBUG_PRINT("Visiting GetInt");
  CHECK_NULLPTR(node);
}

void AST_Name_Map_Visitor::visit(GetCh *node) {
  DEBUG_PRINT("Visiting GetCh");
  CHECK_NULLPTR(node);
}

void AST_Name_Map_Visitor::visit(GetArray *node) {
  DEBUG_PRINT("Visiting GetArray");
  CHECK_NULLPTR(node);
}

void AST_Name_Map_Visitor::visit(IdExp *node) {
  DEBUG_PRINT("Visiting IdExp");
  CHECK_NULLPTR(node);
}

void AST_Name_Map_Visitor::visit(IntExp *node) {
  DEBUG_PRINT("Visiting IntExp");
  CHECK_NULLPTR(node);
}

void AST_Name_Map_Visitor::visit(OpExp *node) {
  DEBUG_PRINT("Visiting OpExp");
  CHECK_NULLPTR(node);
}
