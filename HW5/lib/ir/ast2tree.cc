#include "ast2tree.hh"
#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "config.hh"
#include "namemaps.hh"
#include "temp.hh"
#include "treep.hh"
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

#ifdef DEBUG
#define DEBUG_PRINT(msg)                                                       \
  do {                                                                         \
    std::cerr << msg << std::endl;                                             \
  } while (0)
#define DEBUG_PRINT2(msg, val)                                                 \
  do {                                                                         \
    std::cerr << msg << " " << val << std::endl;                               \
  } while (0)
#else
#define DEBUG_PRINT(msg)                                                       \
  do {                                                                         \
  } while (0)
#define DEBUG_PRINT2(msg, val)                                                 \
  do {                                                                         \
  } while (0)
#endif

#define CHECK_NULLPTR(node)                                                    \
  if (!node) {                                                                 \
    std::cerr << "Error: Null pointer at " << __FILE__ << ":" << __LINE__      \
              << std::endl;                                                    \
    exit(1);                                                                   \
  }

typedef variant<monostate, string, int> TypePar;
typedef variant<monostate, fdmj::IntExp *, vector<fdmj::IntExp *> *> VarInit;

AST_Semant_Map *semantMap = new AST_Semant_Map();

//===----------------------------------------------------------------------===//
// 工具函数
//===----------------------------------------------------------------------===//

inline tree::Type get_var_type(fdmj::Type *type) {
  return type && type->typeKind == fdmj::TypeKind::INT ? tree::Type::INT
                                                       : tree::Type::PTR;
}

template <typename ASTNode, typename IRNode>
static vector<IRNode *> *visitList(ASTToTreeVisitor &visitor,
                                   vector<ASTNode *> *nodes) {
  auto *result = new vector<IRNode *>();
  if (!nodes)
    return result;

  for (ASTNode *node : *nodes) {
    if (!node)
      continue;
    node->accept(visitor);
    if (visitor.visit_tree_result) {
      result->push_back(dynamic_cast<IRNode *>(visitor.visit_tree_result));
    }
  }

  return result;
}

static string
find_in_inheritance(const string &className,
                    const function<bool(const string &)> &predicate,
                    Name_Maps *nameMap) {
  vector<string> chain = *nameMap->get_ancestors(className);
  chain.insert(chain.begin(), className);
  for (const auto &c : chain) {
    if (predicate(c))
      return c;
  }
  return "";
}

string ASTToTreeVisitor::get_var_cname(const string &className,
                                       const string &varName,
                                       Name_Maps *nameMap) {
  return find_in_inheritance(
      className,
      [&](const string &c) { return nameMap->is_class_var(c, varName); },
      nameMap);
}

string ASTToTreeVisitor::get_method_cname(const string &className,
                                          const string &methodName,
                                          Name_Maps *nameMap) {
  return find_in_inheritance(
      className,
      [&](const string &c) { return nameMap->is_method(c, methodName); },
      nameMap);
}

//===----------------------------------------------------------------------===//
// 核心函数
//===----------------------------------------------------------------------===//

Class_table *generate_class_table(AST_Semant_Map *semant_map) {
  auto *classTable = new Class_table();
  Name_Maps *name_maps = semant_map->getNameMaps();

  for (const auto &cls : *name_maps->get_class_list()) {
    auto *var_list = name_maps->get_class_var_list(cls);
    if (var_list) {
      for (const auto &var : *var_list)
        classTable->add_var(var);
    }
  }

  for (const auto &cls : *name_maps->get_class_list()) {
    if (cls == "_^main^_")
      continue;
    auto *methods = name_maps->get_method_list(cls);
    if (methods) {
      for (const auto &m : *methods)
        classTable->add_method(m);
    }
  }

  return classTable;
}

Method_var_table *generate_method_var_table(const string &cls,
                                            const string &method,
                                            Name_Maps *nameMaps,
                                            Temp_map *tempMap) {
  auto *table = new Method_var_table(cls, method, tempMap);

  if (cls != "_^main^_") {
    table->add_var("_^this^_", TypeKind::CLASS);
  }

  auto *locals = nameMaps->get_method_var_list(cls, method);
  if (locals) {
    for (const auto &var : *locals) {
      if (auto *decl = nameMaps->get_method_var(cls, method, var)) {
        table->add_var(var, decl->type->typeKind);
      }
    }
  }

  auto *formals = nameMaps->get_method_formal_list(cls, method);
  if (formals) {
    for (const auto &param : *formals) {
      if (auto *formal = nameMaps->get_method_formal(cls, method, param)) {
        table->add_var(param, formal->type->typeKind);
      }
    }
  }

  return table;
}

tree::Program *ast2tree(fdmj::Program *prog, AST_Semant_Map *semant_map) {
  ASTToTreeVisitor visitor(semant_map);
  visitor.visit(prog);
  return dynamic_cast<tree::Program *>(visitor.getTree());
}

// Program
void ASTToTreeVisitor::visit(fdmj::Program *node) {
  DEBUG_PRINT("Visiting Program");
  CHECK_NULLPTR(node);

  vector<tree::FuncDecl *> *functionDeclList = new vector<tree::FuncDecl *>();

  functionDeclList->push_back(translateMainMethod(node));

  translateClassMethods(node, functionDeclList);

  visit_tree_result = new tree::Program(functionDeclList);
  expResult = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::MainMethod *node) {
  DEBUG_PRINT("Visiting MainMethod");
  CHECK_NULLPTR(node);

  string fullMethodName = methodVarTable->cname + "^" + methodVarTable->mname;

  vector<tree::Temp *> *paramList = nullptr;
  vector<tree::Label *> *exitLabels = nullptr;
  tree::Label *entryLabel = nullptr;

  vector<tree::Block *> *methodBody =
      generate_mainmethod_body(node, entryLabel);

  visit_tree_result =
      new tree::FuncDecl(fullMethodName, paramList, methodBody, tree::Type::INT,
                         tempMap->next_temp - 1, tempMap->next_label - 1);

  expResult = nullptr;
}

