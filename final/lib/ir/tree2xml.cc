#define DEBUG
#undef DEBUG

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "treep.hh"
#include "tinyxml2.hh"
#include "tree2xml.hh"

extern bool hw4_compat;

using namespace std;
using namespace tree;
using namespace tinyxml2;

#define FuncDeclList vector<FuncDecl*>

XMLDocument* tree2xml(Program* prog) {
#ifdef DEBUG
    cout << "in tree2xml::Converting IR to XML" << endl;
#endif
    Tree2XML v;
    v.doc = new XMLDocument();
    XMLDeclaration *decl = v.doc->NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\"");
    v.doc->InsertFirstChild(decl);
    v.visit_result = nullptr;
    if (prog != nullptr) {
        prog->accept(v);
    }
    else 
        v.visit_result = nullptr;
    if (v.visit_result != nullptr) 
        v.doc->InsertEndChild(v.visit_result);
    return v.doc;
}

void Tree2XML::visit(Program* node) {
#ifdef DEBUG
    cout << "Visiting Program" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    XMLElement* element = doc->NewElement("Program");
    if (node->funcdecllist != nullptr) {
        for (auto func : *node->funcdecllist) {
            func->accept(*this);
            if (visit_result != nullptr) element->InsertEndChild(visit_result);
        }
    }
    visit_result = element;
}

void Tree2XML::visit(FuncDecl* node) {
#ifdef DEBUG
    cout << "Visiting FunctionDeclaration" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    XMLElement* element = doc->NewElement("FunctionDeclaration");
    element->SetAttribute("name", node->name.c_str());
    element->SetAttribute("return_type", tree::typeToString(node->return_type).c_str());
    element->SetAttribute("last_temp", node->last_temp_num);
    element->SetAttribute("last_label", node->last_label_num);
    if (node->args != nullptr && node->args->size() > 0) {
        int i=0;
        for (auto arg : *node->args) {
            element->SetAttribute(("arg_temp_"+std::to_string(i)).c_str(), arg->name());
            i++;
        }
    }
    XMLElement *blocksElement = doc->NewElement("Blocks");
    if (node->blocks != nullptr) {
        for (auto block : *node->blocks) {
            block->accept(*this);
            if (visit_result != nullptr) blocksElement->InsertEndChild(visit_result);
        }
    }
    element->InsertEndChild(blocksElement);
    visit_result = element;
#ifdef DEBUG
    cout << "FunctionDeclaration visited" << endl;  
#endif
}

void Tree2XML::visit(Block *block) {
#ifdef DEBUG
    cout << "Visiting Block" << endl;
#endif
    if (block == nullptr) {
        visit_result = nullptr;
        return;
    }
    XMLElement* element = doc->NewElement("Block");
    element->SetAttribute("entry_label", block->entry_label->name());
    //XMLElement* exitLabelsElement = doc->NewElement("ExitLabels");
    if (block->exit_labels != nullptr) {
        int i=0;
        for (auto label : *block->exit_labels) {
            element->SetAttribute(("exit_label_"+to_string(i)).c_str(), label->name());
        }
    }
    const char* tag = hw4_compat ? "Statements" : "Sequence";
    XMLElement* stmsElement = doc->NewElement(tag);
    if (block->sl != nullptr) {
        for (auto stm : *block->sl) {
            stm->accept(*this);
            if (visit_result != nullptr) stmsElement->InsertEndChild(visit_result);
        }
    }
    element->InsertEndChild(stmsElement);
    visit_result = element;
}

void Tree2XML::visit(Jump* node) {
#ifdef DEBUG
    cout << "Visiting Jump" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    XMLElement* element = doc->NewElement("Jump");
    element->SetAttribute("label", node->label->name());
    visit_result = element;
}

void Tree2XML::visit(Cjump* node) {
#ifdef DEBUG
    cout << "Visiting CJump" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    XMLElement* element = doc->NewElement("CJump");
    element->SetAttribute("relop", node->relop.c_str());
    element->SetAttribute("true", node->t->name());
    element->SetAttribute("false", node->f->name());
    node->left->accept(*this);
    if (visit_result != nullptr) element->InsertEndChild(visit_result);
    node->right->accept(*this);
    if (visit_result != nullptr) element->InsertEndChild(visit_result);
    visit_result = element;
}

void Tree2XML::visit(Move* node) {
#ifdef DEBUG
    cout << "Visiting Move" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    XMLElement* element = doc->NewElement("Move");
    node->dst->accept(*this);
    if (visit_result != nullptr) element->InsertEndChild(visit_result);
    node->src->accept(*this);
    if (visit_result != nullptr) element->InsertEndChild(visit_result);
    visit_result = element;
}

