#include <iostream>

#ifdef DEBUG
#define DEBUG_PRINT(msg) do { std::cerr << msg << std::endl; } while (0)
#define DEBUG_PRINT2(msg, val) do { std::cerr << msg << " " << val << std::endl; } while (0)
#else
#define DEBUG_PRINT(msg) do { } while (0)
#define DEBUG_PRINT2(msg, val) do { } while (0)
#endif

#define CHECK_NULLPTR(node) if (!node) { \
    std::cerr << "Error: Null pointer at " << __FILE__ << ":" << __LINE__ << std::endl; \
    return; \
}

#ifndef __AST_NAMEMAPS_HH__
#define __AST_NAMEMAPS_HH__

#include <map>
#include <set>
#include "ASTheader.hh"
#include "FDMJAST.hh"

using namespace std;
using namespace fdmj;

//this is for the maps of various names (things) in a program and their relationships
class Name_Maps {
private:
    //build all name maps to facilitate the easy access of the declarations of all names
    set<string> classes; //set of all class names
    map<string, string> classHierachy; //map class name to its direct ancestor class name
    set<pair<string, string>> methods; //set of class name and method name pairs
    map<pair<string, string>, VarDecl*> classVar; //map classname+varname to its declaration AST node (Type*)
    map<tuple<string, string, string>, VarDecl*> methodVar; //map classname + methodname + varname to VarDecl*
    map<tuple<string, string, string>, Formal*> methodFormal; //map classname+methodname+varname to Formal*
    map<pair<string, string>, vector<string>> methodFormalList; //map classname+methodname to formallist of vars
                        //The last is for the return type (pretending to be a Formal)
    map<pair<string, string>, Type*> methodType; // 存储方法返回类型

public:
    bool is_class(string class_name);
    bool add_class(string class_name); //return false if already exists
    bool add_class_hiearchy(string class_name, string parent_name); //return false if loop found
    set<string> get_ancestors(string class_name);
    bool is_method(string class_name, string method_name); 
    bool add_method(string class_name, string method_name);  //return false if already exists
    //MethodDecl* get_method(string method_name, string class_name);
    bool is_class_var(string class_name, string var_name);
    bool add_class_var(string class_name, string var_name, VarDecl* vd);
    VarDecl* get_class_var(string class_name, string var_name);
    bool is_method_var(string class_name, string method_name, string var_name);
    bool add_method_var(string class_name, string method_name, string var_name, VarDecl* vd);
    VarDecl* get_method_var(string class_name, string method_name, string var_name);
    bool is_method_formal(string class_name, string method_name, string var_name);
    bool add_method_formal(string class_name, string method_name, string var_name, Formal* f);
    Formal* get_method_formal(string class_name, string method_name, string var_name);
    bool add_method_formal_list(string class_name, string method_name, vector<string> vl);
    vector<Formal*>* get_method_formal_list(string class_name, string method_name);
    bool add_method_type(string class_name, string method_name, Type* type);
    Type* get_method_type(string class_name, string method_name);
    vector<string> get_method_formal_list_names(string class_name, string method_name);

    // 获取所有类名
    set<string> get_all_classes() { return classes; }
    // 获取父类名
    string get_parent_class(string class_name) {
        if (classHierachy.find(class_name) != classHierachy.end()) {
            return classHierachy[class_name];
        }
        return "";
    }

    void print();
    vector<string> get_class_var_names(string class_name);
    vector<string> get_class_method_names(string class_name);
    // 获取方法内局部变量名列表
    vector<string> get_method_var_names(string class_name, string method_name);
    bool inherit_var(string parent_class, string child_class, string var_name);
    bool check_method_signature(string class1, string method1, string class2, string method2);

    // 检查变量在特定作用域内是否重复定义
    bool is_var_duplicate_in_scope(string class_name, string method_name, string var_name);
    
    // 检查变量是否可在当前作用域访问（考虑作用域嵌套）
    bool is_var_accessible(string current_class, string current_method, string var_name);
};

//this visitor is to set up the name maps for the program

class AST_Name_Map_Visitor : public AST_Visitor {
private:
    Name_Maps *name_maps; //this is the map for all names in the program
    string current_class; // 当前正在访问的类名
    string current_method; // 当前正在访问的方法名
    bool in_formal; // 标记是否在处理形参
    vector<string> current_formal_list; // 当前方法的形参列表
    
public:
    AST_Name_Map_Visitor() {
        name_maps = new Name_Maps();
        current_class = "";
        current_method = "";
        in_formal = false;
    }
    Name_Maps* getNameMaps() { return name_maps; }

    void visit(Program* node) override;
    void visit(MainMethod* node) override;
    void visit(ClassDecl* node) override;
    void visit(Type *node) override;
    void visit(VarDecl* node) override;
    void visit(MethodDecl* node) override;
    void visit(Formal* node) override;
    void visit(Nested* node) override;
    void visit(If* node) override;
    void visit(While* node) override;
    void visit(Assign* node) override;
    void visit(CallStm* node) override;
    void visit(Continue* node) override;
    void visit(Break* node) override;
    void visit(Return* node) override;
    void visit(PutInt* node) override;
    void visit(PutCh* node) override;
    void visit(PutArray* node) override;
    void visit(Starttime* node) override;
    void visit(Stoptime* node) override;
    void visit(BinaryOp* node) override;
    void visit(UnaryOp* node) override;
    void visit(ArrayExp* node) override;
    void visit(CallExp* node) override;
    void visit(ClassVar* node) override;
    void visit(BoolExp* node) override;
    void visit(This* node) override;
    void visit(Length* node) override;
    void visit(Esc* node) override;
    void visit(GetInt* node) override;
    void visit(GetCh* node) override;
    void visit(GetArray* node) override;
    void visit(IdExp* node) override;
    void visit(OpExp* node) override;
    void visit(IntExp* node) override;
};

Name_Maps* makeNameMaps(Program* node);

#endif