// ClassDecl
void ASTToTreeVisitor::visit(fdmj::ClassDecl *node) {
  DEBUG_PRINT("Visiting ClassDecl");
  CHECK_NULLPTR(node);
  visit_tree_result = nullptr;
  expResult = nullptr;
}

// Type
void ASTToTreeVisitor::visit(fdmj::Type *node) {
  DEBUG_PRINT("Visiting Type");
  CHECK_NULLPTR(node);
  visit_tree_result = nullptr;
  expResult = nullptr;
}

// VarDecl
void ASTToTreeVisitor::visit(fdmj::VarDecl *node) {
  DEBUG_PRINT("Visiting VarDecl");
  CHECK_NULLPTR(node);
  node->id->accept(*this);
  emitVarDecl(expResult->unEx(tempMap)->exp, node);
}

// MethodDecl
void ASTToTreeVisitor::visit(fdmj::MethodDecl *node) {
  DEBUG_PRINT("Visiting MethodDecl");
  CHECK_NULLPTR(node);

  string className = methodVarTable->cname;
  string methodName = methodVarTable->mname;
  string fullMethodName = className + "^" + methodName;

  vector<tree::Temp *> *paramTempList =
      generate_param_list(className, methodName);

  vector<tree::Label *> *exitLabels = nullptr;
  tree::Label *entryLabel = nullptr;

  vector<tree::Block *> *methodBlocks =
      generate_method_body(node, entryLabel, exitLabels);

  tree::Type returnType = get_return_type(methodName);

  visit_tree_result = new tree::FuncDecl(
      fullMethodName, paramTempList, methodBlocks, returnType,
      tempMap->next_temp - 1, tempMap->next_label - 1);

  expResult = nullptr;
}

// Formal
void ASTToTreeVisitor::visit(fdmj::Formal *node) {
  DEBUG_PRINT("Visiting Formal");
  CHECK_NULLPTR(node);
  visit_tree_result = nullptr;
  expResult = nullptr;
}

// Nested
void ASTToTreeVisitor::visit(fdmj::Nested *node) {
  DEBUG_PRINT("Visiting Nested");
  CHECK_NULLPTR(node);

  auto *stmts = visitList<fdmj::Stm, tree::Stm>(*this, node->sl);

  if (!stmts) {
    stmts = new vector<tree::Stm *>();
  }

  visit_tree_result = new tree::Seq(stmts);
  expResult = nullptr;
}

// If
void ASTToTreeVisitor::visit(fdmj::If *node) {
  DEBUG_PRINT("Visiting If");
  CHECK_NULLPTR(node);

  auto *stmts = new vector<tree::Stm *>();

  node->exp->accept(*this);
  Tr_cx *cond = expResult->unCx(tempMap);
  stmts->push_back(cond->stm);

  node->stm1->accept(*this);
  auto *thenStm = dynamic_cast<tree::Stm *>(visit_tree_result);

  tree::Stm *elseStm = nullptr;
  if (node->stm2) {
    node->stm2->accept(*this);
    elseStm = dynamic_cast<tree::Stm *>(visit_tree_result);
  }

  Label *thenLabel = tempMap->newlabel();
  Label *elseLabel = tempMap->newlabel();
  Label *endLabel = tempMap->newlabel();

  cond->true_list->patch(thenLabel);
  cond->false_list->patch(elseLabel);

  stmts->push_back(new tree::LabelStm(thenLabel));
  stmts->push_back(thenStm);
  stmts->push_back(new tree::Jump(endLabel));

  stmts->push_back(new tree::LabelStm(elseLabel));
  if (elseStm)
    stmts->push_back(elseStm);

  stmts->push_back(new tree::LabelStm(endLabel));

  visit_tree_result = new tree::Seq(stmts);
  expResult = nullptr;
}

// While
void ASTToTreeVisitor::visit(fdmj::While *node) {
  DEBUG_PRINT("Visiting While");
  CHECK_NULLPTR(node);

  auto *stmts = new vector<tree::Stm *>();

  Label *startLabel = tempMap->newlabel();
  Label *bodyLabel = tempMap->newlabel();
  Label *exitLabel = tempMap->newlabel();
  loopLabels.push({startLabel, exitLabel});

  stmts->push_back(new tree::LabelStm(startLabel));

  node->exp->accept(*this);
  Tr_cx *cond = expResult->unCx(tempMap);
  stmts->push_back(cond->stm);

  cond->true_list->patch(bodyLabel);
  stmts->push_back(new tree::LabelStm(bodyLabel));

  node->stm->accept(*this);
  stmts->push_back(dynamic_cast<tree::Stm *>(visit_tree_result));

  stmts->push_back(new tree::Jump(startLabel));

  cond->false_list->patch(exitLabel);
  stmts->push_back(new tree::LabelStm(exitLabel));

  loopLabels.pop();

  visit_tree_result = new tree::Seq(stmts);
  expResult = nullptr;
}

// Assign
void ASTToTreeVisitor::visit(fdmj::Assign *node) {
  DEBUG_PRINT("Visiting Assign");
  CHECK_NULLPTR(node);

  node->left->accept(*this);
  tree::Exp *lhs = expResult->unEx(tempMap)->exp;

  node->exp->accept(*this);
  tree::Exp *rhs = expResult->unEx(tempMap)->exp;

  visit_tree_result = new tree::Move(lhs, rhs);
  expResult = nullptr;
}

// CallStm
void ASTToTreeVisitor::visit(fdmj::CallStm *node) {
  DEBUG_PRINT("Visiting CallStm");
  CHECK_NULLPTR(node);

  buildMethodCall(node->obj, node->name, node->par);

  visit_tree_result = new tree::ExpStm(expResult->unEx(tempMap)->exp);
  expResult = nullptr;
}

