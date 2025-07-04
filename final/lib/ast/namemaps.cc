#define DEBUG
#undef DEBUG

#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include "semant.hh"
#include "namemaps.hh"

using namespace std;
using namespace fdmj;

Name_Maps* makeNameMaps(Program* node) {
    AST_Name_Map_Visitor name_visitor;
    node->accept(name_visitor);
    Name_Maps* name_maps = name_visitor.getNameMaps();
    // name_maps->print(); // for DEBUG info
    return name_maps;
}

bool Name_Maps::is_class(string class_name) {
    return classes.find(class_name) != classes.end();
}

bool Name_Maps::add_class(string class_name) {
    if (is_class(class_name)) {
        return false;
    }
    classes.insert(class_name);
    return true;
}

set<string>* Name_Maps::get_class_list() {
    return &classes;
}

bool Name_Maps::detected_loop(map<string, string> classHierachy) {
    for (auto it = classHierachy.begin(); it != classHierachy.end(); it++) {
        string class_name = it->first;
        string parent_name = it->second;
        while (true) {
            if (classHierachy.find(parent_name) == classHierachy.end()) {
                break;
            }
            if (classHierachy[parent_name] == class_name) {
                return true;
            }
            parent_name = classHierachy[parent_name];
        }
    }
    return false;
}

bool Name_Maps::add_class_hiearchy(string class_name, string parent_name) {
    if (!Name_Maps::is_class(class_name) || !Name_Maps::is_class(parent_name)) {
        return false;
    }
    classHierachy[class_name] = parent_name;
    if (detected_loop(classHierachy)) {
        classHierachy.erase(class_name);
        return false;
    }
    return true;
}

vector<string>* Name_Maps::get_ancestors(string class_name) {
    vector<string>* ancestors = new vector<string>();
    if (classHierachy.find(class_name) == classHierachy.end()) {
        return ancestors;
    }
    //below works if no loop
    string parent_name = classHierachy[class_name];
    while (true) {
        ancestors->push_back(parent_name);
        if (classHierachy.find(parent_name) == classHierachy.end()) {
            break;
        }
        parent_name = classHierachy[parent_name];
    }
    return ancestors;
}

bool Name_Maps::is_method(string class_name, string method_name) {
    pair<string, string> p(class_name, method_name);
    return methods.find(p) != methods.end();
}

bool Name_Maps::add_method(string class_name, string method_name) {
    if (Name_Maps::is_method(class_name, method_name)) {
        return false;
    }
    methods.insert(pair<string, string>(class_name, method_name));
    return true;
}

set<string>* Name_Maps::get_method_list(string class_name) {
    set<string>* method_list = new set<string>();
    for (auto it = methods.begin(); it!= methods.end(); it++) {
        if (it->first == class_name) {
            method_list->insert(it->second);
        }
    }
    return method_list;
}

bool Name_Maps::is_class_var(string class_name, string var_name) {
    pair<string, string> p(class_name, var_name);
    return classVar.find(p) != classVar.end();
}

bool Name_Maps::add_class_var(string class_name, string var_name, VarDecl* varDecl) {
    pair<string, string> p(class_name, var_name);
    if (Name_Maps::is_class_var(class_name, var_name)) {
        return false;
    }
    classVar[p] = varDecl;
    return true;
}
VarDecl* Name_Maps::get_class_var(string class_name, string var_name) {
    if (!Name_Maps::is_class_var(class_name, var_name)) {
        return nullptr;
    }
    pair<string, string> p(class_name, var_name);
    return classVar[p];
}

set<string>* Name_Maps::get_class_var_list(string class_name) {
    set<string>* var_list = new set<string>();
    for (auto it = classVar.begin(); it!= classVar.end(); it++) {
        if (it->first.first == class_name) {
            var_list->insert(it->second->id->id); //VarDecl->id->id
        }
    }
    return var_list;
}

bool Name_Maps::is_method_var(string class_name, string method_name, string var_name) {
    tuple<string, string, string> t(class_name, method_name, var_name);
    return methodVar.find(t) != methodVar.end();
}

bool Name_Maps::add_method_var(string class_name, string method_name, string var_name, VarDecl* vd) {
    if (Name_Maps::is_method_var(class_name, method_name, var_name)) {
        return false;
    }
    methodVar[tuple<string, string, string>(class_name, method_name, var_name)] = vd;
    return true;
}

