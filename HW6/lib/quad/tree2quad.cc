#define DEBUG
#undef DEBUG

#include "tree2quad.hh"
#include "quad.hh"
#include "treep.hh"
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;
using namespace tree;
using namespace quad;

/*
We use an instruction selection method (pattern matching) to convert the IR tree
to Quad. The Quad is a simplified tree node/substructure, each Quad is a tree
pattern: Move:  temp <- temp Load:  temp <- mem(temp) Store: mem(temp) <- temp
MoveBinop: temp <- temp op temp
Call:  ExpStm(call) //ignore the result
ExtCall: ExpStm(extcall) //ignore the result
MoveCall: temp <- call
MoveExtCall: temp <- extcall
Label: label
Jump: jump label
CJump: cjump relop temp, temp, label, label
Phi:  temp <- list of {temp, label} //same as the Phi in the tree
*/

QuadProgram *tree2quad(Program *prog) {
#ifdef DEBUG
  cout << "in Tree2Quad::Converting IR to Quad" << endl;
#endif
  Tree2Quad visitor;
  visitor.quadprog = nullptr;
  visitor.visit_result = nullptr;
  visitor.output_term = nullptr;
  visitor.temp_map = new Temp_map();
  visitor.current_func_decl = nullptr;
  prog->accept(visitor);
  return visitor.quadprog;
}

// You need to write all the visit functions below. Now they are all "dummies".
void Tree2Quad::visit(Program *prog) {
#ifdef DEBUG
  cout << "Converting to Quad: Program" << endl;
#endif
  if (prog == nullptr) {
    visit_result = nullptr;
    output_term = nullptr;
    return;
  }

  // Create a vector to store the program's function declarations
  vector<QuadFuncDecl *> *funcs = new vector<QuadFuncDecl *>();

  // Visit each function declaration in the program
  if (prog->funcdecllist) {
    for (FuncDecl *func : *(prog->funcdecllist)) {
      func->accept(*this);

      // Get the QuadFuncDecl from the current_func_decl member variable
      if (current_func_decl) {
        funcs->push_back(current_func_decl);
        current_func_decl = nullptr; // Reset for the next function
      }
    }
  }

  // Create the QuadProgram
  quadprog = new QuadProgram(prog, funcs);
  visit_result = nullptr;
  output_term = nullptr;
  return; // no need to visit the program
}

void Tree2Quad::visit(FuncDecl *node) {
#ifdef DEBUG
  cout << "Converting to Quad: FunctionDeclaration" << endl;
#endif
  if (node == nullptr) {
    visit_result = nullptr;
    return;
  }

  // Create a vector to store the function's parameters
  vector<Temp *> *params = new vector<Temp *>();
  if (node->args) {
    for (Temp *param : *(node->args)) {
      params->push_back(param);
    }
  }

  // Create a vector to store the function's blocks
  vector<QuadBlock *> *blocks = new vector<QuadBlock *>();

  // Initialize the temp_map with the function's last_temp_num and
  // last_label_num
  temp_map->next_temp = node->last_temp_num + 1;
  temp_map->next_label = node->last_label_num + 1;

  // Create the QuadFuncDecl first so blocks can reference it
  QuadFuncDecl *func_decl =
      new QuadFuncDecl(node, node->name, params, blocks, node->last_temp_num,
                       node->last_label_num);

  // Store it in the current_func_decl member variable
  current_func_decl = func_decl;

  // Visit each block in the function
  if (node->blocks) {
    for (Block *block : *(node->blocks)) {
      block->accept(*this);
      // The Block visitor will add the QuadBlock to
      // current_func_decl->quadblocklist
    }
  }

  // We already created the params vector at the beginning of this method
  // No need to create it again

  // We already created the QuadFuncDecl at the beginning of this method
  // No need to create it again

  // Update the last_temp_num and last_label_num in the QuadFuncDecl
  func_decl->last_temp_num = temp_map->next_temp - 1;
  func_decl->last_label_num = temp_map->next_label - 1;

  // Set visit_result to an empty vector
  vector<QuadStm *> *result = new vector<QuadStm *>();
  visit_result = result;
  output_term = nullptr;
  return;
}