// Continue
void ASTToTreeVisitor::visit(fdmj::Continue *node) {
  DEBUG_PRINT("Visiting Continue");
  CHECK_NULLPTR(node);

  auto loopStartLabel = loopLabels.top().first;
  visit_tree_result = new tree::Jump(loopStartLabel);

  expResult = nullptr;
}

// Break
void ASTToTreeVisitor::visit(fdmj::Break *node) {
  DEBUG_PRINT("Visiting Break");
  CHECK_NULLPTR(node);

  auto loopExitLabel = loopLabels.top().second;
  visit_tree_result = new tree::Jump(loopExitLabel);

  expResult = nullptr;
}

// Return
void ASTToTreeVisitor::visit(fdmj::Return *node) {
  DEBUG_PRINT("Visiting Return");
  CHECK_NULLPTR(node);

  node->exp->accept(*this);
  auto *retExpr = expResult->unEx(tempMap)->exp;

  visit_tree_result = new tree::Return(retExpr);
  expResult = nullptr;
}

// PutInt
void ASTToTreeVisitor::visit(fdmj::PutInt *node) {
  DEBUG_PRINT("Visiting PutInt");
  CHECK_NULLPTR(node);

  node->exp->accept(*this);
  auto *arg = expResult->unEx(tempMap)->exp;

  auto *args = new vector<tree::Exp *>({arg});
  auto *call = new tree::ExtCall(tree::Type::INT, "putint", args);

  visit_tree_result = new tree::ExpStm(call);
  expResult = nullptr;
}

// PutCh
void ASTToTreeVisitor::visit(fdmj::PutCh *node) {
  DEBUG_PRINT("Visiting PutCh");
  CHECK_NULLPTR(node);

  node->exp->accept(*this);
  auto *arg = expResult->unEx(tempMap)->exp;

  auto *args = new vector<tree::Exp *>({arg});
  auto *call = new tree::ExtCall(tree::Type::INT, "putch", args);

  visit_tree_result = new tree::ExpStm(call);
  expResult = nullptr;
}

// PutArray
void ASTToTreeVisitor::visit(fdmj::PutArray *node) {
  DEBUG_PRINT("Visiting PutArray");
  CHECK_NULLPTR(node);

  node->n->accept(*this);
  tree::Exp *count = expResult->unEx(tempMap)->exp;

  node->arr->accept(*this);
  tree::Exp *array = expResult->unEx(tempMap)->exp;

  auto *args = new vector<tree::Exp *>({count, array});
  auto *call = new tree::ExtCall(tree::Type::INT, "putarray", args);

  visit_tree_result = new tree::ExpStm(call);
  expResult = nullptr;
}

// Starttime
void ASTToTreeVisitor::visit(fdmj::Starttime *node) {
  DEBUG_PRINT("Visiting Starttime");
  CHECK_NULLPTR(node);

  auto *call = new tree::ExtCall(tree::Type::INT, "starttime",
                                 new vector<tree::Exp *>());
  visit_tree_result = new tree::ExpStm(call);
  expResult = nullptr;
}

// Stoptime
void ASTToTreeVisitor::visit(fdmj::Stoptime *node) {
  DEBUG_PRINT("Visiting Stoptime");
  CHECK_NULLPTR(node);

  auto *call =
      new tree::ExtCall(tree::Type::INT, "stoptime", new vector<tree::Exp *>());
  visit_tree_result = new tree::ExpStm(call);
  expResult = nullptr;
}

// BinaryOp
void ASTToTreeVisitor::visit(fdmj::BinaryOp *node) {
  DEBUG_PRINT("Visiting BinaryOp");
  CHECK_NULLPTR(node);

  const string &op = node->op->op;

  static const set<std::string> arithOps{"+", "-", "*", "/"};
  static const set<std::string> logicOps{"&&", "||"};
  static const set<std::string> compOps{">", "<", "==", ">=", "<=", "!="};

  if (arithOps.count(op)) {
    node->left->accept(*this);
    auto *leftOperand = expResult->unEx(tempMap)->exp;
    node->right->accept(*this);
    auto *rightOperand = expResult->unEx(tempMap)->exp;

    if (leftOperand->type == tree::Type::PTR)
      expResult = new Tr_ex(
          createArrayBinaryOp(leftOperand, rightOperand, op, tempMap));
    else
      expResult = new Tr_ex(
          new tree::Binop(tree::Type::INT, op, leftOperand, rightOperand));
  } else if (logicOps.count(op)) {
    Patch_list *trueList = new Patch_list();
    Patch_list *falseList = new Patch_list();
    auto *stmts = new vector<tree::Stm *>();

    node->left->accept(*this);
    auto *leftCondition = expResult->unCx(tempMap);
    stmts->push_back(leftCondition->stm);

    auto mid = tempMap->newlabel();
    if (op == "||") {
      trueList->add(leftCondition->true_list);
      leftCondition->false_list->patch(mid);
    } else {
      falseList->add(leftCondition->false_list);
      leftCondition->true_list->patch(mid);
    }
    stmts->push_back(new tree::LabelStm(mid));

    node->right->accept(*this);
    auto *rightCondition = expResult->unCx(tempMap);
    stmts->push_back(rightCondition->stm);
    trueList->add(rightCondition->true_list);
    falseList->add(rightCondition->false_list);

    expResult = new Tr_cx(trueList, falseList, new tree::Seq(stmts));
  } else if (compOps.count(op)) {
    Patch_list *trueList = new Patch_list();
    Patch_list *falseList = new Patch_list();
    auto trueLabel = tempMap->newlabel();
    auto falseLabel = tempMap->newlabel();

    trueList->add_patch(trueLabel);
    falseList->add_patch(falseLabel);

    node->left->accept(*this);
    auto *leftExp = expResult->unEx(tempMap);
    node->right->accept(*this);
    auto *rightExpResult = expResult->unEx(tempMap);

    expResult = new Tr_cx(trueList, falseList,
                          new tree::Cjump(op, leftExp->exp, rightExpResult->exp,
                                          trueLabel, falseLabel));
  } else {
    cerr << "cerr: wrong operation in BinaryOp\n";
  }

  visit_tree_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::UnaryOp *node) {
  DEBUG_PRINT("Visiting UnaryOp");
  CHECK_NULLPTR(node);

  node->exp->accept(*this);
  auto *operand = expResult->unEx(tempMap)->exp;
  const string op = node->op->op;

  if (op == "-") {
    if (operand->type == tree::Type::INT) {
      expResult = new Tr_ex(
          new tree::Binop(tree::Type::INT, op, new tree::Const(0), operand));
    } else {
      expResult = new Tr_ex(UnOp_Array(operand, op, tempMap));
    }
  } else if (op == "!") {
    auto *cond = expResult->unCx(tempMap);
    expResult = new Tr_cx(cond->false_list, cond->true_list, cond->stm);
  }

  visit_tree_result = nullptr;
}

