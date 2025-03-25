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
}

void AST_Name_Map_Visitor::visit(ClassDecl *node) {
#ifdef DEBUG
  std::cout << "Visiting ClassDecl" << std::endl;
#endif
  if (node == nullptr) {
    return;
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
  if (node->type != nullptr) {
    node->type->accept(*this);
  }
  if (node->id != nullptr) {
    node->id->accept(*this);
  }
  if (node->fl != nullptr) {
    for (auto f : *(node->fl)) {
      f->accept(*this);
    }
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
}

void AST_Name_Map_Visitor::visit(Formal *node) {
#ifdef DEBUG
  std::cout << "Visiting Formal" << std::endl;
#endif
  if (node == nullptr) {
    return;
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