void Tree2Quad::visit(Block *block) {
#ifdef DEBUG
  cout << "Converting to Quad: Block" << endl;
#endif
  if (block == nullptr) {
    visit_result = nullptr;
    return;
  }

  // Visit the block statements
  vector<QuadStm *> *stms = new vector<QuadStm *>();

  if (block->sl) {
    for (Stm *stm : *(block->sl)) {
      stm->accept(*this);
      // Accumulate statements from visit_result
      if (visit_result && !visit_result->empty()) {
        stms->insert(stms->end(), visit_result->begin(), visit_result->end());
      }
    }
  }

  // Create the QuadBlock
  QuadBlock *quad_block =
      new QuadBlock(block, stms, block->entry_label, block->exit_labels);

  // Store the result in the current function's blocks list
  if (current_func_decl) {
    current_func_decl->quadblocklist->push_back(quad_block);
  }

  // Set visit_result to an empty vector
  vector<QuadStm *> *result = new vector<QuadStm *>();
  visit_result = result;
  output_term = nullptr;
  return;
}

void Tree2Quad::visit(Jump *node) {
#ifdef DEBUG
  cout << "Converting to Quad: Jump" << endl;
#endif
  if (node == nullptr) {
    visit_result = nullptr;
    return;
  }

  // Create empty def/use sets
  set<Temp *> *def = new set<Temp *>();
  set<Temp *> *use = new set<Temp *>();

  // Create and store the Jump quad
  QuadJump *jump_quad = new QuadJump(node, node->label, def, use);
  visit_result = new vector<QuadStm *>(1, jump_quad);
  output_term = nullptr;
  return;
}

void Tree2Quad::visit(tree::Cjump *node) {
#ifdef DEBUG
  cout << "Converting to Quad: CJump" << endl;
#endif
  // only one tile pattern matches this CJump
  if (node == nullptr || node->right == nullptr || node->left == nullptr) {
    visit_result = nullptr;
    return;
  }
  vector<QuadStm *> *result = new vector<QuadStm *>();

  // Visit left operand - this may generate intermediate quads for complex
  // expressions
  node->left->accept(*this);
  QuadTerm *left_term = output_term;

  // If visit_result is not null, it means the left operand generated
  // intermediate quads
  if (visit_result != nullptr) {
    result->insert(result->end(), visit_result->begin(), visit_result->end());
  }

  // Visit right operand - this may generate intermediate quads for complex
  // expressions
  node->right->accept(*this);
  QuadTerm *right_term = output_term;

  // If visit_result is not null, it means the right operand generated
  // intermediate quads
  if (visit_result != nullptr) {
    result->insert(result->end(), visit_result->begin(), visit_result->end());
  }

  // Create def/use sets
  set<Temp *> *def = new set<Temp *>();
  set<Temp *> *use = new set<Temp *>();

  if (left_term->kind == QuadTermKind::TEMP) {
    use->insert(left_term->get_temp()->temp);
  }
  if (right_term->kind == QuadTermKind::TEMP) {
    use->insert(right_term->get_temp()->temp);
  }

  // Create and store the CJump quad
  QuadCJump *cjump_quad = new QuadCJump(node, node->relop, left_term,
                                        right_term, node->t, node->f, def, use);
  result->push_back(cjump_quad);

  visit_result = result;

  output_term = nullptr;
  return;
}