// ArrayExp
void ASTToTreeVisitor::visit(fdmj::ArrayExp *node) {
  DEBUG_PRINT("Visiting ArrayExp");
  CHECK_NULLPTR(node);

  node->index->accept(*this);
  auto rawIdx = expResult->unEx(tempMap)->exp;
  vector<tree::Stm *> *preStmts = new vector<tree::Stm *>();
  bool wrap = false;
  auto idx =
      materializeIfNeeded(rawIdx, wrap, preStmts, tempMap, tree::Type::INT);

  node->arr->accept(*this);
  auto rawArr = expResult->unEx(tempMap)->exp;
  auto arr =
      materializeIfNeeded(rawArr, wrap, preStmts, tempMap, tree::Type::PTR);

  auto safeIdx = buildBoundCheck(idx, arr, tempMap);

  auto adj = new tree::Binop(tree::Type::INT, "+", safeIdx, new tree::Const(1));
  auto offset = new tree::Binop(tree::Type::INT, "*", adj, new tree::Const(4));
  auto addr = new tree::Binop(tree::Type::PTR, "+", arr, offset);

  auto mem = new tree::Mem(tree::Type::INT, addr);
  if (wrap) {
    visit_tree_result = new tree::Seq(preStmts);
    expResult = new Tr_ex(
        new tree::Eseq(tree::Type::INT, new tree::Seq(preStmts), mem));
  } else {
    expResult = new Tr_ex(mem);
  }
  visit_tree_result = nullptr;
}

// CallExp
void ASTToTreeVisitor::visit(fdmj::CallExp *callExpr) {
  DEBUG_PRINT("Visiting CallExp");
  CHECK_NULLPTR(callExpr);

  generate_call_expr(callExpr->obj, callExpr->name, callExpr->par);
}

void ASTToTreeVisitor::visit(fdmj::ClassVar *node) {
  DEBUG_PRINT("Visiting ClassVar");
  CHECK_NULLPTR(node);

  node->obj->accept(*this);
  auto *objAddr = expResult->unEx(tempMap)->exp;

  auto *semantics = semantMap->getSemant(node->obj);
  CHECK_NULLPTR(semantics);
  TypePar typePar = semantics->get_type_par();
  string clsName = *get_if<string>(&typePar);

  string member = node->id->id;
  int offset = classTable->get_var_pos(member);
  auto *memberAddr =
      new tree::Binop(tree::Type::PTR, "+", objAddr, new tree::Const(offset));

  auto *nameMaps = semantMap->getNameMaps();
  string owner = get_var_cname(clsName, member, nameMaps);
  auto *decl = nameMaps->get_class_var(owner, member);

  expResult = new Tr_ex(new tree::Mem(get_var_type(decl->type), memberAddr));
  visit_tree_result = nullptr;
}

// BoolExp
void ASTToTreeVisitor::visit(fdmj::BoolExp *node) {
  DEBUG_PRINT("Visiting BoolExp");
  CHECK_NULLPTR(node);

  auto value = node->val ? 1 : 0;
  expResult = new Tr_ex(new tree::Const(value));

  visit_tree_result = nullptr;
}

// This
void ASTToTreeVisitor::visit(fdmj::This *node) {
  DEBUG_PRINT("Visiting This");
  CHECK_NULLPTR(node);

  if (!methodVarTable) {
    cerr << "cerr: invalid visitor environment for `this`" << endl;
    auto *fallback = new tree::TempExp(tree::Type::PTR, tempMap->newtemp());
    expResult = new Tr_ex(fallback);
  } else {
    auto *thisTemp = methodVarTable->get_var_temp("_^this^_");
    expResult = new Tr_ex(new tree::TempExp(tree::Type::PTR, thisTemp));
  }
  visit_tree_result = nullptr;
}

// Length
void ASTToTreeVisitor::visit(fdmj::Length *node) {
  DEBUG_PRINT("Visiting Length");
  CHECK_NULLPTR(node);

  node->exp->accept(*this);
  tree::Exp *arrayExp = expResult->unEx(tempMap)->exp;

  tree::Temp *lenTemp = tempMap->newtemp();
  tree::Exp *lengthRead = new tree::Mem(tree::Type::INT, arrayExp);
  tree::Exp *lengthTempExp = new tree::TempExp(tree::Type::INT, lenTemp);
  tree::Stm *assignLength = new tree::Move(lengthTempExp, lengthRead);

  expResult =
      new Tr_ex(new tree::Eseq(tree::Type::INT, assignLength, lengthTempExp));
  visit_tree_result = nullptr;
}