void Tree2XML::visit(Seq* node) {
#ifdef DEBUG
    cout << "Visiting Sequence" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    tinyxml2::XMLElement* element = doc->NewElement("Sequence");
    if (node->sl == nullptr || node->sl->size() == 0) {
        //empty block
        visit_result = nullptr;
        return;
    }
    for (auto stm : *node->sl) {
        stm->accept(*this);
        if (visit_result != nullptr) element->InsertEndChild(visit_result);
    }
    visit_result = element;
}

void Tree2XML::visit(LabelStm* node) {
#ifdef DEBUG
    cout << "Visiting Label" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    XMLElement* element = doc->NewElement("Label");
    element->SetAttribute("label", node->label->name());
    visit_result = element;
}

void Tree2XML::visit(Return* node) {
#ifdef DEBUG
    cout << "Visiting Return" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    XMLElement* element = doc->NewElement("Return");
    node->exp->accept(*this);
    if (visit_result != nullptr) element->InsertEndChild(visit_result); 
    visit_result = element;
}

void Tree2XML::visit(Phi* node) {
#ifdef DEBUG
    cout << "Visiting Phi" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    XMLElement* element = doc->NewElement("Phi");
    element->SetAttribute("define", node->temp->name());
    int i = 0;
    for (auto arg : *node->args) {
        element->SetAttribute(("temp_"+to_string(i)).c_str(), arg.first->name());
        element->SetAttribute(("from_"+to_string(i)).c_str(), arg.second->name());
        i = i + 1;
    }
    visit_result = element;
}

void Tree2XML::visit(ExpStm* node) {
#ifdef DEBUG
    cout << "Visiting ExpressionStatement" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    if (node->exp == nullptr) {
        visit_result = nullptr;
        return;
    }
    XMLElement* element = doc->NewElement("ExpressionStatement");
    node->exp->accept(*this);
    if (visit_result != nullptr) element->InsertEndChild(visit_result);
    visit_result = element;
}

void Tree2XML::visit(Binop* node) {
#ifdef DEBUG
    cout << "Visiting BinaryOperation" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    tinyxml2::XMLElement* element = doc->NewElement("BinOp");
    element->SetAttribute("op", node->op.c_str());
    if (!hw4_compat)
        element->SetAttribute("type", node->type == Type::INT ? "INT" : "PTR");
    node->left->accept(*this);
    if (visit_result != nullptr) element->InsertEndChild(visit_result);
    node->right->accept(*this);
    if (visit_result != nullptr) element->InsertEndChild(visit_result);
    visit_result = element;
}

void Tree2XML::visit(Mem* node) {
#ifdef DEBUG
    cout << "Visiting Memory" << endl;
#endif
    XMLElement* element = doc->NewElement("Memory");
    if (!hw4_compat)
        element->SetAttribute("type", node->type == Type::INT ? "INT" : "PTR");
    node->mem->accept(*this);
    if (visit_result != nullptr) element->InsertEndChild(visit_result);
    visit_result = element;
}

void Tree2XML::visit(TempExp* node) {
#ifdef DEBUG
    cout << "Visiting Temp" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    XMLElement* element = doc->NewElement("Temp");
    element->SetAttribute("type", node->type == Type::INT ? "INT" : "PTR");
    element->SetAttribute("temp", node->temp->name());
    visit_result = element;
}

void Tree2XML::visit(Eseq* node) {
#ifdef DEBUG
    cout << "Visiting ESeq" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    } 
    XMLElement* element = doc->NewElement("ESeq");
    node->stm->accept(*this);
    if (visit_result != nullptr) element->InsertEndChild(visit_result);
    node->exp->accept(*this);
    if (visit_result != nullptr) element->InsertEndChild(visit_result);
    visit_result = element;
}

void Tree2XML::visit(Name* node) {
#ifdef DEBUG
    cout << "Visiting Name" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    XMLElement* element = doc->NewElement("Name");
    Label *label = node->name;
    String_Label *slabel = node->sname;
    string name;
    if (label != nullptr) { //then it's a string_label
        name = label->name();
    } else if (slabel != nullptr) {
        name = slabel->str();
    } else {
        cerr << "Error: Name has no label or string label!" << endl;
        visit_result = nullptr;
        return;
    }   
    element->SetAttribute("name", name.c_str());
    visit_result = element;
}

void Tree2XML::visit(Const* node) {
#ifdef DEBUG
    cout << "Visiting Const" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    XMLElement* element = doc->NewElement("Const");
    element->SetAttribute("value", node->constVal);
    visit_result = element;
}