void Tree2Quad::visit(Move *node) {
#ifdef DEBUG
  cout << "Converting to Quad: Move" << endl;
#endif
  if (node == nullptr || node->dst == nullptr || node->src == nullptr) {
    visit_result = nullptr;
    output_term = nullptr;
    return;
  }

  // Handle store: mem(addr) <- src
  if (node->dst->getTreeKind() == Kind::MEM) {
    Mem *mem_dst = static_cast<Mem *>(node->dst);

    // Get address term
    mem_dst->mem->accept(*this);
    QuadTerm *addr_term = output_term;

    // Get source term
    node->src->accept(*this);
    QuadTerm *src_term = output_term;

    // Create def/use sets
    set<Temp *> *def = new set<Temp *>();
    set<Temp *> *use = new set<Temp *>();

    if (addr_term->kind == QuadTermKind::TEMP) {
      use->insert(addr_term->get_temp()->temp);
    }
    if (src_term->kind == QuadTermKind::TEMP) {
      use->insert(src_term->get_temp()->temp);
    }

    // Create and store the Store quad
    QuadStore *store_quad = new QuadStore(node, src_term, addr_term, def, use);
    visit_result = new vector<QuadStm *>(1, store_quad);
    output_term = nullptr;
    return;
  }

  // Handle load/move: dst <- src
  if (node->dst->getTreeKind() == Kind::TEMPEXP) {
    TempExp *temp_dst = static_cast<TempExp *>(node->dst);

    // Handle load: temp <- mem(addr)
    if (node->src->getTreeKind() == Kind::MEM) {
      Mem *mem_src = static_cast<Mem *>(node->src);

      // Get address term
      mem_src->mem->accept(*this);
      QuadTerm *addr_term = output_term;

      // Create def/use sets
      set<Temp *> *def = new set<Temp *>();
      def->insert(temp_dst->temp);

      set<Temp *> *use = new set<Temp *>();
      if (addr_term->kind == QuadTermKind::TEMP) {
        use->insert(addr_term->get_temp()->temp);
      }

      // Create and store the Load quad
      QuadLoad *load_quad = new QuadLoad(node, temp_dst, addr_term, def, use);
      visit_result = new vector<QuadStm *>(1, load_quad);
      output_term = new QuadTerm(temp_dst);
      return;
    }

    // Handle move: temp <- term

    // Handle move binop: temp <- term op term
    if (node->src->getTreeKind() == Kind::BINOP) {
      Binop *binop_src = static_cast<Binop *>(node->src);

      // Visit left operand
      binop_src->left->accept(*this);
      QuadTerm *left_term = output_term;

      // Visit right operand
      binop_src->right->accept(*this);
      QuadTerm *right_term = output_term;

      // Create def/use sets
      set<Temp *> *def = new set<Temp *>();
      def->insert(temp_dst->temp);

      set<Temp *> *use = new set<Temp *>();
      if (left_term->kind == QuadTermKind::TEMP) {
        use->insert(left_term->get_temp()->temp);
      }
      if (right_term->kind == QuadTermKind::TEMP) {
        use->insert(right_term->get_temp()->temp);
      }

      // Create and store the MoveBinop quad
      QuadMoveBinop *binop_quad = new QuadMoveBinop(
          node, temp_dst, left_term, binop_src->op, right_term, def, use);
      visit_result = new vector<QuadStm *>(1, binop_quad);
      output_term = new QuadTerm(temp_dst);
      return;
    }

    // Handle simple move: temp <- term
    node->src->accept(*this);
    QuadTerm *src_term = output_term;

    // Create def/use sets
    set<Temp *> *def = new set<Temp *>();
    def->insert(temp_dst->temp);

    set<Temp *> *use = new set<Temp *>();
    if (src_term->kind == QuadTermKind::TEMP) {
      use->insert(src_term->get_temp()->temp);
    }

    // Create and store the Move quad
    QuadMove *move_quad = new QuadMove(node, temp_dst, src_term, def, use);
    visit_result = new vector<QuadStm *>(1, move_quad);
    output_term = new QuadTerm(temp_dst);
    return;
  }

  // Unsupported move pattern
  cerr << "Error: Unsupported move pattern!" << endl;
  visit_result = nullptr;
  output_term = nullptr;
}

void Tree2Quad::visit(Seq *node) {
#ifdef DEBUG
  cout << "Converting to Quad: Sequence" << endl;
#endif
  if (node == nullptr || node->sl == nullptr) {
    visit_result = nullptr;
    output_term = nullptr;
    return;
  }

  vector<QuadStm *> *result = new vector<QuadStm *>();

  // Visit each statement in the sequence and collect the results
  for (Stm *stm : *(node->sl)) {
    stm->accept(*this);
    if (visit_result != nullptr) {
      result->insert(result->end(), visit_result->begin(), visit_result->end());
    }
  }

  visit_result = result;
  output_term = nullptr;
  return;
}

void Tree2Quad::visit(LabelStm *node) {
#ifdef DEBUG
  cout << "Converting to Quad: Label" << endl;
#endif
  if (node == nullptr) {
    visit_result = nullptr;
    return;
  }

  // Create empty def/use sets
  set<Temp *> *def = new set<Temp *>();
  set<Temp *> *use = new set<Temp *>();

  // Create and store the Label quad
  QuadLabel *label_quad = new QuadLabel(node, node->label, def, use);
  visit_result = new vector<QuadStm *>(1, label_quad);
  output_term = nullptr;
  return;
}

