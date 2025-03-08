#ifndef _EXECUTOR_H
#define _EXECUTOR_H

#include "ASTheader.hh"
#include "FDMJAST.hh"
#include <unordered_map>

using namespace std;
using namespace fdmj;

Program *execute(Program *root);

class Executor : public ASTVisitor {
private:
  unordered_map<string, int> symbolTable;
  int evaluateExpression(Exp *exp);
public:
  AST *newNode =
      nullptr; // newNode is the root of a clone of the original AST (sub) tree
  Executor(AST *newNode) : newNode(newNode) {}
  void visit(Program *node) override;
  void visit(MainMethod *node) override;
  void visit(Assign *node) override;
  void visit(Return *node) override;
  void visit(BinaryOp *node) override;
  void visit(UnaryOp *node) override;
  void visit(Esc *node) override;
  void visit(IdExp *node) override;
  void visit(OpExp *node) override;
  void visit(IntExp *node) override;
};

#endif