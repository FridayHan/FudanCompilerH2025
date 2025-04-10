#define DEBUG
#undef DEBUG

#include "ast2tree.hh"
#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "config.hh"
#include "temp.hh"
#include "treep.hh"
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;
// using namespace fdmj;
// using namespace tree;

tree::Program *ast2tree(fdmj::Program *prog, AST_Semant_Map *semant_map) {
  DEBUG_PRINT("Converting AST to IR");
  CHECK_NULLPTR(prog);
  CHECK_NULLPTR(semant_map);

  ASTToTreeVisitor *visitor = new ASTToTreeVisitor();
  visitor->semant_map = semant_map;
  prog->accept(*visitor);
  tree::Program *p = static_cast<tree::Program *>(visitor->getTree());
  return p;
}

void ASTToTreeVisitor::visit(fdmj::Program *node) {
  DEBUG_PRINT("Visiting Program");
  CHECK_NULLPTR(node);

  if (node->main) {
    node->main->accept(*this);
  }
  vector<tree::FuncDecl *> *funcdecllist = new vector<tree::FuncDecl *>();
  funcdecllist->push_back(
      static_cast<tree::FuncDecl *>(this->visit_tree_result));
}

void ASTToTreeVisitor::visit(fdmj::MainMethod *node) {
  DEBUG_PRINT("visit: MainMethod node");
  CHECK_NULLPTR(node);

  current_method = "_^main^_^main";
  method_var_table_map[current_method] = Method_var_table();

  Label *entry_label = visitor_temp_map->newlabel();
  vector<tree::Label *> *exit_labels = new vector<tree::Label *>();
  vector<tree::Stm *> *stmts = new vector<tree::Stm *>();
  stmts->push_back(new tree::LabelStm(entry_label));

  if (node->vdl) {
    for (auto v : *node->vdl) {
      CHECK_NULLPTR(v);
      v->accept(*this);
      if (visit_tree_result)
        stmts->push_back(static_cast<tree::Stm *>(visit_tree_result));
    }
  }
  if (node->sl) {
    for (auto s : *node->sl) {
      CHECK_NULLPTR(s);
      s->accept(*this);
      stmts->push_back(static_cast<tree::Stm *>(visit_tree_result));
    }
  }

  vector<tree::Block *> *blocks = new vector<tree::Block *>();
  blocks->push_back(new tree::Block(entry_label, exit_labels, stmts));
  visit_tree_result = new tree::FuncDecl(
      "_^main^_^main", nullptr, blocks, tree::Type::INT,
      visitor_temp_map->next_temp - 1, visitor_temp_map->next_label - 1);
}