void Tree2XML::visit(Call* node) {
#ifdef DEBUG
    cout << "Visiting Call" << endl;
#endif
    XMLElement* element = doc->NewElement("Call");
    element->SetAttribute("id", node->id.c_str());
    if (!hw4_compat)
        element->SetAttribute("type", node->type == Type::INT ? "INT" : "PTR");
    node->obj->accept(*this);
    if (visit_result != nullptr) element->InsertEndChild(visit_result);
    XMLElement* argsElement = doc->NewElement("Arguments");
    for (auto arg : *node->args) {
        arg->accept(*this);
        if (visit_result != nullptr) argsElement->InsertEndChild(visit_result);
    }
    element->InsertEndChild(argsElement);
    visit_result = element;
}

void Tree2XML::visit(ExtCall* node) {
#ifdef DEBUG
    cout << "Visiting ExtCall" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    tinyxml2::XMLElement* element = doc->NewElement("ExtCall");
    element->SetAttribute("extfun", node->extfun.c_str());
    if (!hw4_compat)
        element->SetAttribute("type", node->type == Type::INT ? "INT" : "PTR");
    tinyxml2::XMLElement* argsElement = doc->NewElement("Arguments");
    for (auto arg : *node->args) {
        arg->accept(*this);
        if (visit_result != nullptr) argsElement->InsertEndChild(visit_result);
    }
    element->InsertEndChild(argsElement);
    visit_result = element;
}

#define STRONG_MATCH

void DFS(const XMLNode* node, int depth = 0) {
    if (!node) return;

    // 打印当前节点信息（缩进表示层级）
    std::string indent(depth * 2, ' ');  // 缩进空格
    std::cout << indent << "Node: " << node->Value();

    // 处理元素节点（含属性）
    if (const XMLElement* elem = node->ToElement()) {
        // 打印属性
        for (const XMLAttribute* attr = elem->FirstAttribute(); attr; attr = attr->Next()) {
            std::cout << " | Attr: " << attr->Name() << "=" << attr->Value();
        }
    }

    // 打印文本内容（如果有）
    if (const XMLText* text = node->ToText()) {
        std::cout << " | Text: " << text->Value();
    }
    std::cout << std::endl;

    // 递归遍历子节点
    for (const XMLNode* child = node->FirstChild(); child; child = child->NextSibling()) {
        DFS(child, depth + 1);
    }
}

bool Comparison::add_temp_pair(int a, int b) {
    if (func_map[func_context].temp_a2b.find(a) != func_map[func_context].temp_a2b.end()) {
        if (func_map[func_context].temp_a2b[a] != b) {
            cout <<"In Func: "<<func_context<< "Temp " << a << " map to both " << func_map[func_context].temp_a2b[a] << " and " << b << "\n";
            return false;
        }
    } else func_map[func_context].temp_a2b[a] = b;
    return true;
}

bool Comparison::add_label_pair(int a, int b) {
    if (func_map[func_context].label_a2b.find(a) != func_map[func_context].label_a2b.end()) {
        if (func_map[func_context].label_a2b[a] != b) {
            cout <<"In Func: "<<func_context<< "Temp " << a << " map to both " << func_map[func_context].label_a2b[a] << " and " << b << "\n";
            return false;
        }
    } else func_map[func_context].label_a2b[a] = b;
    return true;
}