VarDecl* Name_Maps::get_method_var(string class_name, string method_name, string var_name) {
    if (!Name_Maps::is_method_var(class_name, method_name, var_name)) {
        return nullptr;
    }
    return methodVar[tuple<string, string, string>(class_name, method_name, var_name)];
}

set<string>* Name_Maps::get_method_var_list(string class_name, string method_name) {
    set<string>* var_list = new set<string>();
    for (auto it = methodVar.begin(); it != methodVar.end(); it++) {
        if (get<0>(it->first) == class_name && get<1>(it->first) == method_name) {
            var_list->insert(get<2>(it->first));
        }
    }
    return var_list;
}

bool Name_Maps::is_method_formal(string class_name, string method_name, string var_name) {
    tuple<string, string, string> t(class_name, method_name, var_name);
    return methodFormal.find(t) != methodFormal.end();
}

bool Name_Maps::add_method_formal(string class_name, string method_name, string var_name, Formal* f) {
    if (Name_Maps::is_method_formal(class_name, method_name, var_name)) {
        return false;
    }
    methodFormal[tuple<string, string, string>(class_name, method_name, var_name)] = f;
    return true;
}

Formal* Name_Maps::get_method_formal(string class_name, string method_name, string var_name) {
    if (!Name_Maps::is_method_formal(class_name, method_name, var_name)) {
    cout << "WHAT?! class=" << class_name << " method=" << method_name << " var=" << var_name << endl;
        return nullptr;
    }
    return methodFormal[tuple<string, string, string>(class_name, method_name, var_name)];
}

bool Name_Maps::add_method_formal_list(string class_name, string method_name, vector<string> vl) {
    if (!Name_Maps::is_method(class_name, method_name)) {
        return false;
    }
    pair<string, string> p(class_name, method_name);
    if (methodFormalList.find(p) != methodFormalList.end()) {
        vector<string> existing_formals = methodFormalList[p];
        for (string formal_name : vl) {
            if (find(existing_formals.begin(), existing_formals.end(), formal_name) != existing_formals.end()) {
                //cerr << "Error: Formal variable " << formal_name << " already exists in method " << method_name << " of class " << class_name << endl;
                return false;
            }
        }
    }
    methodFormalList[pair<string, string>(class_name, method_name)] = vl;
    return true;
}

vector<string> Name_Maps::get_method_formal_list(string class_name, string method_name) {
    vector<string> var_list;
    pair<string, string> p(class_name, method_name);
    if (methodFormalList.find(p) == methodFormalList.end()) {
        return var_list;
    }
    vector<string> vl = methodFormalList[pair<string, string>(class_name, method_name)];
    for (auto v : vl) {
        var_list.push_back(v);
    }
    return var_list;
}

bool Name_Maps::add_method_type(string class_name, string method_name, Type* type) {
    pair<string, string> key(class_name, method_name);
    if (methodType.find(key) != methodType.end()) {
        return false; // 已存在
    }
    methodType[key] = type;
    return true;
}

Type* Name_Maps::get_method_type(string class_name, string method_name) {
    pair<string, string> key(class_name, method_name);
    if (methodType.find(key) != methodType.end()) {
        return methodType[key];
    }
    return nullptr;
}

void Name_Maps::print() {
    cout << "Classes: ";
    for (auto c : classes) {
        cout << c << " ; ";
    }
    cout << endl;
    cout << "Class Hiearchy: ";
    for (auto it = classHierachy.begin(); it != classHierachy.end(); it++) {
        cout << it->first << "->" << it->second << " ; ";
    }
    cout << endl;
    cout << "Methods: ";
    for (auto m : methods) {
        cout << m.first << "->" << m.second << " ; ";
    }
    cout << endl;
    cout << "Class Variables: ";
    for (auto it = classVar.begin(); it != classVar.end(); it++) {
        VarDecl* vd = it->second;
        cout << (it->first).first << "->" << (it->first).second << " with type=" << 
            type_kind_string(vd->type->typeKind) << " ; ";
    }
    cout << endl;
    cout << "Method Variables: ";
    for (auto it = methodVar.begin(); it != methodVar.end(); it++) {
        VarDecl* vd = it->second;
        cout << get<0>(it->first) << "->" << get<1>(it->first) << "->" << get<2>(it->first) << 
            " with type=" << type_kind_string(vd->type->typeKind) <<  " ; ";
    }
    cout << endl;
    cout << "Method Formals: ";
    for (string c: *get_class_list()) {
        for (string m: *get_method_list(c)) {
            vector<string> fl = get_method_formal_list(c, m);
            for (string fv : fl) {
                Type *t = get_method_formal(c, m, fv)->type;
                cout << c << "->" << m << "->" << fv << " with type=" << type_kind_string(t->typeKind) << " ; ";
            }
        }
        if (get_method_list(c)->size()!=0)
            cout << endl;
    }
}