// Esc
void ASTToTreeVisitor::visit(fdmj::Esc *node) {
  DEBUG_PRINT("Visiting Esc");
  CHECK_NULLPTR(node);

  auto *stmts = new vector<tree::Stm *>();

  if (node->sl) {
    for (auto *stmt : *node->sl) {
      stmt->accept(*this);
      if (visit_tree_result)
        stmts->push_back(static_cast<tree::Stm *>(visit_tree_result));
    }
  }

  tree::Exp *result = nullptr;
  if (node->exp) {
    node->exp->accept(*this);
    result = expResult->unEx(tempMap)->exp;
  }

  expResult =
      new Tr_ex(new tree::Eseq(tree::Type::INT, new tree::Seq(stmts), result));
  visit_tree_result = nullptr;
}

// GetInt
void ASTToTreeVisitor::visit(fdmj::GetInt *node) {
  DEBUG_PRINT("Visiting GetInt");
  if (!node) {
    expResult = nullptr;
    visit_tree_result = nullptr;
    return;
  }
  emitSimpleInput("getint");
}

// GetCh
void ASTToTreeVisitor::visit(fdmj::GetCh *node) {
  DEBUG_PRINT("Visiting GetCh");
  if (!node) {
    expResult = nullptr;
    visit_tree_result = nullptr;
    return;
  }
  emitSimpleInput("getch");
}

// GetArray
void ASTToTreeVisitor::visit(fdmj::GetArray *node) {
  DEBUG_PRINT("Visiting GetArray");
  if (!node) {
    expResult = nullptr;
    visit_tree_result = nullptr;
    return;
  }

  node->exp->accept(*this);
  if (!expResult) {
    cerr << "Warning: Null array expression in GetArray" << endl;
    expResult = new Tr_ex(new tree::Const(0));
    visit_tree_result = nullptr;
    return;
  }

  vector<tree::Exp *> *args = new vector<tree::Exp *>();
  args->push_back(expResult->unEx(tempMap)->exp);
  expResult = new Tr_ex(new tree::ExtCall(tree::Type::INT, "getarray", args));
  visit_tree_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::IdExp *node) {
  DEBUG_PRINT2("Visiting IdExp: ", node ? node->id : "nullptr");

  if (!node || !methodVarTable) {
    handle_invalid_id(node);
    return;
  }

  const string &varName = node->id;
  tree::Temp *temp = nullptr;
  tree::Type type = tree::Type::INT;

  if (resolve_variable(node, varName, temp, type)) {
    expResult = new Tr_ex(new tree::TempExp(type, temp));
  } else {
    expResult =
        new Tr_ex(new tree::TempExp(tree::Type::INT, tempMap->newtemp()));
  }

  visit_tree_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::OpExp *node) {
  DEBUG_PRINT("Visiting OpExp");
  resetResults();
}

// IntExp
void ASTToTreeVisitor::visit(fdmj::IntExp *node) {
  DEBUG_PRINT("Visiting IntExp");

  expResult = new Tr_ex(new tree::Const(node->val));
  visit_tree_result = nullptr;
}

//===----------------------------------------------------------------------===//
// 辅助函数
//===----------------------------------------------------------------------===//

tree::FuncDecl *ASTToTreeVisitor::translateMainMethod(fdmj::Program *node) {
  initializeMethodContext("_^main^_", "main");
  node->main->accept(*this);
  return dynamic_cast<tree::FuncDecl *>(visit_tree_result);
}

void ASTToTreeVisitor::translateClassMethods(
    fdmj::Program *node, vector<tree::FuncDecl *> *functionDeclList) {
  if (!node->cdl)
    return;

  for (ClassDecl *classDecl : *node->cdl) {
    string className = classDecl->id->id;
    for (MethodDecl *methodDecl : *classDecl->mdl) {
      string methodName = methodDecl->id->id;
      initializeMethodContext(className, methodName);
      methodDecl->accept(*this);
      functionDeclList->push_back(
          dynamic_cast<tree::FuncDecl *>(visit_tree_result));
    }
  }
}

vector<tree::Block *> *
ASTToTreeVisitor::generate_mainmethod_body(fdmj::MainMethod *node,
                                           tree::Label *&entryLabel) {
  vector<tree::Stm *> *stmts = generate_local_var_decls();

  vector<tree::Stm *> *bodyStmts =
      visitList<fdmj::Stm, tree::Stm>(*this, node->sl);
  if (bodyStmts && !bodyStmts->empty()) {
    stmts->insert(stmts->end(), bodyStmts->begin(), bodyStmts->end());
  }

  entryLabel = tempMap->newlabel();
  stmts->insert(stmts->begin(), new tree::LabelStm(entryLabel));

  vector<tree::Block *> *blocks = new vector<tree::Block *>();
  blocks->push_back(new tree::Block(entryLabel, nullptr, stmts));
  return blocks;
}

vector<tree::Stm *> *ASTToTreeVisitor::generate_local_var_decls() {
  vector<tree::Stm *> *stmts = new vector<tree::Stm *>();

  Name_Maps *nameMaps = semantMap->getNameMaps();
  string cls = methodVarTable->cname;
  string mtd = methodVarTable->mname;

  set<string> *vars = nameMaps->get_method_var_list(cls, mtd);
  for (const string &name : *vars) {
    fdmj::VarDecl *decl = nameMaps->get_method_var(cls, mtd, name);
    decl->accept(*this);

    if (!visit_tree_result)
      continue;

    if (auto *seq = dynamic_cast<tree::Seq *>(visit_tree_result)) {
      for (tree::Stm *s : *seq->sl) {
        stmts->push_back(s);
      }
    } else {
      stmts->push_back(dynamic_cast<tree::Stm *>(visit_tree_result));
    }
  }

  return stmts;
}

void ASTToTreeVisitor::emitVarDecl(tree::Exp *destExpr, fdmj::VarDecl *decl) {
  switch (decl->type->typeKind) {
  case fdmj::TypeKind::INT:
    handle_int_decl(destExpr, decl);
    break;
  case fdmj::TypeKind::ARRAY:
    handle_array_decl(destExpr, decl);
    break;
  case fdmj::TypeKind::CLASS:
    handle_class_decl(destExpr, decl);
    break;
  default:
    visit_tree_result = nullptr;
  }
  expResult = nullptr;
}