bool Comparison::dfs_compare(const XMLNode* node1, const XMLNode* node2) {
    if (!node1 || !node2) return false;

    //lab4 example can be different
    if(string(node1->ToElement()->Name())!=string(node2->ToElement()->Name())){
        cout << "node type not match\n";
        return false;
    }

    if(string(node1->ToElement()->Name())=="FunctionDeclaration"){
        func_context = string(node1->ToElement()->Attribute("name"));
        func_map[func_context].biggest_temp = 0;
        func_map[func_context].biggest_label = 0;

        func_map[func_context].last_temp=0;
        func_map[func_context].last_label=0;
    }
#ifdef STRONG_MATCH
    auto elem1=node1->ToElement();
    auto elem2=node2->ToElement();
    auto attr1=elem1->FirstAttribute(),attr2=elem2->FirstAttribute();
    for(;attr1&&attr2;attr1=attr1->Next(),attr2=attr2->Next()){
        if(string(attr1->Name())!=string(attr2->Name())){
            cout << "attribute name not match\n";
            return false;
        }
        set<string> ignore_list={
            "label",
            "entry_label",
            "temp",
            "last_temp",
            "last_label",
            "true",
            "false",
        };
        if(ignore_list.find(attr1->Name()) !=ignore_list.end())continue;
        auto name=string(attr1->Name());
        if(string(elem1->Name())=="FunctionDeclaration"&&name.size()>8&&name.substr(0,8)=="arg_temp")continue;
        if(string(attr1->Value())!=string(attr2->Value())){
            cout << "attribute value not match: "<<attr1->Name()<<" "<<attr1->Value()<<" "<<attr2->Value()<<"\n";
            return false;
        }
    }

    //lab4 example can be different
    if (attr1 || attr2) {
        cout << "node attr not match\n";
        return false;
    }
#endif

    vector<int> label1, temp1;
    if (const XMLElement* elem = node1->ToElement()) {
        for (const XMLAttribute* attr = elem->FirstAttribute(); attr; attr = attr->Next()) {
            string name = string(attr->Name());
            if (name == "label" || name == "entry_label" || name=="true" || name=="false") {
                int label_val = stoi(string(attr->Value()));
                label1.push_back(label_val);
                func_map[func_context].biggest_label = max(func_map[func_context].biggest_label, label_val);
            } else if (name == "temp" || (string(elem->Name())=="FunctionDeclaration"&&name.size()>8&&name.substr(0,8)=="arg_temp")) {
                int temp_val = stoi(string(attr->Value()));
                temp1.push_back(temp_val);
                func_map[func_context].biggest_temp = max(func_map[func_context].biggest_temp, temp_val);
            } else if (name == "last_temp") {
                func_map[func_context].last_temp = stoi(string(attr->Value()));
            } else if (name == "last_label") {
                func_map[func_context].last_label = stoi(string(attr->Value()));
            }
        }
    }

    vector<int> label2, temp2;
    if (const XMLElement* elem = node2->ToElement()) {
        for (const XMLAttribute* attr = elem->FirstAttribute(); attr; attr = attr->Next()) {
            string name = string(attr->Name());
            if (name == "label" || name == "entry_label" || name=="true" || name=="false") {
                label2.push_back(stoi(string(attr->Value())));
            } else if (name == "temp" || (string(elem->Name())=="FunctionDeclaration"&&name.size()>8&&name.substr(0,8)=="arg_temp")) {
                temp2.push_back(stoi(string(attr->Value())));
            }
        }
    }

    if (label1.size() != label2.size() || temp1.size() != temp2.size()) return false;
    for (int i = 0; i < label1.size(); i++) {
        if (!add_label_pair(label1[i], label2[i])) return false;
    }
    for (int i = 0; i < temp1.size(); i++) {
        if (!add_temp_pair(temp1[i], temp2[i])) return false;
    }

    const XMLNode* child1 = node1->FirstChild();
    const XMLNode* child2 = node2->FirstChild();
    for (; child1 && child2; child1 = child1->NextSibling(), child2 = child2->NextSibling()) {
        if (!dfs_compare(child1, child2)) return false;
    }
    if (child1 || child2) {
        cout << "tree struct not match\n";
        return false;
    }
    if (func_map[func_context].biggest_label > func_map[func_context].last_label) {
        cout << "label overflow\n";
        return false;
    }
    if (func_map[func_context].biggest_temp > func_map[func_context].last_temp) {
        cout << "temp overflow\n";
        return false;
    }
    return true;
}

bool Comparison::dfs_compare(string xml1, string xml2) {
    XMLDocument doc1, doc2;
    if (doc1.LoadFile(xml1.c_str()) != XML_SUCCESS) {
        cerr << "Error: " << xml1 << " not found" << endl;
        return false;
    }
    if (doc2.LoadFile(xml2.c_str()) != XML_SUCCESS) {
        cerr << "Error: " << xml2 << " not found" << endl;
        return false;
    }
    if (doc1.ErrorID() != XML_SUCCESS) {
        cerr << "Error: " << xml1 << " is not a valid XML file" << endl;
        return false;
    }
    if (doc2.ErrorID() != XML_SUCCESS) {
        cerr << "Error: " << xml2 << " is not a valid XML file" << endl;
        return false;
    }
    if (!dfs_compare(doc1.RootElement(), doc2.RootElement())) return false;
    return true;
}

void Comparison::print() {
    for(auto [name,mp]: func_map){
        cout<<"Func: "<<name<<"\n";
        for (const auto& pair : mp.temp_a2b) {
            cout << "pair: " << pair.first << " -> " << pair.second << "\n";
        }
        for (const auto& pair : mp.label_a2b) {
            cout << "label: " << pair.first << " -> " << pair.second << "\n";
        }
        cout << "last_temp: " << mp.last_temp << " vs biggest_temp: " << mp.biggest_temp << "\n";
        cout << "last_label: " << mp.last_label << " vs biggest_label: " << mp.biggest_label << "\n";  
    }
}

bool compare(string xml1, string xml2, bool print=false) {
    XMLDocument doc;
    if (doc.LoadFile(xml1.c_str()) != XML_SUCCESS) {
        cerr << "Standard irp not exists\n";
        return true;
    }

    Comparison a2b, b2a;
    if (print) cout << "----------a2b----------\n";
    bool res1 = a2b.dfs_compare(xml1, xml2);
    if (print) a2b.print();

    if (print) cout << "----------b2a----------\n";
    bool res2 = b2a.dfs_compare(xml2, xml1);
    if (print) b2a.print();

    return res1 && res2;
}