// 获取类的所有变量名
vector<string> Name_Maps::get_class_var_names(string class_name) {
    vector<string> var_names;
    for (auto it = classVar.begin(); it != classVar.end(); it++) {
        if ((it->first).first == class_name) {
            var_names.push_back((it->first).second);
        }
    }
    return var_names;
}

// 获取类的所有方法名
vector<string> Name_Maps::get_class_method_names(string class_name) {
    vector<string> method_names;
    for (auto it = methods.begin(); it != methods.end(); it++) {
        if (it->first == class_name) {
            method_names.push_back(it->second);
        }
    }
    return method_names;
}

// 获取指定方法的局部变量名列表
vector<string> Name_Maps::get_method_var_names(string class_name, string method_name) {
    vector<string> var_names;
    for (auto it = methodVar.begin(); it != methodVar.end(); ++it) {
        if (get<0>(it->first) == class_name && get<1>(it->first) == method_name) {
            var_names.push_back(get<2>(it->first));
        }
    }
    return var_names;
}

// 继承父类的变量到子类
bool Name_Maps::inherit_var(string parent_class, string child_class, string var_name) {
    if (!is_class_var(parent_class, var_name)) {
        return false;
    }
    
    // 检查子类是否已经有同名变量
    if (is_class_var(child_class, var_name)) {
        return false;
    }
    
    // 将父类变量复制到子类
    VarDecl* var_decl = get_class_var(parent_class, var_name);
    return add_class_var(child_class, var_name, var_decl);
}