void Tree2Quad::visit(Return *node) {
#ifdef DEBUG
  cout << "Converting to Quad: Return" << endl;
#endif
  if (node == nullptr || node->exp == nullptr) {
    visit_result = nullptr;
    return;
  }

  // Visit the return expression
  node->exp->accept(*this);
  QuadTerm *value = output_term;

  // Create def/use sets
  set<Temp *> *def = new set<Temp *>();
  set<Temp *> *use = new set<Temp *>();

  if (value->kind == QuadTermKind::TEMP) {
    use->insert(value->get_temp()->temp);
  }

  // Create and store the Return quad
  QuadReturn *return_quad = new QuadReturn(node, value, def, use);
  visit_result = new vector<QuadStm *>(1, return_quad);
  output_term = nullptr;
  return;
}

void Tree2Quad::visit(Phi *node) {
#ifdef DEBUG
  cout << "Converting to Quad: Phi" << endl;
#endif
  if (node == nullptr) {
    visit_result = nullptr;
    return;
  }

  // Create def/use sets
  set<Temp *> *def = new set<Temp *>();
  def->insert(node->temp);

  set<Temp *> *use = new set<Temp *>();
  for (auto &pair : *(node->args)) {
    use->insert(pair.first);
  }

  // Create and store the Phi quad
  TempExp *temp_exp = new TempExp(tree::Type::INT, node->temp);
  QuadPhi *phi_quad = new QuadPhi(node, temp_exp, node->args, def, use);
  visit_result = new vector<QuadStm *>(1, phi_quad);
  output_term = new QuadTerm(temp_exp);
  return;
}

void Tree2Quad::visit(ExpStm *node) {
#ifdef DEBUG
  cout << "Converting to Quad: ExpressionStatement" << endl;
#endif
  if (node == nullptr || node->exp == nullptr) {
    visit_result = nullptr;
    return;
  }

  // Visit the expression
  node->exp->accept(*this);
  visit_result = this->visit_result;
  output_term = nullptr;
  return;
}

void Tree2Quad::visit(Binop *node) {
#ifdef DEBUG
  cout << "Converting to Quad: BinaryOperation" << endl;
#endif
  if (node == nullptr || node->left == nullptr || node->right == nullptr) {
    visit_result = nullptr;
    return;
  }

  vector<QuadStm *> *result = new vector<QuadStm *>();

  // Visit left operand - this may generate intermediate quads for complex
  // expressions
  node->left->accept(*this);
  QuadTerm *left_term = output_term;

  // If visit_result is not null, it means the left operand generated
  // intermediate quads
  if (visit_result != nullptr) {
    result->insert(result->end(), visit_result->begin(), visit_result->end());
  }

  // Visit right operand - this may generate intermediate quads for complex
  // expressions
  node->right->accept(*this);
  QuadTerm *right_term = output_term;

  // If visit_result is not null, it means the right operand generated
  // intermediate quads
  if (visit_result != nullptr) {
    result->insert(result->end(), visit_result->begin(), visit_result->end());
  }

  // Create a new temporary to hold the result
  TempExp *dst = new TempExp(node->type, temp_map->newtemp());

  // Create def/use sets
  set<Temp *> *def = new set<Temp *>();
  def->insert(dst->temp);

  set<Temp *> *use = new set<Temp *>();
  if (left_term->kind == QuadTermKind::TEMP) {
    use->insert(left_term->get_temp()->temp);
  }
  if (right_term->kind == QuadTermKind::TEMP) {
    use->insert(right_term->get_temp()->temp);
  }

  // Create and store the MoveBinop quad
  QuadMoveBinop *binop_quad =
      new QuadMoveBinop(node, dst, left_term, node->op, right_term, def, use);
  result->push_back(binop_quad);

  visit_result = result;
  output_term = new QuadTerm(dst);
}

// convert the memory address to a load quad
void Tree2Quad::visit(Mem *node) {
#ifdef DEBUG
  cout << "Converting to Quad: Memory" << endl;
#endif
  if (node == nullptr || node->mem == nullptr) {
    cerr << "Error: memory address is missing!" << endl;
    visit_result = nullptr;
    return;
  }

  // Visit the memory address expression
  node->mem->accept(*this);

  // Store the address term for later use in Move or other operations
  output_term = this->output_term;
  visit_result = nullptr;
  return;
}

