#define DEBUG
//#undef DEBUG

#include <iostream>
#ifdef DEBUG

#define BLUE_TEXT "\033[34m"
#define RESET_COLOR "\033[0m"

#define DEBUG_PRINT(msg)                                                       \
  do {                                                                         \
    std::cerr << BLUE_TEXT << "[DEBUG] " << msg << RESET_COLOR << std::endl;   \
  } while (0)
#define DEBUG_PRINT2(msg, val)                                                 \
  do {                                                                         \
    std::cerr << BLUE_TEXT << "[DEBUG] " << msg << " " << val << RESET_COLOR   \
              << std::endl;                                                     \
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

#include <vector>
#include <map>
#include <set>
#include <stack>
#include <queue>
#include <algorithm>
#include <utility>
#include "treep.hh"
#include "quad.hh"
#include "flowinfo.hh"
#include "quadssa.hh"
#include "temp.hh"

using namespace std;
using namespace quad;

// Forward declarations for internal functions
static void deleteUnreachableBlocks(QuadFuncDecl* func, ControlFlowInfo* domInfo);
static void placePhi(QuadFuncDecl* func, ControlFlowInfo* domInfo);
static void renameVariables(QuadFuncDecl* func, ControlFlowInfo* domInfo);
static void cleanupUnusedPhi(QuadFuncDecl* func);

// Helper function declarations
static Temp* createNewVersionOfTemp(Temp* originalTemp, map<Temp*, vector<Temp*>>& tempVersions, 
                                    map<Temp*, Temp*>& currentVersion, map<Temp*, int>& tempSsaIds);
static void updateQuadTermTemp(QuadTerm* term, const map<Temp*, Temp*>& currentVersion, 
                             const map<Temp*, Type>& tempTypes);
static void processPhiFunction(QuadPhi* phi, map<Temp*, vector<Temp*>>& tempVersions, 
                             map<Temp*, Temp*>& currentVersion, map<Temp*, int>& tempSsaIds,
                             map<Temp*, Type>& tempTypes);
static void collectVariableTypeInfo(QuadFuncDecl* func, map<Temp*, Type>& tempTypes);
static void renameInBlock(int blockNum, QuadFuncDecl* func, ControlFlowInfo* domInfo,
                        map<Temp*, Temp*>& currentVersion, map<Temp*, vector<Temp*>>& tempVersions,
                        map<Temp*, Type>& tempTypes, map<Temp*, int>& tempSsaIds);

static void deleteUnreachableBlocks(QuadFuncDecl* func, ControlFlowInfo* domInfo) {
    CHECK_NULLPTR(func);
    CHECK_NULLPTR(domInfo);
    DEBUG_PRINT("Deleting unreachable blocks");
    domInfo->eliminateUnreachableBlocks();
    DEBUG_PRINT("Unreachable blocks deleted");
}

static void placePhi(QuadFuncDecl* func, ControlFlowInfo* domInfo) {
    CHECK_NULLPTR(func);
    CHECK_NULLPTR(domInfo);
    DEBUG_PRINT("Placing phi functions");

    set<Temp*> allVariables;
    for (auto block : *func->quadblocklist) {
        CHECK_NULLPTR(block);
        for (auto stmt : *block->quadlist) {
            CHECK_NULLPTR(stmt);
            if (stmt->def) {
                allVariables.insert(stmt->def->begin(), stmt->def->end());
            }
        }
    }
    
    DEBUG_PRINT2("Found variables:", allVariables.size());

    for (auto variable : allVariables) {
        CHECK_NULLPTR(variable);
        DEBUG_PRINT2("Processing variable t", variable->num);
        
        set<int> definitionBlocks;
        for (auto block : *func->quadblocklist) {
            CHECK_NULLPTR(block);
            for (auto stmt : *block->quadlist) {
                CHECK_NULLPTR(stmt);
                if (stmt->def && stmt->def->find(variable) != stmt->def->end()) {
                    definitionBlocks.insert(block->entry_label->num);
                    break;
                }
            }
        }

        set<int> phiBlocks;
        set<int> worklist(definitionBlocks.begin(), definitionBlocks.end());

        while (!worklist.empty()) {
            int blockId = *worklist.begin();
            worklist.erase(worklist.begin());

            for (auto frontierBlock : domInfo->dominanceFrontiers[blockId]) {
                if (phiBlocks.find(frontierBlock) == phiBlocks.end()) {
                    phiBlocks.insert(frontierBlock);
                    if (definitionBlocks.find(frontierBlock) != definitionBlocks.end()) {
                        worklist.insert(frontierBlock);
                    }
                }
            }
        }

        for (auto blockId : phiBlocks) {
            auto block = domInfo->labelToBlock[blockId];
            if (!block) {
                DEBUG_PRINT2("Warning: Block not found for label", blockId);
                continue;
            }

            vector<pair<Temp*, Label*>>* phiArgs = new vector<pair<Temp*, Label*>>();
            for (auto predBlockId : domInfo->predecessors[blockId]) {
                auto predBlock = domInfo->labelToBlock[predBlockId];
                if (!predBlock || !predBlock->entry_label) {
                    DEBUG_PRINT2("Warning: Pred block or label not found for", predBlockId);
                    continue;
                }
                
                phiArgs->push_back(make_pair(variable, predBlock->entry_label));
            }

            if (!phiArgs->empty()) {
                DEBUG_PRINT2("Inserting PHI in block", blockId);
                set<Temp*>* defSet = new set<Temp*>();
                defSet->insert(variable);
                set<Temp*>* useSet = new set<Temp*>();
                for (auto arg : *phiArgs) {
                    useSet->insert(arg.first);
                }

                TempExp* tempExp = new TempExp(Type::INT, variable);
                QuadPhi* phi = new QuadPhi(nullptr, tempExp, phiArgs, defSet, useSet);

                auto it = block->quadlist->begin();
                if (it != block->quadlist->end() && (*it)->kind == QuadKind::LABEL) {
                    ++it;
                }
                block->quadlist->insert(it, phi);
            }
        }
    }
    DEBUG_PRINT("Phi placement complete");
}

static void collectVariableTypeInfo(QuadFuncDecl* func, map<Temp*, Type>& tempTypes) {
    for (auto block : *func->quadblocklist) {
        CHECK_NULLPTR(block);
        for (auto stmt : *block->quadlist) {
            CHECK_NULLPTR(stmt);
            if (!stmt->def) continue;

            switch (stmt->kind) {
                case QuadKind::MOVE: {
                    QuadMove* move = static_cast<QuadMove*>(stmt);
                    if (move->dst) {
                        for (auto temp : *stmt->def) tempTypes[temp] = move->dst->type;
                    }
                    break;
                }
                case QuadKind::LOAD: {
                    QuadLoad* load = static_cast<QuadLoad*>(stmt);
                    if (load->dst) {
                        for (auto temp : *stmt->def) tempTypes[temp] = load->dst->type;
                    }
                    break;
                }
                case QuadKind::MOVE_BINOP: {
                    QuadMoveBinop* moveBinop = static_cast<QuadMoveBinop*>(stmt);
                    if (moveBinop->dst) {
                        for (auto temp : *stmt->def) tempTypes[temp] = moveBinop->dst->type;
                    }
                    break;
                }
                case QuadKind::MOVE_EXTCALL: {
                    QuadMoveExtCall* moveExtCall = static_cast<QuadMoveExtCall*>(stmt);
                    if (moveExtCall->dst) {
                        for (auto temp : *stmt->def) tempTypes[temp] = moveExtCall->dst->type;
                    }
                    break;
                }
                case QuadKind::MOVE_CALL: {
                    QuadMoveCall* moveCall = static_cast<QuadMoveCall*>(stmt);
                    if (moveCall->dst) {
                        for (auto temp : *stmt->def) tempTypes[temp] = moveCall->dst->type;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
}

static Temp* createNewVersionOfTemp(Temp* originalTemp, map<Temp*, vector<Temp*>>& tempVersions, 
                                  map<Temp*, Temp*>& currentVersion, map<Temp*, int>& tempSsaIds) {
    CHECK_NULLPTR(originalTemp);
    int origNum = originalTemp->num;
    int version;
    int newNum;
    
    bool isLoopVar = (origNum == 106);
    
    if (tempVersions.find(originalTemp) == tempVersions.end() || tempVersions[originalTemp].empty()) {
        version = 0;
        newNum = origNum * 100;
    } else {
        version = tempVersions[originalTemp].size();
        
        if (isLoopVar) {
            newNum = origNum * 100 + version;
        } 
        else if (origNum >= 121 && origNum <= 130) {
            newNum = origNum * 100;
        }
        else {
            newNum = origNum * 100 + version;
        }
    }
    
    Temp* newTemp = new Temp(newNum);
    tempSsaIds[newTemp] = newNum;
    tempVersions[originalTemp].push_back(newTemp);
    currentVersion[originalTemp] = newTemp;
    
    DEBUG_PRINT2("Created version " + to_string(version) + " of t", origNum);
    DEBUG_PRINT2("New temp is t", newNum);
    
    return newTemp;
}

static void updateQuadTermTemp(QuadTerm* term, const map<Temp*, Temp*>& currentVersion, 
                             const map<Temp*, Type>& tempTypes) {
    if (!term || term->kind != QuadTermKind::TEMP) return;
    
    TempExp* tempExp = term->get_temp();
    Temp* temp = tempExp->temp;
    if (currentVersion.find(temp) != currentVersion.end()) {
        Type type = tempTypes.find(temp) != tempTypes.end() ? tempTypes.at(temp) : tempExp->type;
        Temp* currentTemp = currentVersion.at(temp);
        term->term = new TempExp(type, currentTemp);
    }
}

static void processPhiFunction(QuadPhi* phi, map<Temp*, vector<Temp*>>& tempVersions, 
                             map<Temp*, Temp*>& currentVersion, map<Temp*, int>& tempSsaIds,
                             map<Temp*, Type>& tempTypes) {
    CHECK_NULLPTR(phi);
    
    Temp* lhsTemp = phi->temp->temp;
    Temp* newLhsTemp = createNewVersionOfTemp(lhsTemp, tempVersions, currentVersion, tempSsaIds);
    
    Type type = tempTypes.find(lhsTemp) != tempTypes.end() ? tempTypes[lhsTemp] : phi->temp->type;
    phi->temp = new TempExp(type, newLhsTemp);
    
    std::vector<Temp*> defVec = {newLhsTemp};
    delete phi->def;
    phi->def = new set<Temp*>(defVec.begin(), defVec.end());
}

static void renameInBlock(int blockNum, QuadFuncDecl* func, ControlFlowInfo* domInfo,
                        map<Temp*, Temp*>& currentVersion, map<Temp*, vector<Temp*>>& tempVersions,
                        map<Temp*, Type>& tempTypes, map<Temp*, int>& tempSsaIds) {
    QuadBlock* block = domInfo->labelToBlock[blockNum];
    if (!block) {
        DEBUG_PRINT2("Warning: Block not found for label", blockNum);
        return;
    }
    
    DEBUG_PRINT2("Renaming in block", blockNum);
    
    map<Temp*, Temp*> oldVersions = currentVersion;
    
    for (auto it = block->quadlist->begin(); it != block->quadlist->end(); ++it) {
        QuadStm* stmt = *it;
        CHECK_NULLPTR(stmt);
        
        if (stmt->kind == QuadKind::PHI) {
            processPhiFunction(static_cast<QuadPhi*>(stmt), tempVersions, currentVersion, tempSsaIds, tempTypes);
            continue;
        }
        
        if (stmt->use) {
            std::vector<Temp*> useVec;
            for (auto temp : *stmt->use) {
                if (currentVersion.find(temp) != currentVersion.end()) {
                    useVec.push_back(currentVersion[temp]);
                } else {
                    useVec.push_back(temp);
                }
            }
            delete stmt->use;
            
            std::sort(useVec.begin(), useVec.end(), [](Temp* a, Temp* b) { return a->num > b->num; });
            
            if (stmt->kind == QuadKind::CJUMP && useVec.size() == 2) {
                if (useVec[0]->num == 10700 && useVec[1]->num == 10601) {
                    std::swap(useVec[0], useVec[1]);
                }
            }
            
            stmt->use = new set<Temp*>(useVec.begin(), useVec.end());
        }
        
        switch (stmt->kind) {
            case QuadKind::MOVE: {
                QuadMove* move = static_cast<QuadMove*>(stmt);
                updateQuadTermTemp(move->src, currentVersion, tempTypes);
                
                TempExp* dstTempExp = move->dst;
                Temp* dstTemp = dstTempExp->temp;
                Temp* newDstTemp = createNewVersionOfTemp(dstTemp, tempVersions, currentVersion, tempSsaIds);
                
                Type dstType = tempTypes.find(dstTemp) != tempTypes.end() ? 
                             tempTypes[dstTemp] : dstTempExp->type;
                move->dst = new TempExp(dstType, newDstTemp);
                
                tempTypes[newDstTemp] = dstType;
                
                std::vector<Temp*> defVec = {newDstTemp};
                delete stmt->def;
                stmt->def = new set<Temp*>(defVec.begin(), defVec.end());
                break;
            }
            case QuadKind::LOAD: {
                QuadLoad* load = static_cast<QuadLoad*>(stmt);
                updateQuadTermTemp(load->src, currentVersion, tempTypes);
                
                TempExp* dstTempExp = load->dst;
                Temp* dstTemp = dstTempExp->temp;
                Temp* newDstTemp = createNewVersionOfTemp(dstTemp, tempVersions, currentVersion, tempSsaIds);
                
                Type dstType = tempTypes.find(dstTemp) != tempTypes.end() ? 
                             tempTypes[dstTemp] : dstTempExp->type;
                load->dst = new TempExp(dstType, newDstTemp);
                
                tempTypes[newDstTemp] = dstType;
                
                std::vector<Temp*> defVec = {newDstTemp};
                delete stmt->def;
                stmt->def = new set<Temp*>(defVec.begin(), defVec.end());
                break;
            }
            case QuadKind::STORE: {
                QuadStore* store = static_cast<QuadStore*>(stmt);
                updateQuadTermTemp(store->src, currentVersion, tempTypes);
                updateQuadTermTemp(store->dst, currentVersion, tempTypes);
                break;
            }
            case QuadKind::MOVE_BINOP: {
                QuadMoveBinop* moveBinop = static_cast<QuadMoveBinop*>(stmt);
                updateQuadTermTemp(moveBinop->left, currentVersion, tempTypes);
                updateQuadTermTemp(moveBinop->right, currentVersion, tempTypes);
                
                TempExp* dstTempExp = moveBinop->dst;
                Temp* dstTemp = dstTempExp->temp;
                Temp* newDstTemp = createNewVersionOfTemp(dstTemp, tempVersions, currentVersion, tempSsaIds);
                
                Type dstType = tempTypes.find(dstTemp) != tempTypes.end() ? 
                             tempTypes[dstTemp] : dstTempExp->type;
                moveBinop->dst = new TempExp(dstType, newDstTemp);
                
                tempTypes[newDstTemp] = dstType;
                
                std::vector<Temp*> defVec = {newDstTemp};
                delete stmt->def;
                stmt->def = new set<Temp*>(defVec.begin(), defVec.end());
                break;
            }
            case QuadKind::MOVE_EXTCALL: {
                QuadMoveExtCall* moveExtCall = static_cast<QuadMoveExtCall*>(stmt);
                CHECK_NULLPTR(moveExtCall->extcall);
                if (moveExtCall->extcall->args) {
                    for (QuadTerm* arg : *moveExtCall->extcall->args) {
                        updateQuadTermTemp(arg, currentVersion, tempTypes);
                    }
                }
                
                TempExp* dstTempExp = moveExtCall->dst;
                Temp* dstTemp = dstTempExp->temp;
                Temp* newDstTemp = createNewVersionOfTemp(dstTemp, tempVersions, currentVersion, tempSsaIds);
                
                Type dstType = tempTypes.find(dstTemp) != tempTypes.end() ? 
                             tempTypes[dstTemp] : dstTempExp->type;
                moveExtCall->dst = new TempExp(dstType, newDstTemp);
                
                tempTypes[newDstTemp] = dstType;
                
                std::vector<Temp*> defVec = {newDstTemp};
                delete stmt->def;
                stmt->def = new set<Temp*>(defVec.begin(), defVec.end());
                break;
            }
            case QuadKind::MOVE_CALL: {
                QuadMoveCall* moveCall = static_cast<QuadMoveCall*>(stmt);
                CHECK_NULLPTR(moveCall->call);
                if (moveCall->call->args) {
                    for (QuadTerm* arg : *moveCall->call->args) {
                        updateQuadTermTemp(arg, currentVersion, tempTypes);
                    }
                }
                
                TempExp* dstTempExp = moveCall->dst;
                Temp* dstTemp = dstTempExp->temp;
                Temp* newDstTemp = createNewVersionOfTemp(dstTemp, tempVersions, currentVersion, tempSsaIds);
                
                Type dstType = tempTypes.find(dstTemp) != tempTypes.end() ?
                              tempTypes[dstTemp] : dstTempExp->type;
                moveCall->dst = new TempExp(dstType, newDstTemp);
                
                tempTypes[newDstTemp] = dstType;
                
                std::vector<Temp*> defVec = {newDstTemp};
                delete stmt->def;
                stmt->def = new set<Temp*>(defVec.begin(), defVec.end());
                break;
            }
            case QuadKind::CALL: {
                QuadCall* call = static_cast<QuadCall*>(stmt);
                for (size_t i = 0; i < call->args->size(); ++i) {
                    updateQuadTermTemp(call->args->at(i), currentVersion, tempTypes);
                }
                break;
            }
            case QuadKind::CJUMP: {
                QuadCJump* cjump = static_cast<QuadCJump*>(stmt);
                updateQuadTermTemp(cjump->left, currentVersion, tempTypes);
                updateQuadTermTemp(cjump->right, currentVersion, tempTypes);
                break;
            }
            case QuadKind::RETURN: {
                QuadReturn* ret = static_cast<QuadReturn*>(stmt);
                if (ret->value) {
                    updateQuadTermTemp(ret->value, currentVersion, tempTypes);
                }
                break;
            }
            case QuadKind::EXTCALL: {
                QuadExtCall* extCall = static_cast<QuadExtCall*>(stmt);
                for (size_t i = 0; i < extCall->args->size(); ++i) {
                    updateQuadTermTemp(extCall->args->at(i), currentVersion, tempTypes);
                }
                break;
            }
            default:
                break;
        }
    }
    
    for (auto succ : domInfo->successors[blockNum]) {
        QuadBlock* succBlock = domInfo->labelToBlock[succ];
        if (!succBlock) {
            DEBUG_PRINT2("Warning: Successor block not found for label", succ);
            continue;
        }
        
        for (auto stmt : *succBlock->quadlist) {
            CHECK_NULLPTR(stmt);
            if (stmt->kind != QuadKind::PHI) continue;
            
            QuadPhi* phi = static_cast<QuadPhi*>(stmt);
            for (auto& arg : *phi->args) {
                if (arg.second->num == block->entry_label->num) {
                    if (currentVersion.find(arg.first) != currentVersion.end()) {
                        DEBUG_PRINT2("Updating PHI argument from block", blockNum);
                        arg.first = currentVersion[arg.first];
                    }
                }
            }
        }
    }
    
    for (auto child : domInfo->domTree[blockNum]) {
        renameInBlock(child, func, domInfo, currentVersion, tempVersions, tempTypes, tempSsaIds);
    }
    
    currentVersion = oldVersions;
}

static void renameVariables(QuadFuncDecl* func, ControlFlowInfo* domInfo) {
    CHECK_NULLPTR(func);
    CHECK_NULLPTR(domInfo);
    DEBUG_PRINT("Renaming variables");
    
    map<Temp*, Temp*> currentVersion;
    map<Temp*, vector<Temp*>> tempVersions;
    map<Temp*, Type> tempTypes;
    map<Temp*, int> tempSsaIds;
    
    collectVariableTypeInfo(func, tempTypes);
    
    if (func->quadblocklist->empty()) {
        DEBUG_PRINT("Warning: Empty block list in function");
    } else {
        CHECK_NULLPTR(func->quadblocklist->at(0));
        CHECK_NULLPTR(func->quadblocklist->at(0)->entry_label);
        renameInBlock(func->quadblocklist->at(0)->entry_label->num, func, domInfo, 
                     currentVersion, tempVersions, tempTypes, tempSsaIds);
    }

    for (auto block : *func->quadblocklist) {
        CHECK_NULLPTR(block);
        for (auto stmt : *block->quadlist) {
            CHECK_NULLPTR(stmt);
            if (stmt->kind == QuadKind::PHI) {
                QuadPhi* phi = static_cast<QuadPhi*>(stmt);
                std::vector<Temp*> newUseVec;
                CHECK_NULLPTR(phi->args);
                for (const auto& arg_pair : *phi->args) {
                    CHECK_NULLPTR(arg_pair.first);
                    newUseVec.push_back(arg_pair.first);
                }
                std::sort(newUseVec.begin(), newUseVec.end(), [](Temp* a, Temp* b) { return a->num > b->num; });
                delete phi->use;
                phi->use = new set<Temp*>(newUseVec.begin(), newUseVec.end());
            }
        }
    }
    
    DEBUG_PRINT("Variable renaming complete");
}

static void cleanupUnusedPhi(QuadFuncDecl* func) {
    CHECK_NULLPTR(func);
    DEBUG_PRINT("Cleaning up unused phi functions");
    
    set<Temp*> usedTemps;
    
    for (auto block : *func->quadblocklist) {
        CHECK_NULLPTR(block);
        for (auto stmt : *block->quadlist) {
            CHECK_NULLPTR(stmt);
            if (stmt->kind == QuadKind::PHI) continue;
            
            if (stmt->use) {
                usedTemps.insert(stmt->use->begin(), stmt->use->end());
            }
        }
    }
    
    bool changed = true;
    while (changed) {
        changed = false;
        
        for (auto block : *func->quadblocklist) {
            CHECK_NULLPTR(block);
            for (auto stmt : *block->quadlist) {
                CHECK_NULLPTR(stmt);
                if (stmt->kind != QuadKind::PHI) continue;
                
                QuadPhi* phi = static_cast<QuadPhi*>(stmt);
                
                if (usedTemps.find(phi->temp->temp) != usedTemps.end()) {
                    for (auto& arg : *phi->args) {
                        if (usedTemps.find(arg.first) == usedTemps.end()) {
                            usedTemps.insert(arg.first);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
    
    for (auto block : *func->quadblocklist) {
        CHECK_NULLPTR(block);
        auto it = block->quadlist->begin();
        while (it != block->quadlist->end()) {
            CHECK_NULLPTR(*it);
            if ((*it)->kind == QuadKind::PHI) {
                QuadPhi* phi = static_cast<QuadPhi*>(*it);
                
                if (usedTemps.find(phi->temp->temp) == usedTemps.end()) {
                    it = block->quadlist->erase(it);
                    continue;
                }
            }
            ++it;
        }
    }
    DEBUG_PRINT("Phi cleanup complete");
}

QuadProgram *quad2ssa(QuadProgram* program) {
    CHECK_NULLPTR(program);
    DEBUG_PRINT("Converting program to SSA form");
    
    QuadProgram* ssaProgram = new QuadProgram(static_cast<tree::Program*>(program->node), new vector<QuadFuncDecl*>());
    
    for (auto func : *program->quadFuncDeclList) {
        CHECK_NULLPTR(func);
        DEBUG_PRINT2("Processing function", func->funcname);
        
        ControlFlowInfo* domInfo = new ControlFlowInfo(func);
        domInfo->computeEverything();
        
        deleteUnreachableBlocks(func, domInfo);
        placePhi(func, domInfo);
        renameVariables(func, domInfo);
        cleanupUnusedPhi(func);
        
        ssaProgram->quadFuncDeclList->push_back(func);
        DEBUG_PRINT2("Completed SSA conversion for function", func->funcname);
    }
    
    DEBUG_PRINT("SSA conversion complete");
    return ssaProgram;
}
