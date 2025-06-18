#ifndef _AST2TREE_HH
#define _AST2TREE_HH

#include <stack>

#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "config.hh"
#include "semant.hh"
#include "temp.hh"
#include "tr_exp.hh"
#include "treep.hh"

#define WORDSIZE 4

using namespace std;
// using namespace fdmj;
// using namespace tree;

// forward declaration
class Class_table;
class Method_var_table;
class Patch_list;
class ASTToTreeVisitor;

tree::Program *ast2tree(fdmj::Program *prog, AST_Semant_Map *semant_map);
Class_table *generate_class_table(AST_Semant_Map *semant_map);
Method_var_table *generate_method_var_table(const string &class_name,
                                            const string &method_name,
                                            Name_Maps *nm, tree::Temp_map *temp_map);
tree::Eseq *createArrayBinaryOp(tree::Exp *leftArr, tree::Exp *rightArr,
                                const string &op, Temp_map *temp_map);
tree::Eseq *createArrayUnaryOp(tree::Exp *arrayExp, const string &op,
                               Temp_map *temp_map);
tree::Eseq *UnOp_Array(tree::Exp *srcArray, const string &opType, Temp_map *temp_map);
static tree::Exp *materializeIfNeeded(tree::Exp *expr, bool &needsWrap,
                                      vector<tree::Stm *> *stmts, Temp_map *temp_map,
                                      tree::Type ty);
static tree::Exp *buildBoundCheck(tree::Exp *idx, tree::Exp *arrAddr,
                                  Temp_map *temp_map);

// class table is to map each class var and method to an address offset
// this uses a "universal class" method, i.e., all the classes use the
// same class table, with all the possible vars and methods of all classes
// listed in the same record layout.
class Class_table {
public:
  map<string, int> var_pos_map;
  map<string, int> method_pos_map;
  int offset;

  Class_table() {
    var_pos_map = map<string, int>();
    method_pos_map = map<string, int>();
    offset = 0;
  };
  ~Class_table() {
    var_pos_map.clear();
    method_pos_map.clear();
  }
  int get_var_pos(string var_name) { return var_pos_map[var_name]; }
  int get_method_pos(string method_name) { return method_pos_map[method_name]; }

  void add_var(string vname) {
    if (var_pos_map.find(vname) == var_pos_map.end()) {
      var_pos_map[vname] = offset;
      offset += WORDSIZE;
    }
  }

  void add_method(string mname) {
    if (method_pos_map.find(mname) == method_pos_map.end()) {
      method_pos_map[mname] = offset;
      offset += WORDSIZE;
    }
  }

  int class_size() const { return offset; }
};

// For each method, there is a var table, including formal and local var.
//(if a method local has a conflict in var name with formal, then local var
// is used (ignore the formal))
// Hence, local var will override formal in the Method_var_table.
// Note: each local var and formal has a type as well (INT or PTR)
// The return of a method is also taken as a formal, with a special name
// _^return^_method_name.
class Method_var_table {
public:
  map<string, tree::Temp *> *var_temp_map;
  map<string, tree::Type> *var_type_map;
  Temp_map *temp_map = nullptr;
  string cname, mname;

  ~Method_var_table() {
    delete var_temp_map;
    delete var_type_map;
  }
  Method_var_table(string cname, string mname, Temp_map *temp_map)
      : cname(cname), mname(mname), temp_map(temp_map) {

    var_temp_map = new map<string, tree::Temp *>();
    var_type_map = new map<string, tree::Type>();
  };
  tree::Temp *get_var_temp(string var_name) {
    return var_temp_map->at(var_name);
  }
  tree::Type get_var_type(string var_name) {
    return var_type_map->at(var_name);
  }

  void set_var_temp(string var_name, tree::Temp *temp) {
    (*var_temp_map)[var_name] = temp;
  }
  void set_var_type(string var_name, tree::Type type) {
    (*var_type_map)[var_name] = type;
  }

  void add_var(string var_name, fdmj::TypeKind typeKind) {
    tree::Type type = tree::Type::INT;
    if (typeKind == TypeKind::INT) {
      ;
    } else {
      type = tree::Type::PTR;
    }
    if (var_temp_map->find(var_name) != var_temp_map->end()) {
      set_var_type(var_name, type);
    } else {
      set_var_temp(var_name, temp_map->newtemp());
      set_var_type(var_name, type);
    }
  }
};

class ASTToTreeVisitor : public fdmj::AST_Visitor {
public:
  tree::Tree *visit_tree_result = nullptr;
  Tr_Exp *expResult = nullptr;
  Temp_map *tempMap = new Temp_map();
  AST_Semant_Map *semantMap = nullptr;
  stack<pair<Label *, Label *>> loopLabels;

  Method_var_table *methodVarTable = nullptr;
  Class_table *classTable = nullptr;