void Tree2Quad::visit(TempExp *node) {
#ifdef DEBUG
  cout << "Converting to Quad: Temp" << endl;
#endif
  if (node == nullptr) {
    cerr << "Error: Temp is null!" << endl;
    visit_result = nullptr;
    return;
  }
  output_term = new QuadTerm(node);
  visit_result = nullptr;
  return;
}

// the following is useless since IR is canon
void Tree2Quad::visit(Eseq *node) {
#ifdef DEBUG
  cout << "Converting to Quad: ESeq" << endl;
#endif
  visit_result = nullptr;
  output_term = nullptr;
  return;
}

// convert the label to a QuadTerm(name)
void Tree2Quad::visit(Name *node) {
#ifdef DEBUG
  cout << "Converting to Quad: Name" << endl;
#endif
  if (node == nullptr) {
    visit_result = nullptr;
    return;
  }
  output_term = node->sname ? new QuadTerm(node->sname->name)
                            : new QuadTerm(std::to_string(node->name->name()));
  visit_result = nullptr;
  return;
}

void Tree2Quad::visit(Const *node) {
#ifdef DEBUG
  cout << "Converting to Quad: Const" << endl;
#endif
  if (node == nullptr) {
    visit_result = nullptr;
    return;
  }
  output_term = new QuadTerm(node->constVal);
  visit_result = nullptr;
  return;
}

void Tree2Quad::visit(Call *node) {
#ifdef DEBUG
  cout << "Converting to Quad: Call" << endl;
#endif
  if (node == nullptr) {
    visit_result = nullptr;
    return;
  }

  // Visit object
  QuadTerm *obj_term = nullptr;
  if (node->obj) {
    node->obj->accept(*this);
    obj_term = output_term;
  }

  // Visit arguments
  vector<QuadTerm *> *args = new vector<QuadTerm *>();
  set<Temp *> *use = new set<Temp *>();

  if (node->args) {
    for (Exp *arg : *(node->args)) {
      arg->accept(*this);
      args->push_back(output_term);

      if (output_term->kind == QuadTermKind::TEMP) {
        use->insert(output_term->get_temp()->temp);
      }
    }
  }

  // Create a temp for the result
  TempExp *result_temp = new TempExp(node->type, temp_map->newtemp());

  // Create def/use sets
  set<Temp *> *def = new set<Temp *>();
  def->insert(result_temp->temp);

  if (obj_term && obj_term->kind == QuadTermKind::TEMP) {
    use->insert(obj_term->get_temp()->temp);
  }

  // Create and store the Call quad
  QuadCall *call_quad =
      new QuadCall(node, result_temp, node->id, obj_term, args, def, use);
  visit_result = new vector<QuadStm *>(1, call_quad);
  output_term = new QuadTerm(result_temp);
}

void Tree2Quad::visit(ExtCall *node) {
#ifdef DEBUG
  cout << "Converting to Quad: ExtCall" << endl;
#endif
  if (node == nullptr) {
    visit_result = nullptr;
    return;
  }

  // Visit arguments
  vector<QuadTerm *> *args = new vector<QuadTerm *>();
  set<Temp *> *use = new set<Temp *>();

  if (node->args) {
    for (Exp *arg : *(node->args)) {
      arg->accept(*this);
      args->push_back(output_term);

      if (output_term->kind == QuadTermKind::TEMP) {
        use->insert(output_term->get_temp()->temp);
      }
    }
  }

  // Create def/use sets
  set<Temp *> *def = new set<Temp *>();

  // Only create a result temp if the function returns a value
  // For now, we'll assume functions like "putch" don't return a value
  TempExp *result_temp = nullptr;
  if (node->extfun != "putch") {
    result_temp = new TempExp(node->type, temp_map->newtemp());
    def->insert(result_temp->temp);
  }

  // Create and store the ExtCall quad
  QuadExtCall *extcall_quad =
      new QuadExtCall(node, result_temp, node->extfun, args, def, use);
  visit_result = new vector<QuadStm *>(1, extcall_quad);

  // Set output_term to the result temp if it exists
  output_term = result_temp ? new QuadTerm(result_temp) : nullptr;
  return;
}