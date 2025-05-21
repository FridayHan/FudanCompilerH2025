#define DEBUG
#undef DEBUG

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "temp.hh"
#include "ig.hh"
#include "coloring.hh"

bool isAnEdge(map<int, set<int>>& graph, int src, int dst) {
    if (graph.find(src) == graph.end()) return false; //src not in the graph
    if (graph[src].find(dst) == graph[src].end()) return false; //dst not in the graph
    return true; //edge exists
}

//return true if any node is removed
// 从图中移除度数小于k的非机器寄存器节点
bool Coloring::simplify() {
    bool changed = false;
    
    // 找到所有非机器寄存器且度数小于k的节点
    vector<int> toSimplify;
    for (auto &p : graph) {
        int node = p.first;
        // 跳过机器寄存器和参与move指令的节点
        if (isMachineReg(node) || isMove(node)) continue;
        // 如果节点的度数小于k，可以安全移除
        if (p.second.size() < k) {
            toSimplify.push_back(node);
        }
    }
    
    // 移除这些节点并将其压入栈
    for (int node : toSimplify) {
        simplifiedNodes.push(node);
        eraseNode(node);
        changed = true;
    }
    
    return changed;
}

//return true if changed anything, false otherwise
// 合并由move指令连接的节点
bool Coloring::coalesce() {
    if (movePairs.empty()) return false;
    
    bool changed = false;
    vector<pair<int, int>> movesToProcess(movePairs.begin(), movePairs.end());
    
    for (auto move : movesToProcess) {
        int u = move.first;
        int v = move.second;
        
        // 如果其中一个节点已不在图中，跳过
        if (graph.find(u) == graph.end() || graph.find(v) == graph.end()) {
            movePairs.erase(move);
            continue;
        }
        
        // 如果u和v已经相邻，不能合并
        if (isAnEdge(graph, u, v)) {
            continue;
        }
        
        // 确定合并方向：优先保留机器寄存器
        int keepNode, removeNode;
        if (isMachineReg(u)) {
            keepNode = u;
            removeNode = v;
        } else if (isMachineReg(v)) {
            keepNode = v;
            removeNode = u;
        } else {
            // 如果都不是机器寄存器，保留度数更高的节点
            keepNode = (graph[u].size() >= graph[v].size()) ? u : v;
            removeNode = (keepNode == u) ? v : u;
        }
        
        // 收集两个节点的所有邻居
        set<int> allNeighbors;
        for (int n : graph[keepNode]) allNeighbors.insert(n);
        for (int n : graph[removeNode]) {
            if (n != keepNode) allNeighbors.insert(n);
        }
        
        // 检查合并后是否会导致度数超过k
        bool isSafe = true;
        for (int neighbor : allNeighbors) {
            if (graph[neighbor].size() >= k) {
                isSafe = false;
                break;
            }
        }
        
        if (isSafe) {
            // 将removeNode的所有边添加到keepNode
            for (int neighbor : graph[removeNode]) {
                if (neighbor != keepNode) {
                    addEdge(keepNode, neighbor);
                }
            }
            
            // 记录合并信息
            if (coalescedMoves.find(keepNode) == coalescedMoves.end()) {
                coalescedMoves[keepNode] = set<int>();
            }
            coalescedMoves[keepNode].insert(removeNode);
            
            // 从图中移除节点
            eraseNode(removeNode);
            
            // 更新move信息
            movePairs.erase(move);
            
            changed = true;
            break; // 一次只合并一对，然后重新评估
        }
    }
    
    return changed;
}

//freeze the moves that are not coalesced
//return true if changed anything, false otherwise
// 冻结不能合并的move指令
bool Coloring::freeze() {
    if (movePairs.empty()) return false;
    
    bool changed = false;
    
    // 找到所有参与move且度数小于k的节点
    vector<int> freezeCandidates;
    for (auto it = graph.begin(); it != graph.end(); ++it) {
        int node = it->first;
        if (isMachineReg(node)) continue;
        if (isMove(node) && it->second.size() < k) {
            freezeCandidates.push_back(node);
        }
    }
    
    if (freezeCandidates.empty()) return false;
    
    // 选择第一个候选节点进行冻结
    int nodeToFreeze = freezeCandidates[0];
    
    // 找到与该节点相关的所有move，并从movePairs中移除
    vector<pair<int, int>> movesToRemove;
    for (const auto& move : movePairs) {
        if (move.first == nodeToFreeze || move.second == nodeToFreeze) {
            movesToRemove.push_back(move);
        }
    }
    
    for (const auto& move : movesToRemove) {
        movePairs.erase(move);
        changed = true;
    }
    
    return changed;
}