  ~ASTToTreeVisitor() {
    delete methodVarTable;
    delete tempMap;
    delete classTable;
  }

  ASTToTreeVisitor(AST_Semant_Map *semantmap) : semantMap(semantmap) {
    classTable = generate_class_table(semantmap);
  }

  tree::Tree *getTree() {
    return visit_tree_result;
  } // return the tree from a single visit (program returns a single tree)

  void initializeMethodContext(const string &className,
                               const string &methodName) {
    auto *newTempMap = new Temp_map();
    auto *newVarTable = generate_method_var_table(
        className, methodName, semantMap->getNameMaps(), newTempMap);
    swap(tempMap, newTempMap);
    delete newTempMap;
    swap(methodVarTable, newVarTable);
    delete newVarTable;
  }

  inline void resetResults() {
    visit_tree_result = nullptr;
    expResult = nullptr;
  }

  /*
  T_tree* getTree() { return visit_result_node; }
  Temp_map* getTempMap() { return visitor_temp_map; }
  map<string, Temp_temp*>* getVariableMap() { return visitor_variable_map; }
  */
  void visit(fdmj::Program *node) override;
  void visit(fdmj::MainMethod *node) override;
  void visit(fdmj::ClassDecl *node) override;
  void visit(fdmj::Type *node) override;
  void visit(fdmj::VarDecl *node) override;
  void visit(fdmj::MethodDecl *node) override;
  void visit(fdmj::Formal *node) override;
  void visit(fdmj::Nested *node) override;
  void visit(fdmj::If *node) override;
  void visit(fdmj::While *node) override;
  void visit(fdmj::Assign *node) override;
  void visit(fdmj::CallStm *node) override;
  void visit(fdmj::Continue *node) override;
  void visit(fdmj::Break *node) override;
  void visit(fdmj::Return *node) override;
  void visit(fdmj::PutInt *node) override;
  void visit(fdmj::PutCh *node) override;
  void visit(fdmj::PutArray *node) override;
  void visit(fdmj::Starttime *node) override;
  void visit(fdmj::Stoptime *node) override;
  void visit(fdmj::BinaryOp *node) override;
  void visit(fdmj::UnaryOp *node) override;
  void visit(fdmj::ArrayExp *node) override;
  void visit(fdmj::CallExp *node) override;
  void visit(fdmj::ClassVar *node) override;
  void visit(fdmj::BoolExp *node) override;
  void visit(fdmj::This *node) override;
  void visit(fdmj::Length *node) override;
  void visit(fdmj::Esc *node) override;
  void visit(fdmj::GetInt *node) override;
  void visit(fdmj::GetCh *node) override;
  void visit(fdmj::GetArray *node) override;
  void visit(fdmj::IdExp *node) override;
  void visit(fdmj::OpExp *node) override;
  void visit(fdmj::IntExp *node) override;

  // Program
  void translateClassMethods(fdmj::Program *node,
                               vector<tree::FuncDecl *> *functionDeclList);
  tree::FuncDecl *translateMainMethod(fdmj::Program *node);

  // MainMethod
  vector<tree::Block *> *generate_mainmethod_body(fdmj::MainMethod *node,
                                                  tree::Label *&entryLabel);
  vector<tree::Stm *> *generate_local_var_decls();

  // VarDecl
  string get_var_cname(const string &className, const string &varName,
                       Name_Maps *nameMap);
  string get_method_cname(const string &className, const string &methodName,
                          Name_Maps *nameMap);
  void handle_int_decl(tree::Exp *destExpr, fdmj::VarDecl *decl);
  void handle_array_decl(tree::Exp *destExpr, fdmj::VarDecl *decl);
  void handle_class_decl(tree::Exp *destExpr, fdmj::VarDecl *decl);

  // MethodDecl
  vector<tree::Temp *> *generate_param_list(const string &className,
                                            const string &methodName);
  vector<tree::Block *> *
  generate_method_body(fdmj::MethodDecl *node, tree::Label *&entryLabel,
                       vector<tree::Label *> *exitLabels);
  tree::Type get_return_type(const string &methodName);

  // CallExp
  void buildMethodCall(fdmj::Exp *obj, fdmj::IdExp *name,
                       vector<fdmj::Exp *> *par);
  void generate_call_expr(fdmj::Exp *object, fdmj::IdExp *method,
                          const vector<fdmj::Exp *> *args);

  void emitVarDecl(tree::Exp *dst, fdmj::VarDecl *node);

  // Get*
  void emitSimpleInput(const string &funcName);

  // IdExp
  void handle_invalid_id(fdmj::IdExp *node);
  bool resolve_variable(fdmj::AST *node, const string &varName,
                        tree::Temp *&temp, tree::Type &type);
};

#endif
