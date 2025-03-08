#define DEBUG
#undef DEBUG

#include "executor.hh"
#include "ASTheader.hh"
#include "FDMJAST.hh"
#include <iostream>
#include <variant>
#include <vector>
#include <string>

using namespace std;
using namespace fdmj;

#define StmList vector<Stm *>
#define ExpList vector<Exp *>

Program *execute(Program *root) {
  if (root == nullptr) return nullptr;
  Executor v(nullptr);
  root->accept(v);
  return dynamic_cast<Program *>(v.newNode);
}

template <typename T>
static vector<T *> *visitList(Executor &v, vector<T *> *tl) {
  if (tl == nullptr || tl->size() == 0) return nullptr;
  vector<T *> *vt = new vector<T *>();
  for (T *x : *tl) {
    if (x) {
      x->accept(v);
      if (v.newNode) vt->push_back(static_cast<T *>(v.newNode));
    }
  }
  if (vt->empty()) {
    delete vt;
    vt = nullptr;
  }
  return vt;
}

void Executor::visit(Program *node) {
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
    if (newNode == nullptr) m = nullptr;
    else
      m = static_cast<MainMethod *>(newNode); // newNode must point to a clone of the MainMethod
  }
  // Visit the class declaration list
  newNode = new Program(node->getPos()->clone(), m); // clone a new Program node
}

void Executor::visit(MainMethod *node) {
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

void Executor::visit(Assign *node) {
#ifdef DEBUG
  cerr << "Rewriting Assign..., node = " << stringASTKind(node->getASTKind()) << endl;
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
    cerr << "Error: No left expression found in the Assign statement" << endl;
    newNode = nullptr;
    return;
  }
  Exp *r = nullptr;
  if (node->exp != nullptr) {
    node->exp->accept(*this);
    r = static_cast<Exp *>(newNode);
  } else {
    cerr << "Error: No right expression found in the Assign statement" << endl;
    newNode = nullptr;
    return;
  }

  int value = evaluateExpression(node->exp);
  IntExp* intExp = new IntExp(node->getPos()->clone(), value);

  if (l->getASTKind() == ASTKind::IdExp)
    symbolTable[static_cast<IdExp *>(l)->id] = value;
  else
    cerr << "Error: Left expression is not an IdExp in the Assign statement" << endl;
  newNode = new Assign(node->getPos()->clone(), l, intExp);
}

void Executor::visit(Return *node) {
#ifdef DEBUG
  cerr << "Rewriting Return..., node = " << stringASTKind(node->getASTKind()) << endl;
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
    cerr << "Error: No expression found in the Return statement" << endl;
    newNode = nullptr;
    return;
  }
  int returnValue = evaluateExpression(node->exp);
#ifdef DEBUG
  cerr << "Return value: " << returnValue << endl;
#endif
  std::cout << returnValue << std::endl;
  newNode = new Return(node->getPos()->clone(), e);
}

void Executor::visit(BinaryOp *node) {
#ifdef DEBUG
  cerr << "Rewriting BinaryOp..., node = " << stringASTKind(node->getASTKind()) << endl;
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
    cerr << "Error: No right expression found in the BinaryOp statement" << endl;
    newNode = nullptr;
    return;
  }

  int val1 = evaluateExpression(node->left);
  int val2 = evaluateExpression(node->right);
  int val = evaluateExpression(node);
  newNode = new IntExp(node->getPos()->clone(), val);
}

void Executor::visit(UnaryOp *node) {
  Exp *e = nullptr;
  OpExp *op = nullptr;
#ifdef DEBUG
  cerr << "Rewriting UnaryOp..., node = " << stringASTKind(node->getASTKind()) << endl;
#endif
  if (node == nullptr) {
    newNode = nullptr;
    return;
  }
  if (node->exp != nullptr) {
    node->exp->accept(*this);
    e = static_cast<Exp *>(newNode);
  } else {
    cerr << "Error: No expression found in the UnaryOp statement" << endl;
    newNode = nullptr;
    return;
  }
  if (node->op == nullptr) {
    cerr << "Error: No operator found in the UnaryOp statement" << endl;
    newNode = nullptr;
    return;
  }
  newNode = new UnaryOp(node->getPos()->clone(), node->op->clone(), e);
}

void Executor::visit(Esc *node) {
  StmList *sl = nullptr;
  Exp *e = nullptr;
#ifdef DEBUG
  cerr << "Rewriting Esc..., node = " << stringASTKind(node->getASTKind()) << endl;
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
  int result = evaluateExpression(e);
  node->exp = new IntExp(node->getPos(), result);
}

void Executor::visit(IdExp *node) {
#ifdef DEBUG
  cerr << "Rewriting IdExp..., node = " << stringASTKind(node->getASTKind()) << endl;
#endif
  newNode = (node == nullptr) ? nullptr : static_cast<IdExp *>(node->clone());
}

void Executor::visit(OpExp *node) {
#ifdef DEBUG
  cerr << "Rewriting OpExp..., node = " << stringASTKind(node->getASTKind()) << endl;
#endif
  newNode = (node == nullptr) ? nullptr : static_cast<OpExp *>(node->clone());
}

void Executor::visit(IntExp *node) {
#ifdef DEBUG
  cerr << "Rewriting IntExp..., node = " << stringASTKind(node->getASTKind()) << endl;
#endif
  newNode = (node == nullptr) ? nullptr : static_cast<IntExp *>(node->clone());
}

int Executor::evaluateExpression(Exp *exp) {
  if (!exp) {
      std::cerr << "Error: Null expression encountered!" << std::endl;
      return 0;
  }

  switch (exp->getASTKind()) {
      case ASTKind::IntExp: {
        return static_cast<IntExp*>(exp)->val;
      }
      case ASTKind::IdExp: {
        IdExp *idExp = static_cast<IdExp*>(exp);
        if (symbolTable.find(idExp->id) != symbolTable.end()) {
          return symbolTable[idExp->id];
        } else {
          std::cerr << "Error: Undefined variable " << idExp->id << std::endl;
          return 0;
        }
      }
      case ASTKind::BinaryOp: {
        BinaryOp *binOp = static_cast<BinaryOp*>(exp);
        int val1 = evaluateExpression(binOp->left);
        int val2 = evaluateExpression(binOp->right);
        string op = static_cast<string>(binOp->op->op);
        if (op == "+")       return val1 + val2;
        else if (op == "-")  return val1 - val2;
        else if (op == "*")  return val1 * val2;
        else if (op == "/") {
          if (!val2) {
            std::cerr << "Error: Division by zero!" << std::endl;
            return 0;
          }
          return val1 / val2;
        }
        else if (op == "||") return val1 || val2;
        else if (op == "&&") return val1 && val2;
        else if (op == "<")  return val1 <  val2;
        else if (op == "<=") return val1 <= val2;
        else if (op == ">")  return val1 >  val2;
        else if (op == ">=") return val1 >= val2;
        else if (op == "==") return val1 == val2;
        else if (op == "!=") return val1 != val2;
        else {
          cerr << "Error: Unknown operator in the BinaryOp statement" << endl;
          newNode = nullptr;
          return 0;
        }
      }
      case ASTKind::Esc: {
        Esc *esc = static_cast<Esc*>(exp);
        visit(esc);
        return static_cast<IntExp*>(esc->exp)->val;
      }
      default:
        cerr << "Type: " << stringASTKind(exp->getASTKind()) << endl;
        cerr << "Error: Unsupported expression type!" << endl;
        return 0;
  }
}
