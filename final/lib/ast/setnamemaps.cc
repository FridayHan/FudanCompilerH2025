#define DEBUG
#undef DEBUG

#include <algorithm>
#include <iostream>
#include <map>
#include <variant>
#include <vector>
#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "namemaps.hh"
#include "debug.hh"

using namespace std;
using namespace fdmj;

void AST_Name_Map_Visitor::visit(Program *node) {
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

void AST_Name_Map_Visitor::visit(MainMethod *node) {
  DEBUG_PRINT("Visiting MainMethod");
  CHECK_NULLPTR(node);
  
  string class_name = "MainClass";
  string method_name = "^_main";
  
  current_class = class_name;
  current_method = method_name;
  
  name_maps->add_class(class_name);
  name_maps->add_method(class_name, method_name);
  
  current_formal_list.clear();
  
  string return_var_name = "^_method_return";
  Type* main_type = new Type(node->getPos());
  Formal* return_formal = new Formal(node->getPos(), main_type, new IdExp(node->getPos(), return_var_name));
  
  name_maps->add_method_formal(class_name, method_name, return_var_name, return_formal);
  current_formal_list.push_back(return_var_name);
  
  name_maps->add_method_formal_list(class_name, method_name, current_formal_list);
  
  if (node->vdl) {
    for (auto vd : *(node->vdl)) {
      CHECK_NULLPTR(vd);
      vd->accept(*this);
    }
  }
  if (node->sl) {
    for (auto s : *(node->sl)) {
      CHECK_NULLPTR(s);
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
  CHECK_NULLPTR(node->id); // 增加对 id 的空指针检查
  
  // 记录类名
  string class_name = node->id->id;
  current_class = class_name;
  name_maps->add_class(class_name);
  
  // 先处理类自己的变量和方法
  if (node->vdl) {
    for (auto vd : *(node->vdl)) {
      CHECK_NULLPTR(vd); // 检查每个变量声明指针
      vd->accept(*this);
    }
  }
  
  if (node->mdl) {
    for (auto md : *(node->mdl)) {
      CHECK_NULLPTR(md);
      md->accept(*this);
    }
  }
  
  // 然后处理继承关系
  if (node->eid) {
    CHECK_NULLPTR(node->eid); // 增加对 eid 的空指针检查
    string parent_name = node->eid->id;
    
    // 检查父类是否存在
    if (!name_maps->is_class(parent_name)) {
      cerr << "Error at line " << node->getPos()->sline << ", column " << node->getPos()->scolumn
           << ": Parent class '" << parent_name << "' not found" << endl;
    } else {
      name_maps->add_class_hiearchy(class_name, parent_name);
      
      // 处理继承的变量
      vector<string> parent_vars = name_maps->get_class_var_names(parent_name);
      for (auto var_name : parent_vars) {
        // 检查子类是否已有同名变量
        if (name_maps->is_class_var(class_name, var_name)) {
          VarDecl* child_var = name_maps->get_class_var(class_name, var_name);
          VarDecl* parent_var = name_maps->get_class_var(parent_name, var_name);
          
          if (child_var && parent_var) {
            cerr << "Error at line " << child_var->getPos()->sline << ", column " << child_var->getPos()->scolumn
                 << ": Variable '" << var_name << "' in class '" << class_name 
                 << "' conflicts with inherited variable from parent class '" << parent_name << "'" << endl;
          }
        } else {
          // 将父类变量继承到子类
          name_maps->inherit_var(parent_name, class_name, var_name);
        }
      }
      
      // 处理继承的方法
      vector<string> parent_methods = name_maps->get_class_method_names(parent_name);
      for (auto method_name : parent_methods) {
        // 检查子类是否已有同名方法
        if (name_maps->is_method(class_name, method_name)) {
          // 检查方法签名是否一致
          if (!name_maps->check_method_signature(parent_name, method_name, class_name, method_name)) {
            cerr << "Error: Method '" << method_name << "' in class '" << class_name 
                 << "' does not match signature of inherited method from parent class '" 
                 << parent_name << "'" << endl;
          }
          // 如果签名一致，子类方法覆盖父类方法，不需要额外处理
        } else {
          // 将父类方法添加到子类
          name_maps->add_method(class_name, method_name);
          
          // 使用新方法获取形参列表
          vector<string> param_list = name_maps->get_method_formal_list(parent_name, method_name);
          if (!param_list.empty()) {
            name_maps->add_method_formal_list(class_name, method_name, param_list);
            
            // 复制每个形参
            for (auto param_name : param_list) {
              Formal* formal = name_maps->get_method_formal(parent_name, method_name, param_name);
              if (formal) {
                name_maps->add_method_formal(class_name, method_name, param_name, formal);
              }
            }
          }
          
          // 复制方法的返回类型
          Type* ret_type = name_maps->get_method_type(parent_name, method_name);
          if (ret_type) {
            name_maps->add_method_type(class_name, method_name, ret_type);
          }
        }
      }
    }
  }
  
  // 访问id和eid
  if (node->id) {
    node->id->accept(*this);
  }
  if (node->eid) {
    node->eid->accept(*this);
  }
  
  // 注释掉重置 current_class，确保后续方法访问时能获取到所属类信息
  // current_class = "";
}

void AST_Name_Map_Visitor::visit(Type *node) {
  DEBUG_PRINT("Visiting Type");
  CHECK_NULLPTR(node);
  if (node->typeKind == TypeKind::CLASS) {
    if (node->cid) {
      node->cid->accept(*this);
    }
  }
  if (node->typeKind == TypeKind::ARRAY) {
    if (node->arity) {
      node->arity->accept(*this);
    }
  }
}

void AST_Name_Map_Visitor::visit(VarDecl *node) {
  DEBUG_PRINT("Visiting VarDecl");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->id); // 增加对 id 的空指针检查
  
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
  
  if (node->type) {
    node->type->accept(*this);
  }
  if (node->id) {
    node->id->accept(*this);
  }
  if (node->init.index() == 1) {
    auto ie = get<IntExp *>(node->init);
    CHECK_NULLPTR(ie); // 检查通过 variant 获取的 IntExp 指针
    ie->accept(*this);
  } else if (node->init.index() == 2) {
    auto vec = get<vector<IntExp *> *>(node->init);
    if (vec) {
      for (auto e : *vec) {
        CHECK_NULLPTR(e); // 检查每个数组元素指针
        e->accept(*this);
      }
    }
  }
}

void AST_Name_Map_Visitor::visit(MethodDecl *node) {
  DEBUG_PRINT("Visiting MethodDecl");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->id); // 增加对 id 的空指针检查
  
  // 记录方法名
  string method_name = node->id->id;
  current_method = method_name;
  name_maps->add_method(current_class, method_name);
  
  // 准备收集形参列表
  current_formal_list.clear();
  in_formal = true;
  
  if (node->fl) {
    for (auto f : *(node->fl)) {
      CHECK_NULLPTR(f); // 检查形参指针
      f->accept(*this);
    }
  }
  
  // 添加方法返回值作为特殊形参
  string return_var_name = "^_method_return";
  if (node->type) {
    // 存储方法的返回类型
    name_maps->add_method_type(current_class, method_name, node->type->clone());
    
    Formal* return_formal = new Formal(node->getPos(), node->type->clone(), new IdExp(node->getPos(), return_var_name));
    name_maps->add_method_formal(current_class, method_name, return_var_name, return_formal);
    current_formal_list.push_back(return_var_name);
  }
  
  // 添加方法形参列表
  name_maps->add_method_formal_list(current_class, method_name, current_formal_list);
  in_formal = false;
  
  if (node->type) {
    node->type->accept(*this);
  }
  if (node->id) {
    node->id->accept(*this);
  }
  
  if (node->vdl) {
    for (auto vd : *(node->vdl)) {
      CHECK_NULLPTR(vd);
      vd->accept(*this);
    }
  }
  if (node->sl) {
    for (auto s : *(node->sl)) {
      CHECK_NULLPTR(s);
      s->accept(*this);
    }
  }
  
  // 访问完方法后重置当前方法名
  current_method = "";
}

void AST_Name_Map_Visitor::visit(Formal *node) {
  DEBUG_PRINT("Visiting Formal");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->id); // 增加对 id 的空指针检查
  
  // 记录方法形参
  if (!current_class.empty() && !current_method.empty() && in_formal) {
    string var_name = node->id->id;
    name_maps->add_method_formal(current_class, current_method, var_name, node);
    current_formal_list.push_back(var_name);
  }
  
  if (node->type) {
    node->type->accept(*this);
  }
  if (node->id) {
    node->id->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(Nested *node) {
  DEBUG_PRINT("Visiting Nested");
  CHECK_NULLPTR(node);
  if (node->sl) {
    for (auto s : *(node->sl)) {
      CHECK_NULLPTR(s);
      s->accept(*this);
    }
  }
}

void AST_Name_Map_Visitor::visit(If *node) {
  DEBUG_PRINT("Visiting If");
  CHECK_NULLPTR(node);
  if (node->exp) {
    node->exp->accept(*this);
  }
  if (node->stm1) {
    node->stm1->accept(*this);
  }
  if (node->stm2) {
    node->stm2->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(While *node) {
  DEBUG_PRINT("Visiting While");
  CHECK_NULLPTR(node);
  if (node->exp) {
    node->exp->accept(*this);
  }
  if (node->stm) {
    node->stm->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(Assign *node) {
  DEBUG_PRINT("Visiting Assign");
  CHECK_NULLPTR(node);
  if (node->left) {
    node->left->accept(*this);
  }
  if (node->exp) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(CallStm *node) {
  DEBUG_PRINT("Visiting CallStm");
  CHECK_NULLPTR(node);
  if (node->obj) {
    node->obj->accept(*this);
  }
  if (node->name) {
    node->name->accept(*this);
  }
  if (node->par) {
    for (auto p : *(node->par)) {
      CHECK_NULLPTR(p); // 检查每个实参指针
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
  if (node->exp) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(PutInt *node) {
  DEBUG_PRINT("Visiting PutInt");
  CHECK_NULLPTR(node);
  if (node->exp) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(PutCh *node) {
  DEBUG_PRINT("Visiting PutCh");
  CHECK_NULLPTR(node);
  if (node->exp) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(PutArray *node) {
  DEBUG_PRINT("Visiting PutArray");
  CHECK_NULLPTR(node);
  if (node->n) {
    node->n->accept(*this);
  }
  if (node->arr) {
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
  if (node->left) {
    node->left->accept(*this);
  }
  if (node->right) {
    node->right->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(UnaryOp *node) {
  DEBUG_PRINT("Visiting UnaryOp");
  CHECK_NULLPTR(node);
  if (node->exp) {
    node->exp->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(ArrayExp *node) {
  DEBUG_PRINT("Visiting ArrayExp");
  CHECK_NULLPTR(node);
  if (node->arr) {
    node->arr->accept(*this);
  }
  if (node->index) {
    node->index->accept(*this);
  }
}

void AST_Name_Map_Visitor::visit(CallExp *node) {
  DEBUG_PRINT("Visiting CallExp");
  CHECK_NULLPTR(node);
  if (node->obj) {
    node->obj->accept(*this);
  }
  if (node->name) {
    node->name->accept(*this);
  }
  if (node->par) {
    for (auto p : *(node->par)) {
      CHECK_NULLPTR(p); // 检查每个实参指针
      p->accept(*this);
    }
  }
}

void AST_Name_Map_Visitor::visit(ClassVar *node) {
  DEBUG_PRINT("Visiting ClassVar");
  CHECK_NULLPTR(node);
  if (node->obj) {
    node->obj->accept(*this);
  }
  if (node->id) {
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
  if (node->exp) {
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
