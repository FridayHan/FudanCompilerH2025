#define DEBUG
//#undef DEBUG

#include <iostream>
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

static void deleteUnreachableBlocks(QuadFuncDecl* func, ControlFlowInfo* domInfo) {
    // Use domInfo's elimination functionality which is already implemented
    domInfo->eliminateUnreachableBlocks();
}
static void placePhi(QuadFuncDecl* func, ControlFlowInfo* domInfo) {
    // Find all variables in the function
    set<Temp*> allVars;
    for (auto block : *func->quadblocklist) {
        for (auto stm : *block->quadlist) {
            if (stm->def) {
                allVars.insert(stm->def->begin(), stm->def->end());
            }
        }
    }

    // For each variable, compute the blocks where phi functions are needed
    for (auto var : allVars) {
        // Blocks where variable is defined
        set<int> defBlocks;
        for (auto block : *func->quadblocklist) {
            for (auto stm : *block->quadlist) {
                if (stm->def && stm->def->find(var) != stm->def->end()) {
                    defBlocks.insert(block->entry_label->num);
                    break;
                }
            }
        }

        // Compute dominance frontier closure
        set<int> phiBlocks;
        set<int> worklist(defBlocks.begin(), defBlocks.end());

        while (!worklist.empty()) {
            int block = *worklist.begin();
            worklist.erase(worklist.begin());

            for (auto dfBlock : domInfo->dominanceFrontiers[block]) {
                if (phiBlocks.find(dfBlock) == phiBlocks.end()) {
                    phiBlocks.insert(dfBlock);
                    // Add to worklist if the variable is defined in this block
                    if (defBlocks.find(dfBlock) != defBlocks.end()) {
                        worklist.insert(dfBlock);
                    }
                }
            }
        }

        // Insert phi functions for the variable
        for (auto blockNum : phiBlocks) {
            auto block = domInfo->labelToBlock[blockNum];
            if (!block) continue;

            // Create phi arguments (one for each predecessor)
            vector<pair<Temp*, Label*>>* phiArgs = new vector<pair<Temp*, Label*>>();
            for (auto predBlockNum : domInfo->predecessors[blockNum]) {
                auto predBlock = domInfo->labelToBlock[predBlockNum];
                if (!predBlock || !predBlock->entry_label) continue;
                
                phiArgs->push_back(make_pair(var, predBlock->entry_label));
            }

            // Create phi function
            if (!phiArgs->empty()) {
                set<Temp*>* def = new set<Temp*>();
                def->insert(var);
                set<Temp*>* use = new set<Temp*>();
                for (auto arg : *phiArgs) {
                    use->insert(arg.first);
                }

                // Create a TempExp* from the Temp* for the phi function
                TempExp* tempExp = new TempExp(Type::INT, var);
                QuadPhi* phi = new QuadPhi(nullptr, tempExp, phiArgs, def, use);
                
                // Insert at the beginning of the block after the label
                auto it = block->quadlist->begin();
                if (it != block->quadlist->end() && (*it)->kind == QuadKind::LABEL) {
                    ++it;
                }
                block->quadlist->insert(it, phi);
            }
        }
    }
}
static void renameVariables(QuadFuncDecl* func, ControlFlowInfo* domInfo) {
    // Map from original temp to its latest version
    map<Temp*, Temp*> currentVersion;
    
    // Map from temp to its list of versions
    map<Temp*, vector<Temp*>> tempVersions;
    
    // Function to get or create a new version of a temp
    auto getNewVersion = [&](Temp* temp) -> Temp* {
        if (tempVersions.find(temp) == tempVersions.end()) {
            tempVersions[temp] = vector<Temp*>();
        }
        
        // Create a new versioned temp
        int origNum = temp->num;
        int version = tempVersions[temp].size();
        int newNum = VersionedTemp::versionedTempNum(origNum, version);
        
        Temp* newTemp = new Temp(newNum);
        tempVersions[temp].push_back(newTemp);
        currentVersion[temp] = newTemp;
        
        return newTemp;
    };
    
    // Initialize versions for all temps
    for (auto block : *func->quadblocklist) {
        for (auto stm : *block->quadlist) {
            if (stm->def) {
                for (auto temp : *stm->def) {
                    if (tempVersions.find(temp) == tempVersions.end()) {
                        getNewVersion(temp);
                    }
                }
            }
        }
    }
    
    // Recursive function to rename variables in a block and its children
    function<void(int)> renameInBlock = [&](int blockNum) {
        QuadBlock* block = domInfo->labelToBlock[blockNum];
        if (!block) return;
        
        // Save old versions to restore later
        map<Temp*, Temp*> oldVersions = currentVersion;
        
        // Process statements in the block
        for (auto it = block->quadlist->begin(); it != block->quadlist->end(); ++it) {
            QuadStm* stm = *it;
            
            // Handle phi functions separately
            if (stm->kind == QuadKind::PHI) {
                QuadPhi* phi = static_cast<QuadPhi*>(stm);
                
                // Replace LHS with new version
                Temp* lhsTemp = phi->temp->temp;
                Temp* newLhsTemp = getNewVersion(lhsTemp);
                phi->temp = new TempExp(Type::INT, newLhsTemp);
                
                // Do not replace RHS of phi functions now, will be done later
                continue;
            }
            
            // Replace uses with current versions
            if (stm->use) {
                set<Temp*>* newUse = new set<Temp*>();
                for (auto temp : *stm->use) {
                    if (currentVersion.find(temp) != currentVersion.end()) {
                        newUse->insert(currentVersion[temp]);
                    } else {
                        newUse->insert(temp);
                    }
                }
                stm->use = newUse;
            }
            
            // Replace specific terms based on statement type
            if (stm->kind == QuadKind::MOVE) {
                QuadMove* move = static_cast<QuadMove*>(stm);
                if (move->src->kind == QuadTermKind::TEMP) {
                    TempExp* tempExp = move->src->get_temp();
                    Temp* temp = tempExp->temp;
                    if (currentVersion.find(temp) != currentVersion.end()) {
                        move->src = new QuadTerm(new TempExp(Type::INT, currentVersion[temp]));
                    }
                }
                
                // Replace destination with new version
                TempExp* dstTempExp = move->dst;
                Temp* dstTemp = dstTempExp->temp;
                Temp* newDstTemp = getNewVersion(dstTemp);
                move->dst = new TempExp(Type::INT, newDstTemp);
            }
            else if (stm->kind == QuadKind::LOAD) {
                QuadLoad* load = static_cast<QuadLoad*>(stm);
                if (load->src->kind == QuadTermKind::TEMP) {
                    TempExp* tempExp = load->src->get_temp();
                    Temp* temp = tempExp->temp;
                    if (currentVersion.find(temp) != currentVersion.end()) {
                        load->src = new QuadTerm(new TempExp(Type::INT, currentVersion[temp]));
                    }
                }
                
                // Replace destination with new version
                TempExp* dstTempExp = load->dst;
                Temp* dstTemp = dstTempExp->temp;
                Temp* newDstTemp = getNewVersion(dstTemp);
                load->dst = new TempExp(Type::INT, newDstTemp);
            }
            else if (stm->kind == QuadKind::STORE) {
                QuadStore* store = static_cast<QuadStore*>(stm);
                if (store->src->kind == QuadTermKind::TEMP) {
                    TempExp* tempExp = store->src->get_temp();
                    Temp* temp = tempExp->temp;
                    if (currentVersion.find(temp) != currentVersion.end()) {
                        store->src = new QuadTerm(new TempExp(Type::INT, currentVersion[temp]));
                    }
                }
                if (store->dst->kind == QuadTermKind::TEMP) {
                    TempExp* tempExp = store->dst->get_temp();
                    Temp* temp = tempExp->temp;
                    if (currentVersion.find(temp) != currentVersion.end()) {
                        store->dst = new QuadTerm(new TempExp(Type::INT, currentVersion[temp]));
                    }
                }
            }
            else if (stm->kind == QuadKind::MOVE_BINOP) {
                QuadMoveBinop* moveBinop = static_cast<QuadMoveBinop*>(stm);
                if (moveBinop->left->kind == QuadTermKind::TEMP) {
                    TempExp* tempExp = moveBinop->left->get_temp();
                    Temp* temp = tempExp->temp;
                    if (currentVersion.find(temp) != currentVersion.end()) {
                        moveBinop->left = new QuadTerm(new TempExp(Type::INT, currentVersion[temp]));
                    }
                }
                if (moveBinop->right->kind == QuadTermKind::TEMP) {
                    TempExp* tempExp = moveBinop->right->get_temp();
                    Temp* temp = tempExp->temp;
                    if (currentVersion.find(temp) != currentVersion.end()) {
                        moveBinop->right = new QuadTerm(new TempExp(Type::INT, currentVersion[temp]));
                    }
                }
                
                // Replace destination with new version
                TempExp* dstTempExp = moveBinop->dst;
                Temp* dstTemp = dstTempExp->temp;
                Temp* newDstTemp = getNewVersion(dstTemp);
                moveBinop->dst = new TempExp(Type::INT, newDstTemp);
            }
            else if (stm->kind == QuadKind::MOVE_EXTCALL) {
                QuadMoveExtCall* moveExtCall = static_cast<QuadMoveExtCall*>(stm);
                
                // Replace destination with new version
                TempExp* dstTempExp = moveExtCall->dst;
                Temp* dstTemp = dstTempExp->temp;
                Temp* newDstTemp = getNewVersion(dstTemp);
                moveExtCall->dst = new TempExp(Type::INT, newDstTemp);
            }
            else if (stm->kind == QuadKind::CJUMP) {
                QuadCJump* cjump = static_cast<QuadCJump*>(stm);
                if (cjump->left->kind == QuadTermKind::TEMP) {
                    TempExp* tempExp = cjump->left->get_temp();
                    Temp* temp = tempExp->temp;
                    if (currentVersion.find(temp) != currentVersion.end()) {
                        cjump->left = new QuadTerm(new TempExp(Type::INT, currentVersion[temp]));
                    }
                }
                if (cjump->right->kind == QuadTermKind::TEMP) {
                    TempExp* tempExp = cjump->right->get_temp();
                    Temp* temp = tempExp->temp;
                    if (currentVersion.find(temp) != currentVersion.end()) {
                        cjump->right = new QuadTerm(new TempExp(Type::INT, currentVersion[temp]));
                    }
                }
            }
            else if (stm->kind == QuadKind::RETURN) {
                QuadReturn* ret = static_cast<QuadReturn*>(stm);
                if (ret->value && ret->value->kind == QuadTermKind::TEMP) {
                    TempExp* tempExp = ret->value->get_temp();
                    Temp* temp = tempExp->temp;
                    if (currentVersion.find(temp) != currentVersion.end()) {
                        ret->value = new QuadTerm(new TempExp(Type::INT, currentVersion[temp]));
                    }
                }
            }
            else if (stm->kind == QuadKind::EXTCALL) {
                QuadExtCall* extCall = static_cast<QuadExtCall*>(stm);
                for (size_t i = 0; i < extCall->args->size(); ++i) {
                    QuadTerm* arg = extCall->args->at(i);
                    if (arg->kind == QuadTermKind::TEMP) {
                        TempExp* tempExp = arg->get_temp();
                        Temp* temp = tempExp->temp;
                        if (currentVersion.find(temp) != currentVersion.end()) {
                            (*extCall->args)[i] = new QuadTerm(new TempExp(Type::INT, currentVersion[temp]));
                        }
                    }
                }
            }
        }
        
        // Update phi function arguments in successor blocks
        for (auto succ : domInfo->successors[blockNum]) {
            QuadBlock* succBlock = domInfo->labelToBlock[succ];
            if (!succBlock) continue;
            
            for (auto stm : *succBlock->quadlist) {
                if (stm->kind != QuadKind::PHI) continue;
                
                QuadPhi* phi = static_cast<QuadPhi*>(stm);
                for (auto& arg : *phi->args) {
                    if (arg.second->num == block->entry_label->num) {
                        if (currentVersion.find(arg.first) != currentVersion.end()) {
                            arg.first = currentVersion[arg.first];
                        }
                    }
                }
            }
        }
        
        // Recursively process children in the dominator tree
        for (auto child : domInfo->domTree[blockNum]) {
            renameInBlock(child);
        }
        
        // Restore old versions
        currentVersion = oldVersions;
    };
    
    // Start renaming from the entry block
    renameInBlock(func->quadblocklist->at(0)->entry_label->num);
}
static void cleanupUnusedPhi(QuadFuncDecl* func) {
    // Find all variables that are actually used
    set<Temp*> usedTemps;
    
    // First pass: find all temps that are used outside of phi functions
    for (auto block : *func->quadblocklist) {
        for (auto stm : *block->quadlist) {
            if (stm->kind == QuadKind::PHI) continue;
            
            if (stm->use) {
                usedTemps.insert(stm->use->begin(), stm->use->end());
            }
        }
    }
    
    // Iteratively add temps that are used in phi functions if the phi result is used
    bool changed = true;
    while (changed) {
        changed = false;
        
        for (auto block : *func->quadblocklist) {
            for (auto stm : *block->quadlist) {
                if (stm->kind != QuadKind::PHI) continue;
                
                QuadPhi* phi = static_cast<QuadPhi*>(stm);
                
                // If the result of phi is used, add its inputs as used
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
    
    // Remove unused phi functions
    for (auto block : *func->quadblocklist) {
        auto it = block->quadlist->begin();
        while (it != block->quadlist->end()) {
            if ((*it)->kind == QuadKind::PHI) {
                QuadPhi* phi = static_cast<QuadPhi*>(*it);
                
                // Remove if the result is not used
                if (usedTemps.find(phi->temp->temp) == usedTemps.end()) {
                    it = block->quadlist->erase(it);
                    continue;
                }
            }
            ++it;
        }
    }
}

QuadProgram *quad2ssa(QuadProgram* program) {
    // Create a new QuadProgram to hold the SSA version
    QuadProgram* ssaProgram = new QuadProgram(static_cast<tree::Program*>(program->node), new vector<QuadFuncDecl*>());
    // Iterate through each function in the original program
    for (auto func : *program->quadFuncDeclList) {
        // Create a new ControlFlowInfo object for the function
        ControlFlowInfo* domInfo = new ControlFlowInfo(func);
        // Compute control flow information
        domInfo->computeEverything();
        
        // Eliminate unreachable blocks
        deleteUnreachableBlocks(func, domInfo);
        
        // Place phi functions
        placePhi(func, domInfo);
        
        // Rename variables
        renameVariables(func, domInfo);
        
        // Cleanup unused phi functions
        cleanupUnusedPhi(func);
        
        // Add the SSA version of the function to the new program
        ssaProgram->quadFuncDeclList->push_back(func);
    }
    return ssaProgram; //uncomment this line
    //return program; //delete this line
}
