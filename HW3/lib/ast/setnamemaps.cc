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

// TODO

void AST_Name_Map_Visitor::visit(Program *node) {
#ifdef DEBUG
  std::cout << "Visiting Program" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  if (node->main != nullptr) {
    node->main->accept(*this);
  }
  if (node->cdl != nullptr) {
    for (auto cl : *(node->cdl)) {
      cl->accept(*this);
    }
  }
}

// rest of the visit functions here
void AST_Name_Map_Visitor::visit(MainMethod *node) {
#ifdef DEBUG
  std::cout << "Visiting MainMethod" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  
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
#ifdef DEBUG
  std::cout << "Visiting ClassDecl" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  
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
#ifdef DEBUG
  std::cout << "Visiting Type" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
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
#ifdef DEBUG
  std::cout << "Visiting VarDecl" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  
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
    std::get<IntExp *>(node->init)->accept(*this);
  } else if (node->init.index() == 2) {
    for (auto e : *(std::get<vector<IntExp *> *>(node->init))) {
      e->accept(*this);
    }
  }
}

void AST_Name_Map_Visitor::visit(MethodDecl *node) {
#ifdef DEBUG
  std::cout << "Visiting MethodDecl" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  
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
#ifdef DEBUG
  std::cout << "Visiting Formal" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  
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
#ifdef DEBUG
  std::cout << "Visiting Nested" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  if (node->sl != nullptr) {
    for (auto s : *(node->sl)) {
      s->accept(*this);
    }
  }
}

void AST_Name_Map_Visitor::visit(If *node) {
#ifdef DEBUG
  std::cout << "Visiting If" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
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
#ifdef DEBUG
  std::cout << "Visiting While" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
  if (node->stm != nullptr) {
    node->stm->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(Assign *node) {
#ifdef DEBUG
  std::cout << "Visiting Assign" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  if (node->left != nullptr) {
    node->left->accept(*this);
  }
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(CallStm *node) {
#ifdef DEBUG
  std::cout << "Visiting CallStm" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
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
#ifdef DEBUG
  std::cout << "Visiting Continue" << std::endl;
#endif
 if (node == nullptr) {
    return;
  }
}

void AST_Name_Map_Visitor::visit(Break *node) {
#ifdef DEBUG
  std::cout << "Visiting Break" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
}

void AST_Name_Map_Visitor::visit(Return *node) {
#ifdef DEBUG
  std::cout << "Visiting Return" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(PutInt *node) {
#ifdef DEBUG
  std::cout << "Visiting PutInt" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(PutCh *node) {
#ifdef DEBUG
  std::cout << "Visiting PutCh" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(PutArray *node) {
#ifdef DEBUG
  std::cout << "Visiting PutArray" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  if (node->n != nullptr) {
    node->n->accept(*this);
  }
  if (node->arr != nullptr) {
    node->arr->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(Starttime *node) {
#ifdef DEBUG
  std::cout << "Visiting Starttime" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
}

void AST_Name_Map_Visitor::visit(Stoptime *node) {
#ifdef DEBUG
  std::cout << "Visiting Stoptime" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
}

void AST_Name_Map_Visitor::visit(BinaryOp *node) {
#ifdef DEBUG
  std::cout << "Visiting BinaryOp" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  if (node->left != nullptr) {
    node->left->accept(*this);
  }
  if (node->right != nullptr) {
    node->right->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(UnaryOp *node) {
#ifdef DEBUG
  std::cout << "Visiting UnaryOp" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(ArrayExp *node) {
#ifdef DEBUG
  std::cout << "Visiting ArrayExp" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  if (node->arr != nullptr) {
    node->arr->accept(*this);
  }
  if (node->index != nullptr) {
    node->index->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(CallExp *node) {
#ifdef DEBUG
  std::cout << "Visiting CallExp" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
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
#ifdef DEBUG
  std::cout << "Visiting ClassVar" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  if (node->obj != nullptr) {
    node->obj->accept(*this);
  }
  if (node->id != nullptr) {
    node->id->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(BoolExp *node) {
#ifdef DEBUG
  std::cout << "Visiting BoolExp" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
}

void AST_Name_Map_Visitor::visit(This *node) {
#ifdef DEBUG
  std::cout << "Visiting This" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
}

void AST_Name_Map_Visitor::visit(Length *node) {
#ifdef DEBUG
  std::cout << "Visiting Length" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
  if (node->exp != nullptr) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(Esc *node) {
#ifdef DEBUG
  std::cout << "Visiting Esc" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
}

void AST_Name_Map_Visitor::visit(GetInt *node) {
#ifdef DEBUG
  std::cout << "Visiting GetInt" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
}

void AST_Name_Map_Visitor::visit(GetCh *node) {
#ifdef DEBUG
  std::cout << "Visiting GetCh" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
}

void AST_Name_Map_Visitor::visit(GetArray *node) {
#ifdef DEBUG
  std::cout << "Visiting GetArray" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
}

void AST_Name_Map_Visitor::visit(IdExp *node) {
#ifdef DEBUG
  std::cout << "Visiting IdExp" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
}

void AST_Name_Map_Visitor::visit(IntExp *node) {
  #ifdef DEBUG
    std::cout << "Visiting IntExp" << std::endl;
  #endif
    if (node == nullptr) {
      return;
    }
  }

void AST_Name_Map_Visitor::visit(OpExp *node) {
#ifdef DEBUG
  std::cout << "Visiting OpExp" << std::endl;
#endif
  if (node == nullptr) {
    return;
  }
}