void ASTToTreeVisitor::handle_int_decl(tree::Exp *target, fdmj::VarDecl *decl) {
  if (auto **init = get_if<IntExp *>(&decl->init)) {
    (*init)->accept(*this);
    visit_tree_result = new tree::Move(target, expResult->unEx(tempMap)->exp);
  } else {
    visit_tree_result = nullptr;
  }
}

void ASTToTreeVisitor::handle_array_decl(tree::Exp *target,
                                         fdmj::VarDecl *decl) {
  vector<tree::Stm *> *stmts = new vector<tree::Stm *>();
  int len = decl->type->arity->val;
  if (auto **inits = get_if<vector<IntExp *> *>(&decl->init); inits && *inits) {
    len = (*inits)->size();
  }
  stmts->push_back(new tree::Move(
      target, new tree::ExtCall(
                  tree::Type::PTR, "malloc",
                  new vector<tree::Exp *>(1, new tree::Const((len + 1) * 4)))));
  stmts->push_back(new tree::Move(new tree::Mem(tree::Type::INT, target),
                                  new tree::Const(len)));
  if (auto **inits = get_if<vector<IntExp *> *>(&decl->init); inits && *inits) {
    int idx = 0;
    for (auto *e : **inits) {
      tree::Binop *addr = new tree::Binop(tree::Type::PTR, "+", target,
                                          new tree::Const((idx + 1) * 4));
      stmts->push_back(new tree::Move(new tree::Mem(tree::Type::INT, addr),
                                      new tree::Const(e->val)));
      ++idx;
    }
  }
  visit_tree_result = new tree::Seq(stmts);
}

void ASTToTreeVisitor::handle_class_decl(tree::Exp *target,
                                         fdmj::VarDecl *decl) {
  vector<tree::Stm *> *stmts = new vector<tree::Stm *>();
  stmts->push_back(new tree::Move(
      target,
      new tree::ExtCall(tree::Type::PTR, "malloc",
                        new vector<tree::Exp *>(
                            1, new tree::Const(classTable->class_size())))));
  string cls = decl->type->cid->id;
  Name_Maps *nameMaps = semantMap->getNameMaps();
  for (auto &[field, offset] : classTable->var_pos_map) {
    auto *fieldDecl =
        nameMaps->get_class_var(get_var_cname(cls, field, nameMaps), field);
    if (fieldDecl->type->typeKind == fdmj::TypeKind::CLASS)
      continue;
    tree::Binop *addr =
        new tree::Binop(tree::Type::PTR, "+", target, new tree::Const(offset));
    auto *tempExpr =
        new tree::TempExp(get_var_type(fieldDecl->type), tempMap->newtemp());
    emitVarDecl(tempExpr, fieldDecl);
    if (!visit_tree_result)
      continue;
    tree::Seq *seq = dynamic_cast<tree::Seq *>(visit_tree_result);
    tree::Exp *val =
        seq ? new tree::Eseq(tree::Type::PTR, seq, tempExpr)
            : new tree::Eseq(tree::Type::PTR,
                             new tree::Seq(new vector<tree::Stm *>{
                                 dynamic_cast<tree::Stm *>(visit_tree_result)}),
                             tempExpr);
    stmts->push_back(new tree::Move(new tree::Mem(tree::Type::PTR, addr), val));
  }
  set<string> done;
  vector<string> chain = *nameMaps->get_ancestors(cls);
  chain.insert(chain.begin(), cls);
  for (auto &c : chain) {
    for (auto &m : *nameMaps->get_method_list(c)) {
      if (!done.insert(m).second)
        continue;
      int offset = classTable->get_method_pos(m);
      tree::Binop *addr = new tree::Binop(tree::Type::PTR, "+", target,
                                          new tree::Const(offset));
      stmts->push_back(
          new tree::Move(new tree::Mem(tree::Type::PTR, addr),
                         new tree::Name(new tree::String_Label(c + "^" + m))));
    }
  }
  visit_tree_result = new tree::Seq(stmts);
}

vector<tree::Temp *> *
ASTToTreeVisitor::generate_param_list(const string &className,
                                      const string &methodName) {
  vector<tree::Temp *> *paramList = new vector<tree::Temp *>();

  paramList->push_back(methodVarTable->get_var_temp("_^this^_"));

  Name_Maps *nameMaps = semantMap->getNameMaps();
  for (const string &param :
       *nameMaps->get_method_formal_list(className, methodName)) {
    if (param != "_^return^_" + methodName) {
      paramList->push_back(methodVarTable->get_var_temp(param));
    }
  }

  return paramList;
}

vector<tree::Block *> *
ASTToTreeVisitor::generate_method_body(fdmj::MethodDecl *node,
                                       tree::Label *&entryLabel,
                                       vector<tree::Label *> *exitLabels) {
  vector<tree::Stm *> *stmts = generate_local_var_decls();
  vector<tree::Stm *> *body = visitList<fdmj::Stm, tree::Stm>(*this, node->sl);

  if (body && !body->empty()) {
    stmts->insert(stmts->end(), body->begin(), body->end());
  }

  entryLabel = tempMap->newlabel();
  stmts->insert(stmts->begin(), new tree::LabelStm(entryLabel));

  vector<tree::Block *> *blocks = new vector<tree::Block *>();
  blocks->push_back(new tree::Block(entryLabel, exitLabels, stmts));
  return blocks;
}

tree::Type ASTToTreeVisitor::get_return_type(const string &methodName) {
  return methodVarTable->get_var_type("_^return^_" + methodName);
}

