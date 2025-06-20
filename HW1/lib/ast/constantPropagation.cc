#define DEBUG
#undef DEBUG

#include "constantPropagation.hh"
#include "ASTheader.hh"
#include "FDMJAST.hh"
#include <iostream>
#include <variant>
#include <vector>

using namespace std;
using namespace fdmj;

#define StmList vector<Stm *>
#define ExpList vector<Exp *>

Program *constantPropagate(Program *root) {
  if (root == nullptr)
    return nullptr;
  ConstantPropagation v(nullptr);
  root->accept(v);
  return dynamic_cast<Program *>(v.newNode);
}

template <typename T>
static vector<T *> *visitList(ConstantPropagation &v, vector<T *> *tl) {
  if (tl == nullptr || tl->size() == 0)
    return nullptr;
  vector<T *> *vt = new vector<T *>();
  for (T *x : *tl) {
    if (x == nullptr)
      continue;
    x->accept(v);
    if (v.newNode == nullptr)
      continue;
    vt->push_back(static_cast<T *>(v.newNode));
  }
  if (vt->size() == 0) {
    delete vt;
    vt = nullptr;
  }
  return vt;
}

void ConstantPropagation::visit(Program *node) {
#ifdef DEBUG
  cerr << "Rewriting Program...\n";
#endif
  if (node == nullptr) {
    newNode = nullptr;
    return;
  }
  // Visit the main method node if there is one
  MainMethod *m;
  if (node->main == nullptr)
    m = nullptr;
  else {
    node->main->accept(*this);
    if (newNode == nullptr)
      m = nullptr;
    else
      m = static_cast<MainMethod *>(
          newNode); // newNode must point to a clone of the MainMethod
  }
  // Visit the class declaration list
  newNode = new Program(node->getPos()->clone(), m); // clone a new Program node
}

void ConstantPropagation::visit(MainMethod *node) {
#ifdef DEBUG
  cerr << "Rewriting MainMethod...\n";
#endif
  // Visit the statement list
  if (node == nullptr) {
    newNode = nullptr;
    return;
  }
  StmList *sl = nullptr;
  if (node->sl != nullptr)
    sl = visitList<Stm>(*this, node->sl);
  newNode = new MainMethod(node->getPos()->clone(), sl);
}

void ConstantPropagation::visit(Assign *node) {
#ifdef DEBUG
  cerr << "Rewriting Assign..., node = " << stringASTKind(node->getASTKind())
       << endl;
#endif
  if (node == nullptr) {
    newNode = nullptr;
    return;
  }
  Exp *l = nullptr;
  if (node->left != nullptr) {
    node->left->accept(*this);
    l = static_cast<Exp *>(newNode);
  } else {
    #ifdef DEBUG
    cerr << "Error: No left expression found in the Assign statement" << endl;
    #endif
    newNode = nullptr;
    return;
  }
  Exp *r = nullptr;
  if (node->exp != nullptr) {
    node->exp->accept(*this);
    r = static_cast<Exp *>(newNode);
  } else {
    #ifdef DEBUG
    cerr << "Error: No right expression found in the Assign statement" << endl;
    #endif
    newNode = nullptr;
    return;
  }
  newNode = new Assign(node->getPos()->clone(), l, r);
}

void ConstantPropagation::visit(Return *node) {
#ifdef DEBUG
  cerr << "Rewriting Return..., node = " << stringASTKind(node->getASTKind())
       << endl;
#endif
  if (node == nullptr) {
    newNode = nullptr;
    return;
  }
  Exp *e = nullptr;
  if (node->exp != nullptr) {
    node->exp->accept(*this);
    e = static_cast<Exp *>(newNode);
  } else {
    #ifdef DEBUG
    cerr << "Error: No expression found in the Return statement" << endl;
    #endif
    newNode = nullptr;
    return;
  }
  newNode = new Return(node->getPos()->clone(), e);
}