//This is a soft spill: we just remove the node from the graph and add it to the simplified nodes
//as if nothing happened. The actual spill happens when select&coloring
// 软溢出：从图中选择一个度数高的节点移除
bool Coloring::spill() {
    // 如果图为空，返回false
    if (graph.empty()) return false;
    
    // 找到图中度数最高的非机器寄存器节点
    int maxDegree = -1;
    int spillNode = -1;
    
    for (const auto& p : graph) {
        int node = p.first;
        if (isMachineReg(node)) continue;
        
        int degree = p.second.size();
        if (degree > maxDegree) {
            maxDegree = degree;
            spillNode = node;
        }
    }
    
    // 如果没有找到可溢出的节点，返回false
    if (spillNode == -1) return false;
    
    // 将该节点加入到simplifiedNodes栈中
    simplifiedNodes.push(spillNode);
    eraseNode(spillNode);
    
    return true;
}

//now try to select the registers for the nodes
//finally check the validity of the coloring
// 为节点选择颜色（寄存器）
bool Coloring::select() {
    colors.clear(); // 先清除所有现有的颜色分配
    
    // 为机器寄存器分配固定颜色
    for (int i = 0; i < k; i++) {
        if (graph.find(i) != graph.end() || ig->graph.find(i) != ig->graph.end()) {
            colors[i] = i;
        }
    }
    
    // 为所有合并的节点分配颜色
    for (const auto& p : coalescedMoves) {
        int mainNode = p.first;
        if (colors.find(mainNode) != colors.end()) {
            for (int mergedNode : p.second) {
                colors[mergedNode] = colors[mainNode];
            }
        }
    }
    
    // 从栈中弹出节点，为它们分配颜色
    stack<int> tempStack = simplifiedNodes; // 创建临时栈副本
    while (!tempStack.empty()) {
        int node = tempStack.top();
        tempStack.pop();
        
        // 检查节点是否已被合并
        bool isCoalesced = false;
        for (const auto& p : coalescedMoves) {
            if (p.second.find(node) != p.second.end()) {
                isCoalesced = true;
                break;
            }
        }
        if (isCoalesced) continue;
        
        // 获取邻居已使用的颜色
        set<int> usedColors;
        for (const auto& p : ig->graph) {
            if (isAnEdge(ig->graph, node, p.first) && colors.find(p.first) != colors.end()) {
                usedColors.insert(colors[p.first]);
            }
        }
        
        // 寻找可用颜色
        int chosenColor = -1;
        for (int color = 0; color < k; color++) {
            if (usedColors.find(color) == usedColors.end()) {
                chosenColor = color;
                break;
            }
        }
        
        // 如果找到可用颜色，分配它
        if (chosenColor != -1) {
            colors[node] = chosenColor;
        } else {
            // 否则标记为溢出
            spilled.insert(node);
        }
    }
    
    // 检查所有图中的节点是否都已着色或标记为溢出
    for (const auto& p : ig->graph) {
        int node = p.first;
        if (!isMachineReg(node) && colors.find(node) == colors.end() && spilled.find(node) == spilled.end()) {
            // 节点既未着色也未标记为溢出，尝试为其分配颜色
            set<int> usedColors;
            for (int neighbor : p.second) {
                if (colors.find(neighbor) != colors.end()) {
                    usedColors.insert(colors[neighbor]);
                }
            }
            
            int color = 0;
            while (usedColors.find(color) != usedColors.end() && color < k) {
                color++;
            }
            
            if (color < k) {
                colors[node] = color;
            } else {
                spilled.insert(node);
            }
        }
    }
    
    // 验证着色是否有效
    return checkColoring();
}