void ASTToTreeVisitor::buildMethodCall(fdmj::Exp *obj, fdmj::IdExp *name,
                                       vector<fdmj::Exp *> *par) {
  obj->accept(*this);
  tree::Exp *this_ptr = expResult->unEx(tempMap)->exp;

  AST_Semant *semantics = semantMap->getSemant(obj);
  TypePar typePar = semantics->get_type_par();
  string class_name = *get_if<string>(&typePar);
  string method_name = name->id;

  int method_offset = classTable->get_method_pos(method_name);
  tree::Exp *method_addr = new tree::Binop(tree::Type::PTR, "+", this_ptr,
                                           new tree::Const(method_offset));
  tree::Mem *method_ptr = new tree::Mem(tree::Type::PTR, method_addr);

  auto *args = new vector<tree::Exp *>();
  args->push_back(this_ptr);
  if (par) {
    for (auto *p : *par) {
      p->accept(*this);
      args->push_back(expResult->unEx(tempMap)->exp);
    }
  }

  Name_Maps *nameMaps = semantMap->getNameMaps();
  string def_cls = get_method_cname(class_name, method_name, nameMaps);
  Formal *ret_formal = nameMaps->get_method_formal(def_cls, method_name,
                                                   "_^return^_" + method_name);
  tree::Type ret_type = get_var_type(ret_formal->type);

  auto *call = new tree::Call(ret_type, method_name, method_ptr, args);
  expResult = new Tr_ex(call);
}

static void appendLengthCheck(vector<tree::Stm *> *stmts, tree::Exp *leftLen,
                              tree::Exp *rightLen, Temp_map *temp_map) {
  auto errorLbl = temp_map->newlabel();
  auto contLbl = temp_map->newlabel();
  stmts->push_back(new tree::Cjump("!=", leftLen, rightLen, errorLbl, contLbl));
  stmts->push_back(new tree::LabelStm(errorLbl));
  stmts->push_back(new tree::ExpStm(new tree::ExtCall(
      tree::Type::INT, "exit", new vector<tree::Exp *>{new tree::Const(-1)})));
  stmts->push_back(new tree::LabelStm(contLbl));
}