// 检查方法签名是否一致并提供详细错误信息
bool Name_Maps::check_method_signature(string class1, string method1, string class2, string method2) {
    // 获取方法形参列表
    pair<string, string> key1(class1, method1);
    pair<string, string> key2(class2, method2);
    
    if (methodFormalList.find(key1) == methodFormalList.end() || 
        methodFormalList.find(key2) == methodFormalList.end()) {
        cerr << "Error: Cannot compare method signatures - one or both methods not found" << endl;
        return false;
    }
    
    // 比较形参列表长度
    vector<string> params1 = methodFormalList[key1];
    vector<string> params2 = methodFormalList[key2];
    
    // 减去返回值参数（总是最后一个）
    int size1 = params1.size() - 1;
    int size2 = params2.size() - 1;
    
    if (size1 != size2) {
        cerr << "Error: Method '" << method2 << "' in class '" << class2 
             << "' has " << size2 << " parameters, but inherited method from class '" 
             << class1 << "' has " << size1 << " parameters" << endl;
        return false;
    }
    
    // 比较每个参数的类型
    for (int i = 0; i < size1; i++) {
        Formal* formal1 = get_method_formal(class1, method1, params1[i]);
        Formal* formal2 = get_method_formal(class2, method2, params2[i]);
        
        if (!formal1 || !formal2) {
            cerr << "Error: Cannot access formal parameter information for methods comparison" << endl;
            return false;
        }
        
        if (formal1->type->typeKind != formal2->type->typeKind) {
            cerr << "Error: Parameter " << (i+1) << " of method '" << method2 
                 << "' in class '" << class2 << "' has type '" 
                 << type_kind_string(formal2->type->typeKind) 
                 << "', but in parent class '" << class1 
                 << "' it has type '" << type_kind_string(formal1->type->typeKind) << "'" << endl;
            return false;
        }
        
        // 如果是类类型，还需要比较类名
        if (formal1->type->typeKind == TypeKind::CLASS) {
            string class_name1 = formal1->type->cid ? formal1->type->cid->id : "";
            string class_name2 = formal2->type->cid ? formal2->type->cid->id : "";
            if (class_name1 != class_name2) {
                cerr << "Error: Parameter " << (i+1) << " of method '" << method2 
                     << "' in class '" << class2 << "' has class type '" << class_name2 
                     << "', but in parent class '" << class1 
                     << "' it has class type '" << class_name1 << "'" << endl;
                return false;
            }
        }
        
        // 如果是数组类型，还需要比较维度
        if (formal1->type->typeKind == TypeKind::ARRAY) {
            int arity1 = formal1->type->arity ? formal1->type->arity->val : 0;
            int arity2 = formal2->type->arity ? formal2->type->arity->val : 0;
            if (arity1 != arity2) {
                cerr << "Error: Parameter " << (i+1) << " of method '" << method2 
                     << "' in class '" << class2 << "' has array dimension " << arity2 
                     << ", but in parent class '" << class1 
                     << "' it has dimension " << arity1 << endl;
                return false;
            }
        }
    }
    
    // 比较返回值类型
    Type* ret_type1 = get_method_type(class1, method1);
    Type* ret_type2 = get_method_type(class2, method2);
    
    if (!ret_type1 || !ret_type2) {
        cerr << "Error: Cannot access return type information for methods comparison" << endl;
        if (!ret_type1) cerr << "  - Missing return type for " << class1 << "::" << method1 << endl;
        if (!ret_type2) cerr << "  - Missing return type for " << class2 << "::" << method2 << endl;
        return false;
    }
    
    if (ret_type1->typeKind != ret_type2->typeKind) {
        cerr << "Error: Return type of method '" << method2 
             << "' in class '" << class2 << "' is '" 
             << type_kind_string(ret_type2->typeKind) 
             << "', but in parent class '" << class1 
             << "' it is '" << type_kind_string(ret_type1->typeKind) << "'" << endl;
        return false;
    }
    
    // 如果返回值是类类型，还需要比较类名
    if (ret_type1->typeKind == TypeKind::CLASS) {
        string class_name1 = ret_type1->cid ? ret_type1->cid->id : "";
        string class_name2 = ret_type2->cid ? ret_type2->cid->id : "";
        if (class_name1 != class_name2) {
            cerr << "Error: Return type of method '" << method2 
                 << "' in class '" << class2 << "' is class '" << class_name2 
                 << "', but in parent class '" << class1 
                 << "' it is class '" << class_name1 << "'" << endl;
            return false;
        }
    }
    
    // 如果返回值是数组类型，还需要比较维度
    if (ret_type1->typeKind == TypeKind::ARRAY) {
        int arity1 = ret_type1->arity ? ret_type1->arity->val : 0;
        int arity2 = ret_type2->arity ? ret_type2->arity->val : 0;
        if (arity1 != arity2) {
            cerr << "Error: Return type of method '" << method2 
                 << "' in class '" << class2 << "' has array dimension " << arity2 
                 << ", but in parent class '" << class1 
                 << "' it has dimension " << arity1 << endl;
            return false;
        }
    }
    
    return true;
}

// 检查变量在特定作用域内是否重复定义
bool Name_Maps::is_var_duplicate_in_scope(string class_name, string method_name, string var_name) {
    if (method_name.empty()) {
        // 类作用域检查
        if (is_class_var(class_name, var_name)) {
            return true;
        }
    } else {
        // 方法作用域检查
        
        // 1. 检查是否与方法内的其他局部变量重复
        if (is_method_var(class_name, method_name, var_name)) {
            return true;
        }
        
        // 2. 检查是否与方法参数重复
        if (is_method_formal(class_name, method_name, var_name)) {
            return true;
        }
    }
    
    return false;
}

// 检查变量是否可在当前作用域访问（考虑作用域嵌套）
bool Name_Maps::is_var_accessible(string current_class, string current_method, string var_name) {
    // 1. 首先检查方法的局部变量
    if (!current_method.empty() && is_method_var(current_class, current_method, var_name)) {
        return true;
    }
    
    // 2. 检查方法参数
    if (!current_method.empty() && is_method_formal(current_class, current_method, var_name)) {
        return true;
    }
    
    // 3. 检查当前类的成员变量
    if (!current_class.empty() && is_class_var(current_class, var_name)) {
        return true;
    }
    
    // 4. 检查父类的成员变量
    if (!current_class.empty()) {
        vector<string>* ancestors = get_ancestors(current_class);
        for (const auto& ancestor : *ancestors) {
            if (is_class_var(ancestor, var_name)) {
                return true;
            }
        }
    }
    
    // 5. 检查_^main^_的变量（全局变量）
    if (is_class_var("_^main^_", var_name)) {
        return true;
    }
    
    return false;
}