#ifndef _TREE2XML_HH
#define _TREE2XML_HH

#include "treep.hh"
#include "tinyxml2.hh"

// Compatibility flag. When true, Tree2XML will emit the same XML format
// as used in HW4 (e.g. omit certain type attributes and use
// <Statements> tags). This flag is set during AST to IR translation based
// on whether the program uses array or class features.
extern bool hw4_compat;

using namespace std;
using namespace tree;
using namespace tinyxml2;

class Tree2XML : public Visitor {
public:
    XMLDocument* doc;
    XMLElement* visit_result;

    void visit(tree::Program *prog) override;
    void visit(tree::FuncDecl *func) override;
    void visit(tree::Block *block) override;
    void visit(tree::Jump *jump) override;
    void visit(tree::Cjump *cjump) override;
    void visit(tree::Move *move) override;
    void visit(tree::Seq *seq) override;
    void visit(tree::LabelStm *labelstm) override;
    void visit(tree::Return *ret) override;
    void visit(tree::Phi *phi) override;
    void visit(tree::ExpStm *exp) override;
    void visit(tree::Binop *binop) override;
    void visit(tree::Mem *mem) override;
    void visit(tree::TempExp *tempexp) override;
    void visit(tree::Eseq *eseq) override;
    void visit(tree::Name *name) override;
    void visit(tree::Const *const) override;
    void visit(tree::Call *call) override;
    void visit(tree::ExtCall *extcall) override;
};

XMLDocument* tree2xml(tree::Program* prog);

struct FuncTempMap{
    map<int,int> temp_a2b,label_a2b;
    int last_temp,last_label;
    int biggest_temp,biggest_label;
};

class Comparison {
public:
    bool dfs_compare(string xml1, string xml2);
    void print();
private:
    // a->b是不是单射
    bool dfs_compare(const XMLNode* node1, const XMLNode* node2);
    bool add_temp_pair(int a, int b);
    bool add_label_pair(int a, int b);

    string func_context;
    map<string,FuncTempMap> func_map;
};

bool compare(string xml1, string xml2, bool print);

#endif