void ConstantPropagation::visit(BinaryOp *node) {
#ifdef DEBUG
  cerr << "Rewriting BinaryOp..., node = " << stringASTKind(node->getASTKind())
       << endl;
#endif
  if (node == nullptr) {
    newNode = nullptr;
    return;
  }
  Exp *l = nullptr;
  if (node->left != nullptr) {
    node->left->accept(*this);
    l = static_cast<Exp *>(newNode);
  } else {
    cerr << "Error: No left expression found in the BinaryOp statement" << endl;
    newNode = nullptr;
    return;
  }
  Exp *r = nullptr;
  if (node->right != nullptr) {
    node->right->accept(*this);
    r = static_cast<Exp *>(newNode);
  } else {
    cerr << "Error: No right expression found in the BinaryOp statement"
         << endl;
    newNode = nullptr;
    return;
  }
  if (l->getASTKind() == ASTKind::IntExp && r->getASTKind() == ASTKind::IntExp) {
    int val1 = static_cast<IntExp *>(l)->val;
    int val2 = static_cast<IntExp *>(r)->val;
    int val = 0;
    if      (node->op->op == "+")  val = val1 +  val2;
    else if (node->op->op == "-")  val = val1 -  val2;
    else if (node->op->op == "*")  val = val1 *  val2;
    else if (node->op->op == "/")  val = val1 /  val2;
    else if (node->op->op == "||") val = val1 || val2;
    else if (node->op->op == "&&") val = val1 && val2;
    else if (node->op->op == "<")  val = val1 <  val2;
    else if (node->op->op == "<=") val = val1 <= val2;
    else if (node->op->op == ">")  val = val1 >  val2;
    else if (node->op->op == ">=") val = val1 >= val2;
    else if (node->op->op == "==") val = val1 == val2;
    else if (node->op->op == "!=") val = val1 != val2;
    else {
      #ifdef DEBUG
      cerr << "Error: Unknown operator in the BinaryOp statement" << endl;
      #endif
      newNode = nullptr;
      return;
    }
    // switch (node->op->op) // TODO: 需要把op变成enum
    // {
    // case "+":  val = val1 +  val2; break;
    // case "-":  val = val1 -  val2; break;
    // case "*":  val = val1 *  val2; break;
    // case "/":  val = val1 /  val2; break;
    // case "||": val = val1 || val2; break;
    // case "&&": val = val1 && val2; break;
    // case "<":  val = val1 <  val2; break;
    // case "<=": val = val1 <= val2; break;
    // case ">":  val = val1 >  val2; break;
    // case ">=": val = val1 >= val2; break;
    // case "==": val = val1 == val2; break;
    // case "!=": val = val1 != val2; break;
    // default: cerr << "Error: Unknown operator in the BinaryOp statement" << endl; return;
    // }
    newNode = new IntExp(node->getPos()->clone(), val);
    return;
  }
  newNode = new BinaryOp(node->getPos()->clone(), l, node->op->clone(), r);
}

void ConstantPropagation::visit(UnaryOp *node) {
  Exp *e = nullptr;
  OpExp *op = nullptr;
#ifdef DEBUG
  cerr << "Rewriting UnaryOp..., node = " << stringASTKind(node->getASTKind())
       << endl;
#endif
  if (node == nullptr) {
    newNode = nullptr;
    return;
  }
  if (node->exp != nullptr) {
    node->exp->accept(*this);
    e = static_cast<Exp *>(newNode);
  } else {
    #ifdef DEBUG
    cerr << "Error: No expression found in the UnaryOp statement" << endl;
    #endif
    newNode = nullptr;
    return;
  }
  if (node->op == nullptr) {
    #ifdef DEBUG
    cerr << "Error: No operator found in the UnaryOp statement" << endl;
    #endif
    newNode = nullptr;
    return;
  }
  if (node->op->op == "-" && e->getASTKind() == ASTKind:: IntExp) {
    int val = -(static_cast<IntExp *>(e)->val);
    newNode = new IntExp(node->getPos()->clone(), val);
    return;
  }
  newNode = new UnaryOp(node->getPos()->clone(), node->op->clone(), e);
}

void ConstantPropagation::visit(Esc *node) {
  StmList *sl = nullptr;
  Exp *e = nullptr;
#ifdef DEBUG
  cerr << "Rewriting Esc..., node = " << stringASTKind(node->getASTKind())
       << endl;
#endif
  if (node == nullptr) {
    newNode = nullptr;
    return;
  }
  if (node->sl != nullptr)
    sl = visitList<Stm>(*this, node->sl);
  if (node->exp != nullptr) {
    node->exp->accept(*this);
    e = static_cast<Exp *>(newNode);
  }
  newNode = new Esc(node->getPos()->clone(), sl, e);
}

void ConstantPropagation::visit(IdExp *node) {
#ifdef DEBUG
  cerr << "Rewriting IdExp..., node = " << stringASTKind(node->getASTKind())
       << endl;
#endif
  newNode = (node == nullptr) ? nullptr : static_cast<IdExp *>(node->clone());
}

void ConstantPropagation::visit(OpExp *node) {
#ifdef DEBUG
  cerr << "Rewriting OpExp..., node = " << stringASTKind(node->getASTKind())
       << endl;
#endif
  newNode = (node == nullptr) ? nullptr : static_cast<OpExp *>(node->clone());
}

void ConstantPropagation::visit(IntExp *node) {
#ifdef DEBUG
  cerr << "Rewriting IntExp..., node = " << stringASTKind(node->getASTKind())
       << endl;
#endif
  newNode = (node == nullptr) ? nullptr : static_cast<IntExp *>(node->clone());
}