tree::Eseq *createArrayBinaryOp(tree::Exp *leftArr, tree::Exp *rightArr,
                                const string &op, Temp_map *temp_map) {
  auto *stmts = new vector<tree::Stm *>();

  auto *leftLength = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
  auto *rightArrayLength =
      new tree::TempExp(tree::Type::INT, temp_map->newtemp());
  stmts->push_back(
      new tree::Move(leftLength, new tree::Mem(tree::Type::INT, leftArr)));
  stmts->push_back(new tree::Move(rightArrayLength,
                                  new tree::Mem(tree::Type::INT, rightArr)));

  appendLengthCheck(stmts, leftLength, rightArrayLength, temp_map);

  auto *count =
      new tree::Binop(tree::Type::INT, "+", leftLength, new tree::Const(1));
  auto *bytes =
      new tree::Binop(tree::Type::INT, "*", count, new tree::Const(4));
  auto *resArr = new tree::TempExp(tree::Type::PTR, temp_map->newtemp());
  stmts->push_back(new tree::Move(
      resArr, new tree::ExtCall(tree::Type::PTR, "malloc",
                                new vector<tree::Exp *>{bytes})));
  stmts->push_back(
      new tree::Move(new tree::Mem(tree::Type::PTR, resArr), leftLength));

  auto *offset = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
  auto *endOff = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
  stmts->push_back(new tree::Move(offset, new tree::Const(4)));
  stmts->push_back(new tree::Move(endOff, bytes));

  auto lblStart = temp_map->newlabel();
  auto lblBody = temp_map->newlabel();
  auto lblExit = temp_map->newlabel();
  stmts->push_back(new tree::LabelStm(lblStart));
  stmts->push_back(new tree::Cjump("<", offset, endOff, lblBody, lblExit));
  stmts->push_back(new tree::LabelStm(lblBody));

  auto *lElem = new tree::Mem(
      tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", leftArr, offset));
  auto *rElem = new tree::Mem(
      tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", rightArr, offset));
  auto *dst = new tree::Mem(
      tree::Type::PTR, new tree::Binop(tree::Type::PTR, "+", resArr, offset));
  stmts->push_back(
      new tree::Move(dst, new tree::Binop(tree::Type::INT, op, lElem, rElem)));

  stmts->push_back(
      new tree::Move(offset, new tree::Binop(tree::Type::INT, "+", offset,
                                             new tree::Const(4))));
  stmts->push_back(new tree::Jump(lblStart));
  stmts->push_back(new tree::LabelStm(lblExit));

  auto *seq = new tree::Seq(stmts);
  return new tree::Eseq(tree::Type::PTR, seq, resArr);
}

static tree::TempExp *allocateArray(tree::Exp *lengthExpr, Temp_map *temp_map,
                                    vector<tree::Stm *> *stmts) {
  auto *count =
      new tree::Binop(tree::Type::INT, "+", lengthExpr, new tree::Const(1));
  auto *bytes =
      new tree::Binop(tree::Type::INT, "*", count, new tree::Const(4));

  auto *call = new tree::ExtCall(tree::Type::PTR, "malloc",
                                 new vector<tree::Exp *>{bytes});
  auto *array = new tree::TempExp(tree::Type::PTR, temp_map->newtemp());
  stmts->push_back(new tree::Move(array, call));
  stmts->push_back(
      new tree::Move(new tree::Mem(tree::Type::PTR, array), lengthExpr));
  return array;
}

static void appendArrayLoop(vector<tree::Stm *> *stmts, tree::Exp *src,
                            tree::TempExp *dst, tree::Exp *len,
                            const string &op, Temp_map *temp_map) {
  auto *offset = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
  auto *endOff = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
  auto *bytes = new tree::Binop(
      tree::Type::INT, "*",
      new tree::Binop(tree::Type::INT, "+", len, new tree::Const(1)),
      new tree::Const(4));

  stmts->push_back(new tree::Move(offset, new tree::Const(4)));
  stmts->push_back(new tree::Move(endOff, bytes));

  auto lbl0 = temp_map->newlabel(), lbl1 = temp_map->newlabel(),
       lbl2 = temp_map->newlabel();
  stmts->push_back(new tree::LabelStm(lbl0));
  stmts->push_back(new tree::Cjump("<", offset, endOff, lbl1, lbl2));
  stmts->push_back(new tree::LabelStm(lbl1));

  auto *leftExp = new tree::Mem(
      tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", src, offset));
  auto *rightExpResult = new tree::Mem(
      tree::Type::PTR, new tree::Binop(tree::Type::PTR, "+", dst, offset));
  auto *opExpr =
      new tree::Binop(tree::Type::INT, op, new tree::Const(0), leftExp);
  stmts->push_back(new tree::Move(rightExpResult, opExpr));

  stmts->push_back(
      new tree::Move(offset, new tree::Binop(tree::Type::INT, "+", offset,
                                             new tree::Const(4))));
  stmts->push_back(new tree::Jump(lbl0));
  stmts->push_back(new tree::LabelStm(lbl2));
}

tree::Eseq *UnOp_Array(tree::Exp *srcArray, const string &opType,
                       Temp_map *temp_map) {
  auto *lenTmp = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
  auto *fetchLen = new tree::Mem(tree::Type::INT, srcArray);
  auto *seqStmts = new vector<tree::Stm *>{new tree::Move(lenTmp, fetchLen)};

  auto *newArr = allocateArray(lenTmp, temp_map, seqStmts);
  appendArrayLoop(seqStmts, srcArray, newArr, lenTmp, opType, temp_map);

  auto *seq = new tree::Seq(seqStmts);
  return new tree::Eseq(tree::Type::PTR, seq, newArr);
}

static tree::Exp *materializeIfNeeded(tree::Exp *expr, bool &needsWrap,
                                      vector<tree::Stm *> *stmts,
                                      Temp_map *temp_map, tree::Type ty) {
  if (dynamic_cast<tree::Eseq *>(expr) || dynamic_cast<tree::Call *>(expr)) {
    auto tmp = new tree::TempExp(ty, temp_map->newtemp());
    stmts->push_back(new tree::Move(tmp, expr));
    needsWrap = true;
    return tmp;
  }
  return expr;
}

static tree::Exp *buildBoundCheck(tree::Exp *idx, tree::Exp *arrAddr,
                                  Temp_map *temp_map) {
  auto stmts = new vector<tree::Stm *>();
  auto lenTmp = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
  stmts->push_back(
      new tree::Move(lenTmp, new tree::Mem(tree::Type::INT, arrAddr)));

  auto errLbl = temp_map->newlabel();
  auto okLbl = temp_map->newlabel();
  stmts->push_back(new tree::Cjump(">=", idx, lenTmp, errLbl, okLbl));

  stmts->push_back(new tree::LabelStm(errLbl));
  stmts->push_back(new tree::ExpStm(new tree::ExtCall(
      tree::Type::INT, "exit", new vector<tree::Exp *>{new tree::Const(-1)})));
  stmts->push_back(new tree::LabelStm(okLbl));

  return new tree::Eseq(tree::Type::INT, new tree::Seq(stmts), idx);
}

void ASTToTreeVisitor::generate_call_expr(fdmj::Exp *object,
                                          fdmj::IdExp *method,
                                          const vector<fdmj::Exp *> *args) {
  object->accept(*this);
  auto *thisPtr = expResult->unEx(tempMap)->exp;

  Name_Maps *nameMaps = semantMap->getNameMaps();
  AST_Semant *semantics = semantMap->getSemant(object);
  TypePar typePar = semantics->get_type_par();
  string className = *get_if<string>(&typePar);
  string methodName = method->id;

  int offset = classTable->get_method_pos(methodName);
  auto *vptr = new tree::Mem(
      tree::Type::PTR,
      new tree::Binop(tree::Type::PTR, "+", thisPtr, new tree::Const(offset)));

  auto *params = new vector<tree::Exp *>{thisPtr};
  if (args) {
    for (auto *arg : *args) {
      arg->accept(*this);
      params->push_back(expResult->unEx(tempMap)->exp);
    }
  }

  string implClass = get_method_cname(className, methodName, nameMaps);
  Formal *returnFormal = nameMaps->get_method_formal(implClass, methodName,
                                                     "_^return^_" + methodName);
  auto returnType = get_var_type(returnFormal->type);

  expResult = new Tr_ex(new tree::Call(returnType, methodName, vptr, params));
  visit_tree_result = nullptr;
}

void ASTToTreeVisitor::emitSimpleInput(const string &funcName) {
  vector<tree::Exp *> *args = new vector<tree::Exp *>();
  expResult = new Tr_ex(new tree::ExtCall(tree::Type::INT, funcName, args));
  visit_tree_result = nullptr;
}

void ASTToTreeVisitor::handle_invalid_id(fdmj::IdExp *node) {
  cerr << "Error: Invalid IdExp access";
  if (node)
    cerr << " for variable " << node->id;
  cerr << endl;

  tree::Temp *fallback = tempMap->newtemp();
  expResult = new Tr_ex(new tree::TempExp(tree::Type::INT, fallback));
  visit_tree_result = nullptr;
}

bool ASTToTreeVisitor::resolve_variable(fdmj::AST *node, const string &varName,
                                        tree::Temp *&temp, tree::Type &type) {
  try {
    temp = methodVarTable->get_var_temp(varName);
    type = methodVarTable->get_var_type(varName);
    return true;
  } catch (const exception &) {
    cerr << "Warning: Variable " << varName << " not found, creating fallback."
         << endl;
    temp = tempMap->newtemp();
    type = tree::Type::INT;

    AST_Semant *semantics = semantMap->getSemant(node);
    if (semantics) {
      TypeKind tk = semantics->get_type();
      if (tk == TypeKind::ARRAY || tk == TypeKind::CLASS)
        type = tree::Type::PTR;
    }

    return false;
  }
}