void ASTToTreeVisitor::visit(fdmj::ClassDecl *node) {
  DEBUG_PRINT("visit: ClassDecl node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::Type *node) {
  DEBUG_PRINT("visit: Type node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::VarDecl *node) {
  DEBUG_PRINT("visit: VarDecl node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->id);

  node->id->accept(*this);
  Temp *temp = static_cast<TempExp *>(visit_tree_result)->temp;
  if (holds_alternative<IntExp *>(node->init)) {
    visit_tree_result =
        new tree::Move(new tree::TempExp(tree::Type::INT, temp),
                       new tree::Const(get<IntExp *>(node->init)->val));
  } else {
    visit_tree_result = nullptr;
  }
}

void ASTToTreeVisitor::visit(fdmj::MethodDecl *node) {
  DEBUG_PRINT("visit: MethodDecl node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::Formal *node) {
  DEBUG_PRINT("visit: Formal node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::Nested *node) {
  DEBUG_PRINT("visit: Nested node");
  CHECK_NULLPTR(node);

  vector<tree::Stm *> *stmts = new vector<tree::Stm *>();
  if (node->sl) {
    for (auto s : *node->sl) {
      CHECK_NULLPTR(s);
      s->accept(*this);
      stmts->push_back(static_cast<tree::Stm *>(visit_tree_result));
    }
  }
  visit_tree_result = new tree::Seq(stmts);
}

void ASTToTreeVisitor::visit(fdmj::If *node) {
  DEBUG_PRINT("visit: If node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->exp);

  node->exp->accept(*this);
  Tr_cx *cond = currentExp->unCx(visitor_temp_map);

  Label *true_label = visitor_temp_map->newlabel();
  Label *false_label = visitor_temp_map->newlabel();
  Label *end_label = visitor_temp_map->newlabel();

  vector<tree::Stm *> *stmts = new vector<tree::Stm *>();
  stmts->push_back(cond->stm);

  if (!node->stm1 && !node->stm2) {
    cond->true_list->patch(end_label);
    cond->false_list->patch(end_label);
    stmts->push_back(new tree::LabelStm(end_label));
  }

  else if (!node->stm1) {
    Label *else_label = visitor_temp_map->newlabel();
    cond->true_list->patch(end_label);
    cond->false_list->patch(else_label);

    stmts->push_back(new tree::LabelStm(else_label));
    CHECK_NULLPTR(node->stm2);
    node->stm2->accept(*this);
    stmts->push_back(static_cast<tree::Stm *>(visit_tree_result));
    stmts->push_back(new tree::LabelStm(end_label));
  }

  else if (!node->stm2) {
    cond->true_list->patch(true_label);
    cond->false_list->patch(end_label);

    stmts->push_back(new tree::LabelStm(true_label));
    CHECK_NULLPTR(node->stm1);
    node->stm1->accept(*this);
    stmts->push_back(static_cast<tree::Stm *>(visit_tree_result));
    stmts->push_back(new tree::LabelStm(end_label));
  }

  else {
    Label *else_label = visitor_temp_map->newlabel();
    cond->true_list->patch(true_label);
    cond->false_list->patch(else_label);

    stmts->push_back(new tree::LabelStm(true_label));
    CHECK_NULLPTR(node->stm1);
    node->stm1->accept(*this);
    stmts->push_back(static_cast<tree::Stm *>(visit_tree_result));
    stmts->push_back(new tree::Jump(end_label));

    stmts->push_back(new tree::LabelStm(else_label));
    CHECK_NULLPTR(node->stm2);
    node->stm2->accept(*this);
    stmts->push_back(static_cast<tree::Stm *>(visit_tree_result));

    stmts->push_back(new tree::LabelStm(end_label));
  }

  visit_tree_result = new tree::Seq(stmts);
}


void ASTToTreeVisitor::visit(fdmj::While *node) {
  DEBUG_PRINT("visit: While node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->exp);
  CHECK_NULLPTR(node->stm);

  node->exp->accept(*this);
  Tr_ex *exp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
  Tr_cx *cond = exp->unCx(visitor_temp_map);

  Label *test_label = visitor_temp_map->newlabel();
  Label *body_label = visitor_temp_map->newlabel();
  Label *end_label = visitor_temp_map->newlabel();

  while_test = test_label;
  while_end = end_label;

  vector<tree::Stm *> *stmts = new vector<tree::Stm *>();
  stmts->push_back(new tree::LabelStm(test_label));
  cond->true_list->patch(body_label);
  cond->false_list->patch(end_label);
  stmts->push_back(cond->stm);
  stmts->push_back(new tree::LabelStm(body_label));

  node->stm->accept(*this);
  stmts->push_back(static_cast<tree::Stm *>(visit_tree_result));

  stmts->push_back(new tree::Jump(test_label));
  stmts->push_back(new tree::LabelStm(end_label));

  visit_tree_result = new tree::Seq(stmts);
}


void ASTToTreeVisitor::visit(fdmj::Assign *node) {
  DEBUG_PRINT("visit: Assign node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->exp);
  CHECK_NULLPTR(node->left);

  node->exp->accept(*this);
  Tr_ex *exp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
  node->left->accept(*this);
  Temp *temp = static_cast<TempExp *>(visit_tree_result)->temp;
  visit_tree_result = new tree::Move(new tree::TempExp(tree::Type::INT, temp),
                                     exp->unEx(visitor_temp_map)->exp);
}

void ASTToTreeVisitor::visit(fdmj::CallStm *node) {
  DEBUG_PRINT("visit: CallStm node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::Continue *node) {
  DEBUG_PRINT("visit: Continue node");
  CHECK_NULLPTR(node);

  if (!while_test) {
    DEBUG_PRINT("Error: Continue not in loop");
    return;
  }
  visit_tree_result = new tree::Jump(while_test);
}

void ASTToTreeVisitor::visit(fdmj::Break *node) {
  DEBUG_PRINT("visit: Break node");
  CHECK_NULLPTR(node);

  if (!while_end) {
    DEBUG_PRINT("Error: Break not in loop");
    return;
  }
  visit_tree_result = new tree::Jump(while_end);
}

void ASTToTreeVisitor::visit(fdmj::Return *node) {
  DEBUG_PRINT("visit: Return node");
  CHECK_NULLPTR(node);

  if (node->exp) {
    CHECK_NULLPTR(node->exp);
    node->exp->accept(*this);
    Tr_ex *exp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
    visit_tree_result = new tree::Return(exp->unEx(visitor_temp_map)->exp);
  } else {
    visit_tree_result = new tree::Return(nullptr);
  }
}

void ASTToTreeVisitor::visit(fdmj::PutInt *node) {
  DEBUG_PRINT("visit: PutInt node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->exp);

  node->exp->accept(*this);
  Tr_ex *exp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
  vector<tree::Exp *> *args = new vector<tree::Exp *>();
  args->push_back(exp->unEx(visitor_temp_map)->exp);
  visit_tree_result = new tree::ExpStm(
      new tree::ExtCall(tree::Type(tree::Type::INT), "putint", args));
}

void ASTToTreeVisitor::visit(fdmj::PutCh *node) {
  DEBUG_PRINT("visit: PutCh node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->exp);

  node->exp->accept(*this);
  Tr_ex *exp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
  vector<tree::Exp *> *args = new vector<tree::Exp *>();
  args->push_back(exp->unEx(visitor_temp_map)->exp);
  visit_tree_result = new tree::ExpStm(
      new tree::ExtCall(tree::Type(tree::Type::INT), "putch", args));
}

void ASTToTreeVisitor::visit(fdmj::PutArray *node) {
  DEBUG_PRINT("visit: PutArray node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::Starttime *node) {
  DEBUG_PRINT("visit: Starttime node");
  CHECK_NULLPTR(node);

  visit_tree_result = new tree::ExpStm(
      new tree::ExtCall(tree::Type(tree::Type::INT), "starttime", nullptr));
}

void ASTToTreeVisitor::visit(fdmj::Stoptime *node) {
  DEBUG_PRINT("visit: Stoptime node");
  CHECK_NULLPTR(node);

  visit_tree_result = new tree::ExpStm(
      new tree::ExtCall(tree::Type(tree::Type::INT), "stoptime", nullptr));
}

void ASTToTreeVisitor::visit(fdmj::BinaryOp *node) {
  DEBUG_PRINT("visit: BinaryOp node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->op);
  CHECK_NULLPTR(node->left);
  CHECK_NULLPTR(node->right);

  std::string op = node->op->op;

  // 逻辑运算 && ||
  if (op == "&&" || op == "||") {
    node->left->accept(*this);
    Tr_cx *left_cx = currentExp->unCx(visitor_temp_map);

    node->right->accept(*this);
    Tr_cx *right_cx = currentExp->unCx(visitor_temp_map);

    Label *mid_label = visitor_temp_map->newlabel();
    vector<tree::Stm *> *stmts = new vector<tree::Stm *>();
    stmts->push_back(left_cx->stm);
    stmts->push_back(new tree::LabelStm(mid_label));
    stmts->push_back(right_cx->stm);
    tree::Seq *seq = new tree::Seq(stmts);

    Patch_list *true_list = new Patch_list();
    Patch_list *false_list = new Patch_list();

    if (op == "&&") {
      left_cx->true_list->patch(mid_label);
      true_list->add(right_cx->true_list);
      false_list->add(left_cx->false_list);
      false_list->add(right_cx->false_list);
    } else if (op == "||") {
      left_cx->false_list->patch(mid_label);
      true_list->add(left_cx->true_list);
      true_list->add(right_cx->true_list);
      false_list->add(right_cx->false_list);
    } else {
      DEBUG_PRINT("Error: Unknown logical operator");
      return;
    }

    currentExp = new Tr_cx(true_list, false_list, seq);
    visit_tree_result = currentExp->unEx(visitor_temp_map)->exp;
    return;
  }

  // 算术运算 + - * /
  if (op == "+" || op == "-" || op == "*" || op == "/") {
    node->left->accept(*this);
    Tr_ex *left = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
    node->right->accept(*this);
    Tr_ex *right = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));

    tree::Exp *binop = new tree::Binop(tree::Type::INT, op, left->exp, right->exp);
    visit_tree_result = binop;
    currentExp = new Tr_ex(binop);
    return;
  }

  // 比较运算 == != > >= < <=
  if (op == "==" || op == "!=" || op == ">" || op == ">=" || op == "<" || op == "<=") {
    node->left->accept(*this);
    Tr_ex *left = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
    node->right->accept(*this);
    Tr_ex *right = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));

    Label *true_label = (op == "==" || op == "!=" || op == ">") ? visitor_temp_map->newlabel() : nullptr;
    Label *false_label = (op == "==" || op == "!=" || op == ">") ? visitor_temp_map->newlabel() : nullptr;

    tree::Cjump *cj = new tree::Cjump(op, left->exp, right->exp, true_label, false_label);

    Patch_list *true_list = new Patch_list();
    Patch_list *false_list = new Patch_list();
    true_list->add_patch(cj->t);
    false_list->add_patch(cj->f);

    Tr_cx *cx = new Tr_cx(true_list, false_list, cj);
    currentExp = cx;
    visit_tree_result = cx->unEx(visitor_temp_map)->exp;
    return;
  }
}


void ASTToTreeVisitor::visit(fdmj::UnaryOp *node) {
  DEBUG_PRINT("visit: UnaryOp node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->op);
  CHECK_NULLPTR(node->exp);

  node->exp->accept(*this);
  Tr_ex *exp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
  tree::Type t = tree::Type::INT;

  if (node->op->op == "-") {
    visit_tree_result = new tree::Binop(t, "-", exp->exp, new tree::Const(0));
  } else if (node->op->op == "!") {
    visit_tree_result = new tree::Binop(t, "==", exp->exp, new tree::Const(0));
  }
  currentExp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
}

void ASTToTreeVisitor::visit(fdmj::ArrayExp *node) {
  DEBUG_PRINT("visit: ArrayExp node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::CallExp *node) {
  DEBUG_PRINT("visit: CallExp node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::ClassVar *node) {
  DEBUG_PRINT("visit: ClassVar node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::BoolExp *node) {
  DEBUG_PRINT("visit: BoolExp node");
  CHECK_NULLPTR(node);
  if (node->val == true) {
    visit_tree_result = new tree::Const(1);
  } else {
    visit_tree_result = new tree::Const(0);
  }
  currentExp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
}

void ASTToTreeVisitor::visit(fdmj::This *node) {
  DEBUG_PRINT("visit: This node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::Length *node) {
  DEBUG_PRINT("visit: Length node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::Esc *node) {
  DEBUG_PRINT("visit: Esc node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->exp);

  vector<tree::Stm *> *stmts = new vector<tree::Stm *>();
  if (node->sl) {
    for (auto s : *node->sl) {
      CHECK_NULLPTR(s);
      s->accept(*this);
      stmts->push_back(static_cast<tree::Stm *>(visit_tree_result));
    }
  }

  node->exp->accept(*this);
  Tr_ex *exp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));

  visit_tree_result =
      new tree::Eseq(tree::Type::INT, new tree::Seq(stmts), exp->exp);
  currentExp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
}

void ASTToTreeVisitor::visit(fdmj::GetInt *node) {
  DEBUG_PRINT("visit: GetInt node");
  CHECK_NULLPTR(node);

  visit_tree_result = new tree::ExpStm(
      new tree::ExtCall(tree::Type(tree::Type::INT), "getint", nullptr));
  currentExp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
}

void ASTToTreeVisitor::visit(fdmj::GetCh *node) {
  DEBUG_PRINT("visit: GetCh node");
  CHECK_NULLPTR(node);

  visit_tree_result = new tree::ExpStm(
      new tree::ExtCall(tree::Type(tree::Type::INT), "getch", nullptr));
  currentExp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
}

void ASTToTreeVisitor::visit(fdmj::GetArray *node) {
  DEBUG_PRINT("visit: GetArray node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::IdExp *node) {
  DEBUG_PRINT("visit: IdExp node");
  CHECK_NULLPTR(node);

  std::string id = node->id;
  auto *var_temp_map = method_var_table_map[current_method].var_temp_map;

  Temp *temp = nullptr;
  if (var_temp_map->find(id) == var_temp_map->end()) {
    temp = visitor_temp_map->newtemp();
    var_temp_map->insert({id, temp});
  } else {
    temp = method_var_table_map[current_method].get_var_temp(id);
  }

  visit_tree_result = new tree::TempExp(tree::Type::INT, temp);
  currentExp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
}


void ASTToTreeVisitor::visit(fdmj::OpExp *node) {
  DEBUG_PRINT("visit: OpExp node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::IntExp *node) {
  DEBUG_PRINT("visit: IntExp node");
  CHECK_NULLPTR(node);

  visit_tree_result = new tree::Const(node->val);
  currentExp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
}
