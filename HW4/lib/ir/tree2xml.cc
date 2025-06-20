#define DEBUG
#undef DEBUG

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "treep.hh"
#include "tinyxml2.hh"
#include "tree2xml.hh"

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
    if (node->args != nullptr) {
        for (auto arg : *node->args) {
            element->SetAttribute("arg_temp", arg->name());
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
        for (auto label : *block->exit_labels) {
            element->SetAttribute("exit_label", label->name());
        }
    }
    XMLElement* stmsElement = doc->NewElement("Statements");
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
    for (auto arg : *node->args) {
        element->SetAttribute("arg", arg.first->name());
        element->SetAttribute("from", arg.second->name());
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
    element->SetAttribute("name", node->name->name());
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
    tinyxml2::XMLElement* argsElement = doc->NewElement("Arguments");
    for (auto arg : *node->args) {
        arg->accept(*this);
        if (visit_result != nullptr) argsElement->InsertEndChild(visit_result);
    }
    element->InsertEndChild(argsElement);
    visit_result = element;
}

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
    if (temp_a2b.find(a) != temp_a2b.end()) {
        if (temp_a2b[a] != b) {
            cout << "Temp " << a << " map to both " << temp_a2b[a] << " and " << b << "\n";
            return false;
        }
    } else temp_a2b[a] = b;
    return true;
}

bool Comparison::add_label_pair(int a, int b) {
    if (label_a2b.find(a) != label_a2b.end()) {
        if (label_a2b[a] != b) {
            cout << "Temp " << a << " map to both " << label_a2b[a] << " and " << b << "\n";
            return false;
        }
    } else label_a2b[a] = b;
    return true;
}

bool Comparison::dfs_compare(const XMLNode* node1, const XMLNode* node2) {
    if (!node1 || !node2) return false;

    vector<int> label1, temp1;
    if (const XMLElement* elem = node1->ToElement()) {
        for (const XMLAttribute* attr = elem->FirstAttribute(); attr; attr = attr->Next()) {
            string name = string(attr->Name());
            if (name == "label" || name == "entry_label") {
                int label_val = stoi(string(attr->Value()));
                label1.push_back(label_val);
                biggest_label = max(biggest_label, label_val);
            } else if (name == "temp") {
                int temp_val = stoi(string(attr->Value()));
                temp1.push_back(temp_val);
                biggest_temp = max(biggest_temp, temp_val);
            } else if (name == "last_temp") {
                last_temp = stoi(string(attr->Value()));
            } else if (name == "last_label") {
                last_label = stoi(string(attr->Value()));
            }
        }
    }

    vector<int> label2, temp2;
    if (const XMLElement* elem = node2->ToElement()) {
        for (const XMLAttribute* attr = elem->FirstAttribute(); attr; attr = attr->Next()) {
            string name = string(attr->Name());
            if (name == "label" || name == "entry_label") {
                label2.push_back(stoi(string(attr->Value())));
            } else if (name == "temp") {
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
    biggest_temp = biggest_label = 0;
    last_temp = last_label = 0;
    if (!dfs_compare(doc1.RootElement(), doc2.RootElement())) return false;
    if (biggest_label > last_label) {
        cout << "label overflow\n";
        return false;
    }
    if (biggest_temp > last_temp) {
        cout << "temp overflow\n";
        return false;
    }
    return true;
}

void Comparison::print() {
    for (const auto& pair : temp_a2b) {
        cout << "pair: " << pair.first << " -> " << pair.second << "\n";
    }
    for (const auto& pair : label_a2b) {
        cout << "label: " << pair.first << " -> " << pair.second << "\n";
    }
    cout << "last_temp: " << last_temp << " vs biggest_temp: " << biggest_temp << "\n";
    cout << "last_label: " << last_label << " vs biggest_label: " << biggest_label << "\n";    
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