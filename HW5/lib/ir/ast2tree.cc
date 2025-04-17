#define DEBUG
// #undef DEBUG

#include "ast2tree.hh"
#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "config.hh"
#include "temp.hh"
#include "treep.hh"
#include <algorithm>
#include <assert.h>
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
  visit_tree_result = new tree::Program(funcdecllist);
  CHECK_NULLPTR(visit_tree_result);
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
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::ClassDecl *node) {
  DEBUG_PRINT("visit: ClassDecl node");
  CHECK_NULLPTR(node);
}

void ASTToTreeVisitor::visit(fdmj::Type *node) {
  DEBUG_PRINT("visit: Type node");
  CHECK_NULLPTR(node);

  // 该节点仅用于类型信息，不生成任何 IR
  currentExp = nullptr;
  visit_tree_result = nullptr;
}

void ASTToTreeVisitor::visit(fdmj::VarDecl *node) {
  DEBUG_PRINT("visit: VarDecl node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->id);

  string id = node->id->id;
  Temp *temp = visitor_temp_map->newtemp();
  method_var_table_map[current_method].var_temp_map->insert({id, temp});

  tree::Type ir_type;
  switch (node->type->typeKind) {
  case fdmj::TypeKind::INT: {
    ir_type = tree::Type::INT;
    if (holds_alternative<monostate>(node->init)) {
      visit_tree_result = nullptr;
    } else if (holds_alternative<IntExp *>(node->init)) {
      auto int_exp = get<fdmj::IntExp *>(node->init);
      visit_tree_result = new tree::Move(new tree::TempExp(ir_type, temp),
                                         new tree::Const(int_exp->val));
    } else {
      DEBUG_PRINT("VarDecl: Unknown init variant type");
      visit_tree_result = nullptr;
      return;
    }
    break;
  }
  case fdmj::TypeKind::ARRAY: {
    ir_type = tree::Type::PTR;
    vector<tree::Stm *> *stmts = new vector<tree::Stm *>();
    if (holds_alternative<monostate>(node->init)) {
      int len = 0;
      tree::Exp *alloc_size = new tree::Const((len + 1) * 4);
      stmts->push_back(
          new tree::Move(new tree::TempExp(tree::Type::PTR, temp),
                         new tree::Call(tree::Type::PTR, "malloc", nullptr,
                                        new vector<tree::Exp *>{alloc_size})));
      // arr[0] = 0
      stmts->push_back(new tree::Move(
          new tree::Mem(tree::Type::PTR,
                        new tree::TempExp(tree::Type::PTR, temp)),
          new tree::Const(0)));
    } else if (holds_alternative<vector<fdmj::IntExp *> *>(node->init)) {
      auto vec = get<vector<fdmj::IntExp *> *>(node->init);
      if (!vec) {
        int len = 0;
        tree::Exp *alloc_size = new tree::Const((len + 1) * 4);
        stmts->push_back(new tree::Move(
            new tree::TempExp(tree::Type::PTR, temp),
            new tree::Call(tree::Type::PTR, "malloc", nullptr,
                           new vector<tree::Exp *>{alloc_size})));
        // arr[0] = 0
        stmts->push_back(new tree::Move(
            new tree::Mem(tree::Type::PTR,
                          new tree::TempExp(tree::Type::PTR, temp)),
            new tree::Const(0)));
        return;
      } // TODO: should be merged to monostate
      int len = vec->size();

      // 分配空间 (len + 1) * 4
      tree::Exp *alloc_size = new tree::Const((len + 1) * 4);
      stmts->push_back(
          new tree::Move(new tree::TempExp(tree::Type::PTR, temp),
                         new tree::Call(tree::Type::PTR, "malloc", nullptr,
                                        new vector<tree::Exp *>{alloc_size})));

      // 存储长度 arr[0] = len
      stmts->push_back(new tree::Move(
          new tree::Mem(tree::Type::PTR,
                        new tree::TempExp(tree::Type::PTR, temp)),
          new tree::Const(len)));

      // 存储数组元素 arr[i+1] = vec[i]
      for (int i = 0; i < len; ++i) {
        tree::Exp *addr = new tree::Binop(
            tree::Type::PTR, "+", new tree::TempExp(tree::Type::PTR, temp),
            new tree::Const((i + 1) * 4));
        stmts->push_back(new tree::Move(new tree::Mem(tree::Type::INT, addr),
                                        new tree::Const((*vec)[i]->val)));
      }
    } else {
      DEBUG_PRINT("VarDecl(ARRAY): Unknown init variant type");
      visit_tree_result = nullptr;
      return;
    }
    visit_tree_result = new tree::Seq(stmts);
    break;
  }
  default: {
    DEBUG_PRINT("VarDecl: Unknown type");
    visit_tree_result = nullptr;
    return;
  }
  }
  local_var_type_map[id] = ir_type;
}

void ASTToTreeVisitor::visit(fdmj::MethodDecl *node) {
  DEBUG_PRINT("visit: MethodDecl node");
  CHECK_NULLPTR(node);

  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::Formal *node) {
  DEBUG_PRINT("visit: Formal node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->id);

  // 1. 获取变量名（由 IdExp 节点提供）
  string id = node->id->id;

  // 2. 获取当前方法对应的 var->Temp 映射表
  auto *var_temp_map = method_var_table_map[current_method].var_temp_map;
  CHECK_NULLPTR(var_temp_map);

  // 3. 若变量尚未注册，生成新 temp 并注册
  Temp *temp = nullptr;
  if (var_temp_map->find(id) == var_temp_map->end()) {
    temp = visitor_temp_map->newtemp();
    var_temp_map->insert({id, temp});
  } else {
    temp = method_var_table_map[current_method].get_var_temp(id);
  }

  // 4. 不产生 IR，因此置空 currentExp 和 visit_tree_result
  currentExp = nullptr;
  visit_tree_result = nullptr;
  CHECK_NULLPTR(visit_tree_result);
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
  CHECK_NULLPTR(visit_tree_result);
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
  } else if (!node->stm1) {
    Label *else_label = visitor_temp_map->newlabel();
    cond->true_list->patch(end_label);
    cond->false_list->patch(else_label);

    stmts->push_back(new tree::LabelStm(else_label));
    CHECK_NULLPTR(node->stm2);
    node->stm2->accept(*this);
    stmts->push_back(static_cast<tree::Stm *>(visit_tree_result));
    stmts->push_back(new tree::LabelStm(end_label));
  } else if (!node->stm2) {
    cond->true_list->patch(true_label);
    cond->false_list->patch(end_label);

    stmts->push_back(new tree::LabelStm(true_label));
    CHECK_NULLPTR(node->stm1);
    node->stm1->accept(*this);
    stmts->push_back(static_cast<tree::Stm *>(visit_tree_result));
    stmts->push_back(new tree::LabelStm(end_label));
  } else {
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
  CHECK_NULLPTR(visit_tree_result);
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
  CHECK_NULLPTR(visit_tree_result);
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
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::CallStm *node) {
  DEBUG_PRINT("visit: CallStm node");
  CHECK_NULLPTR(node);

  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::Continue *node) {
  DEBUG_PRINT("visit: Continue node");
  CHECK_NULLPTR(node);

  if (!while_test) {
    DEBUG_PRINT("Error: Continue not in loop");
    return;
  }
  visit_tree_result = new tree::Jump(while_test);

  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::Break *node) {
  DEBUG_PRINT("visit: Break node");
  CHECK_NULLPTR(node);

  if (!while_end) {
    DEBUG_PRINT("Error: Break not in loop");
    return;
  }
  visit_tree_result = new tree::Jump(while_end);
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::Return *node) {
  DEBUG_PRINT("visit: Return node");
  CHECK_NULLPTR(node);

  if (node->exp) {
    CHECK_NULLPTR(node->exp);
    node->exp->accept(*this);
    // Tr_ex *exp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
    visit_tree_result =
        new tree::Return(currentExp->unEx(visitor_temp_map)->exp);
  } else {
    visit_tree_result = new tree::Return(nullptr);
  }
  CHECK_NULLPTR(visit_tree_result);
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
  CHECK_NULLPTR(visit_tree_result);
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
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::PutArray *node) {
  DEBUG_PRINT("visit: PutArray node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->n);
  CHECK_NULLPTR(node->arr);

  // 访问 n
  node->n->accept(*this);
  tree::Exp *n = currentExp->unEx(visitor_temp_map)->exp;

  // 访问 arr
  node->arr->accept(*this);
  tree::Exp *arr = currentExp->unEx(visitor_temp_map)->exp;

  // 构造调用参数
  vector<tree::Exp *> *args = new vector<tree::Exp *>();
  args->push_back(n);
  args->push_back(arr);

  // 构造 ExtCall 语句
  visit_tree_result =
      new tree::ExpStm(new tree::ExtCall(tree::Type::INT, "putarray", args));
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::Starttime *node) {
  DEBUG_PRINT("visit: Starttime node");
  CHECK_NULLPTR(node);

  visit_tree_result = new tree::ExpStm(
      new tree::ExtCall(tree::Type(tree::Type::INT), "starttime", nullptr));
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::Stoptime *node) {
  DEBUG_PRINT("visit: Stoptime node");
  CHECK_NULLPTR(node);

  visit_tree_result = new tree::ExpStm(
      new tree::ExtCall(tree::Type(tree::Type::INT), "stoptime", nullptr));
  CHECK_NULLPTR(visit_tree_result);
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::BinaryOp *node) {
  DEBUG_PRINT("visit: BinaryOp node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->op);
  CHECK_NULLPTR(node->left);
  CHECK_NULLPTR(node->right);

  string op = node->op->op;

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
  else if (op == "+" || op == "-" || op == "*" || op == "/") {
    node->left->accept(*this);
    Tr_ex *left = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
    node->right->accept(*this);
    Tr_ex *right = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));

    tree::Exp *binop =
        new tree::Binop(tree::Type::INT, op, left->exp, right->exp);
    visit_tree_result = binop;
    currentExp = new Tr_ex(binop);
    return;
  }

  // 比较运算 == != > >= < <=
  else if (op == "==" || op == "!=" || op == ">" || op == ">=" || op == "<" ||
           op == "<=") {
    node->left->accept(*this);
    Tr_ex *left = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
    node->right->accept(*this);
    Tr_ex *right = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));

    Label *true_label = visitor_temp_map->newlabel();
    Label *false_label = visitor_temp_map->newlabel();

    tree::Cjump *cj =
        new tree::Cjump(op, left->exp, right->exp, true_label, false_label);

    Patch_list *true_list = new Patch_list();
    Patch_list *false_list = new Patch_list();
    true_list->add_patch(cj->t);
    false_list->add_patch(cj->f);

    Tr_cx *cx = new Tr_cx(true_list, false_list, cj);
    currentExp = cx;
    visit_tree_result = cx->unEx(visitor_temp_map)->exp;
    return;
  }
  CHECK_NULLPTR(visit_tree_result);
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
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::ArrayExp *node) {
  DEBUG_PRINT("visit: ArrayExp node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->arr);
  CHECK_NULLPTR(node->index);

  // 1. 先生成 base
  node->arr->accept(*this);
  tree::Exp *base = currentExp->unEx(visitor_temp_map)->exp;

  // 2. 生成原始 idx
  node->index->accept(*this);
  tree::Exp *rawIdx = currentExp->unEx(visitor_temp_map)->exp;

  // 3. 构造越界检查的中间变量和标签
  Temp *lenTemp = visitor_temp_map->newtemp();
  Label *okLabel  = visitor_temp_map->newlabel();
  Label *errLabel = visitor_temp_map->newlabel();

  // 4. 把所有检查语句收集到一个 stmts 列表
  auto *checkStmts = new vector<tree::Stm *>();
  // lenTemp = Mem(base)
  checkStmts->push_back(new tree::Move(
    new tree::TempExp(tree::Type::INT, lenTemp),
    new tree::Mem(tree::Type::INT, base)
  ));
  // if (rawIdx >= lenTemp) goto errLabel else goto okLabel
  checkStmts->push_back(new tree::Cjump(
    ">=", rawIdx,
    new tree::TempExp(tree::Type::INT, lenTemp),
    errLabel, okLabel
  ));
  // err 分支：exit(-1)
  checkStmts->push_back(new tree::LabelStm(errLabel));
  // checkStmts->push_back(new tree::ExpStm(
  //   new tree::Call(tree::Type::INT, "exit", nullptr,
  //     new vector<tree::Exp *>{ new tree::Const(-1) })
  // ));
  // ok 分支
  checkStmts->push_back(new tree::LabelStm(okLabel));

  // 5. 用 Eseq 把这些检查语句和 rawIdx 包装起来
  // 对应标准 IR 中 <ESeq> … <Const value="0"/> 的结构，只不过这里保留原始 idx
  tree::Exp *idxEseq = new tree::Eseq(
    tree::Type::INT,
    new tree::Seq(checkStmts),
    rawIdx
  );

  // 6. 在 offset 计算中使用这个带副作用的 idxEseq
  // offset = (idxEseq + 1) * 4
  tree::Exp *offset = new tree::Binop(
    tree::Type::INT, "*",
    new tree::Binop(tree::Type::INT, "+", idxEseq, new tree::Const(1)),
    new tree::Const(4)
  );

  // 7. 计算最终地址，并返回 Mem(address)
  tree::Exp *address = new tree::Binop(
    tree::Type::PTR, "+",
    base,
    offset
  );
  currentExp = new Tr_ex(new tree::Mem(tree::Type::INT, address));
  visit_tree_result = currentExp->unEx(visitor_temp_map)->exp;
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::CallExp *node) {
  DEBUG_PRINT("visit: CallExp node");
  CHECK_NULLPTR(node);

  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::ClassVar *node) {
  DEBUG_PRINT("visit: ClassVar node");
  CHECK_NULLPTR(node);

  CHECK_NULLPTR(visit_tree_result);
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
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::This *node) {
  DEBUG_PRINT("visit: This node");
  CHECK_NULLPTR(node);

  // 获取当前方法中的变量映射表
  auto &var_table = method_var_table_map[current_method];
  if (!var_table.var_temp_map) {
    DEBUG_PRINT("Error: method_var_table_map not initialized.");
    visit_tree_result = nullptr;
    return;
  }

  auto *var_temp_map = var_table.var_temp_map;
  auto it = var_temp_map->find("this");
  if (it == var_temp_map->end()) {
    DEBUG_PRINT("Error: 'this' not found in var_temp_map.");
    visit_tree_result = nullptr;
    return;
  }

  Temp *temp = it->second;

  visit_tree_result = new tree::TempExp(tree::Type::PTR, temp);
  currentExp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::Length *node) {
  DEBUG_PRINT("visit: Length node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->exp);

  node->exp->accept(*this);
  tree::Exp *arr_base = currentExp->unEx(visitor_temp_map)->exp;

  tree::Exp *length_exp = new tree::Mem(tree::Type::INT, arr_base);

  visit_tree_result = length_exp;
  currentExp = new Tr_ex(length_exp);
  CHECK_NULLPTR(visit_tree_result);
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
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::GetInt *node) {
  DEBUG_PRINT("visit: GetInt node");
  CHECK_NULLPTR(node);

  visit_tree_result = new tree::ExpStm(
      new tree::ExtCall(tree::Type(tree::Type::INT), "getint", nullptr));
  currentExp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::GetCh *node) {
  DEBUG_PRINT("visit: GetCh node");
  CHECK_NULLPTR(node);

  visit_tree_result = new tree::ExpStm(
      new tree::ExtCall(tree::Type(tree::Type::INT), "getch", nullptr));
  currentExp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::GetArray *node) {
  DEBUG_PRINT("visit: GetArray node");
  CHECK_NULLPTR(node);
  CHECK_NULLPTR(node->exp);

  // 获取数组 base 地址
  node->exp->accept(*this);
  tree::Exp *base = currentExp->unEx(visitor_temp_map)->exp;

  // 取长度：Mem(base)
  tree::Exp *length = new tree::Mem(tree::Type::INT, base);

  vector<tree::Exp *> *args = new vector<tree::Exp *>();
  args->push_back(length);
  args->push_back(base);

  visit_tree_result =
      new tree::ExpStm(new tree::ExtCall(tree::Type::INT, "getarray", args));
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::IdExp *node) {
  DEBUG_PRINT("visit: IdExp node");
  CHECK_NULLPTR(node);

  string id = node->id;

  // 获取变量映射表
  auto &var_table = method_var_table_map[current_method];
  if (!var_table.var_temp_map) {
    var_table.var_temp_map = new map<string, Temp *>();
  }
  auto *var_temp_map = var_table.var_temp_map;

  Temp *temp = nullptr;
  if (var_temp_map->find(id) == var_temp_map->end()) {
    // 新变量，分配临时变量并记录
    temp = visitor_temp_map->newtemp();
    var_temp_map->insert({id, temp});
    cout << "====ERROR==============ERROR============ERROR============" << endl;
  } else {
    temp = (*var_temp_map)[id];
  }

  // 设置 visit_tree_result，必须返回 tree::Exp*
  tree::Type vtype = local_var_type_map[id];
  visit_tree_result = new tree::TempExp(vtype, temp);
  currentExp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::OpExp *node) {
  DEBUG_PRINT("visit: OpExp node");
  CHECK_NULLPTR(node);

  CHECK_NULLPTR(visit_tree_result);
}

void ASTToTreeVisitor::visit(fdmj::IntExp *node) {
  DEBUG_PRINT("visit: IntExp node");
  CHECK_NULLPTR(node);

  visit_tree_result = new tree::Const(node->val);
  currentExp = new Tr_ex(static_cast<tree::Exp *>(visit_tree_result));
  CHECK_NULLPTR(visit_tree_result);
}
