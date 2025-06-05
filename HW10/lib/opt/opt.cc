#define DEBUG
//#undef DEBUG

#include <string>
#include <stack>
#include <variant>
#include <vector>
#include <map>
#include "quad.hh"
#include "opt.hh"

void Opt::calculateBT() {
    // Entry block is the first block in the list
    if (func->quadblocklist->empty()) return;
    int entry = func->quadblocklist->front()->entry_label->num;
    block_executable[entry] = true;

    auto evalTerm = [&](QuadTerm *term) -> RtValue {
        if (!term) return RtValue(ValueType::NO_VALUE);
        if (term->kind == QuadTermKind::CONST) {
            return RtValue(term->get_const());
        } else if (term->kind == QuadTermKind::TEMP) {
            return getRtValue(term->get_temp()->temp->num);
        }
        return RtValue(ValueType::MANY_VALUES);
    };

    auto evalBinop = [&](const string &op, RtValue l, RtValue r) -> RtValue {
        if (l.getType() == ValueType::MANY_VALUES ||
            r.getType() == ValueType::MANY_VALUES)
            return RtValue(ValueType::MANY_VALUES);
        if (l.getType() == ValueType::NO_VALUE ||
            r.getType() == ValueType::NO_VALUE)
            return RtValue(ValueType::NO_VALUE);
        int lv = l.getIntValue();
        int rv = r.getIntValue();
        int res = 0;
        if (op == "+") res = lv + rv;
        else if (op == "-") res = lv - rv;
        else if (op == "*") res = lv * rv;
        else if (op == "/" && rv != 0) res = lv / rv;
        else return RtValue(ValueType::MANY_VALUES);
        return RtValue(res);
    };

    auto mergeValue = [&](RtValue a, RtValue b) -> RtValue {
        if (a.getType() == ValueType::NO_VALUE) return b;
        if (b.getType() == ValueType::NO_VALUE) return a;
        if (a.getType() == ValueType::ONE_VALUE &&
            b.getType() == ValueType::ONE_VALUE &&
            a.getIntValue() == b.getIntValue())
            return a;
        return RtValue(ValueType::MANY_VALUES);
    };

    auto updateValue = [&](int t, RtValue v) -> bool {
        RtValue old = getRtValue(t);
        if (old.getType() == v.getType()) {
            if (old.getType() != ValueType::ONE_VALUE ||
                old.getIntValue() == v.getIntValue())
                return false;
        }
        temp_value[t] = v;
        return true;
    };

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto &block : *func->quadblocklist) {
            if (!block_executable[block->entry_label->num]) continue;

            // evaluate each statement
            for (auto &stm : *block->quadlist) {
                switch (stm->kind) {
                    case QuadKind::MOVE: {
                        auto m = static_cast<QuadMove*>(stm);
                        RtValue v = evalTerm(m->src);
                        if (updateValue(m->dst->temp->num, v)) changed = true;
                        break;
                    }
                    case QuadKind::LOAD: {
                        auto m = static_cast<QuadLoad*>(stm);
                        if (updateValue(m->dst->temp->num,
                                         RtValue(ValueType::MANY_VALUES)))
                            changed = true;
                        break;
                    }
                    case QuadKind::MOVE_BINOP: {
                        auto m = static_cast<QuadMoveBinop*>(stm);
                        RtValue l = evalTerm(m->left);
                        RtValue r = evalTerm(m->right);
                        RtValue v = evalBinop(m->binop, l, r);
                        if (updateValue(m->dst->temp->num, v)) changed = true;
                        break;
                    }
                    case QuadKind::MOVE_CALL: {
                        auto m = static_cast<QuadMoveCall*>(stm);
                        if (updateValue(m->dst->temp->num,
                                         RtValue(ValueType::MANY_VALUES)))
                            changed = true;
                        break;
                    }
                    case QuadKind::MOVE_EXTCALL: {
                        auto m = static_cast<QuadMoveExtCall*>(stm);
                        if (updateValue(m->dst->temp->num,
                                         RtValue(ValueType::MANY_VALUES)))
                            changed = true;
                        break;
                    }
                    case QuadKind::PHI: {
                        auto p = static_cast<QuadPhi*>(stm);
                        RtValue cur(ValueType::NO_VALUE);
                        bool first = true;
                        for (auto &arg : *p->args) {
                            if (block_executable[arg.second->num]) {
                                RtValue argval = getRtValue(arg.first->num);
                                if (first) {
                                    cur = argval;
                                    first = false;
                                } else {
                                    cur = mergeValue(cur, argval);
                                }
                            }
                        }
                        if (updateValue(p->temp->temp->num, cur)) changed = true;
                        break;
                    }
                    case QuadKind::CJUMP: {
                        auto c = static_cast<QuadCJump*>(stm);
                        RtValue l = evalTerm(c->left);
                        RtValue r = evalTerm(c->right);
                        if (l.getType() == ValueType::ONE_VALUE &&
                            r.getType() == ValueType::ONE_VALUE) {
                            bool res = false;
                            int lv = l.getIntValue();
                            int rv = r.getIntValue();
                            if (c->relop == "==") res = lv == rv;
                            else if (c->relop == "!=") res = lv != rv;
                            else if (c->relop == "<") res = lv < rv;
                            else if (c->relop == "<=") res = lv <= rv;
                            else if (c->relop == ">") res = lv > rv;
                            else if (c->relop == ">=") res = lv >= rv;
                            int labelnum = res ? c->t->num : c->f->num;
                            if (!block_executable[labelnum]) {
                                block_executable[labelnum] = true;
                                changed = true;
                            }
                        } else {
                            if (!block_executable[c->t->num]) {
                                block_executable[c->t->num] = true;
                                changed = true;
                            }
                            if (!block_executable[c->f->num]) {
                                block_executable[c->f->num] = true;
                                changed = true;
                            }
                        }
                        break;
                    }
                    case QuadKind::JUMP: {
                        auto j = static_cast<QuadJump*>(stm);
                        if (!block_executable[j->label->num]) {
                            block_executable[j->label->num] = true;
                            changed = true;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
}


void Opt::modifyFunc() {
    vector<QuadBlock*> *newBlocks = new vector<QuadBlock*>();
    std::set<int> removed_bases;

    for (auto &block : *func->quadblocklist) {
        if (!block_executable[block->entry_label->num]) continue;

        vector<QuadStm*> *newList = new vector<QuadStm*>();

        for (auto &stm : *block->quadlist) {
            switch (stm->kind) {
                case QuadKind::MOVE: {
                    auto m = static_cast<QuadMove*>(stm);
                    if (m->src && m->src->kind == QuadTermKind::TEMP) {
                        Temp* tmp = m->src->get_temp()->temp;
                        RtValue v = getRtValue(tmp->num);
                        if (v.getType() == ValueType::ONE_VALUE) {
                            if (m->use) m->use->erase(tmp);
                            m->src = new QuadTerm(v.getIntValue());
                        }
                    }
                    if (getRtValue(m->dst->temp->num).getType() == ValueType::ONE_VALUE) {
                        int base = (m->dst->temp->num >= 10000) ? m->dst->temp->num / 100 : m->dst->temp->num;
                        removed_bases.insert(base);
                    } else {
                        newList->push_back(stm);
                    }
                    break;
                }
                case QuadKind::STORE: {
                    auto s = static_cast<QuadStore*>(stm);
                    auto replaceTerm = [&](QuadTerm*& t){
                        if (t && t->kind == QuadTermKind::TEMP) {
                            Temp* tmp = t->get_temp()->temp;
                            RtValue v = getRtValue(tmp->num);
                            if (v.getType() == ValueType::ONE_VALUE) {
                                if (s->use) s->use->erase(tmp);
                                t = new QuadTerm(v.getIntValue());
                            }
                        }
                    };
                    replaceTerm(s->src);
                    replaceTerm(s->dst);
                    newList->push_back(stm);
                    break;
                }
                case QuadKind::MOVE_BINOP: {
                    auto m = static_cast<QuadMoveBinop*>(stm);
                    auto replaceTerm = [&](QuadTerm *&t){
                        if (t && t->kind == QuadTermKind::TEMP) {
                            Temp* tmp = t->get_temp()->temp;
                            RtValue v = getRtValue(tmp->num);
                            if (v.getType() == ValueType::ONE_VALUE) {
                                if (m->use) m->use->erase(tmp);
                                t = new QuadTerm(v.getIntValue());
                            }
                        }
                    };
                    replaceTerm(m->left);
                    replaceTerm(m->right);
                    if (getRtValue(m->dst->temp->num).getType() == ValueType::ONE_VALUE) {
                        int base = (m->dst->temp->num >= 10000) ? m->dst->temp->num / 100 : m->dst->temp->num;
                        removed_bases.insert(base);
                    } else {
                        newList->push_back(stm);
                    }
                    break;
                }
                case QuadKind::PHI: {
                    auto p = static_cast<QuadPhi*>(stm);
                    if (getRtValue(p->temp->temp->num).getType() == ValueType::ONE_VALUE) {
                        int base = (p->temp->temp->num >= 10000) ? p->temp->temp->num / 100 : p->temp->temp->num;
                        removed_bases.insert(base);
                    } else {
                        newList->push_back(stm);
                    }
                    break;
                }
                case QuadKind::CJUMP: {
                    auto c = static_cast<QuadCJump*>(stm);
                    auto rep = [&](QuadTerm *&t) {
                        if (t && t->kind == QuadTermKind::TEMP) {
                            Temp* tmp = t->get_temp()->temp;
                            RtValue v = getRtValue(tmp->num);
                            if (v.getType() == ValueType::ONE_VALUE) {
                                if (c->use) c->use->erase(tmp);
                                t = new QuadTerm(v.getIntValue());
                            }
                        }
                    };
                    rep(c->left); rep(c->right);

                    if (c->left->kind == QuadTermKind::CONST && c->right->kind == QuadTermKind::CONST) {
                        int lv = c->left->get_const();
                        int rv = c->right->get_const();
                        bool res = false;
                        if (c->relop == "==") res = lv == rv;
                        else if (c->relop == "!=") res = lv != rv;
                        else if (c->relop == "<") res = lv < rv;
                        else if (c->relop == "<=") res = lv <= rv;
                        else if (c->relop == ">") res = lv > rv;
                        else if (c->relop == ">=") res = lv >= rv;
                        QuadJump *j = new QuadJump(nullptr, res ? c->t : c->f, nullptr, nullptr);
                        stm = j;
                    }
                    newList->push_back(stm);
                    break;
                }
                case QuadKind::RETURN: {
                    auto r = static_cast<QuadReturn*>(stm);
                    if (r->value && r->value->kind == QuadTermKind::TEMP) {
                        Temp* tmp = r->value->get_temp()->temp;
                        RtValue v = getRtValue(tmp->num);
                        if (v.getType() == ValueType::ONE_VALUE) {
                            if (r->use) r->use->erase(tmp);
                            r->value = new QuadTerm(v.getIntValue());
                        }
                    }
                    newList->push_back(stm);
                    break;
                }
                default:
                    newList->push_back(stm);
                    break;
            }
        }

        block->quadlist = newList;
        block->exit_labels->clear();
        if (!newList->empty()) {
            QuadStm *last = newList->back();
            if (last->kind == QuadKind::JUMP) {
                block->exit_labels->push_back(static_cast<QuadJump*>(last)->label);
            } else if (last->kind == QuadKind::CJUMP) {
                auto c = static_cast<QuadCJump*>(last);
                block->exit_labels->push_back(c->t);
                block->exit_labels->push_back(c->f);
            }
        }
        newBlocks->push_back(block);
    }

    func->quadblocklist = newBlocks;
    func->last_temp_num += 2;
    std::cout << "Removed bases: " << removed_bases.size() << std::endl;
}

QuadFuncDecl* Opt::optFunc() {
    // Initialize the block_executable map
    for (auto& block : *func->quadblocklist) {
        block_executable[block->entry_label->num] = false;
        label2block[block->entry_label->num] = block;
    }

    // Initialize the temp_value map for parameters
    for (auto& temp : *func->params) {
        temp_value[temp->num] = RtValue(ValueType::MANY_VALUES); // Initialize to many values for all parameters
    }

    calculateBT();

    printRtValue(); 
    printBlockExecutable();

    modifyFunc();

    return func;
}

QuadProgram* optProg(QuadProgram* prog) {
    QuadProgram* newProg = new QuadProgram(nullptr, new vector<QuadFuncDecl*>());
    for (int i=0; i < prog->quadFuncDeclList->size(); i++) {
        Opt optthis(prog->quadFuncDeclList->at(i));
        newProg->quadFuncDeclList->push_back(optthis.optFunc());
    }
    return newProg;
}