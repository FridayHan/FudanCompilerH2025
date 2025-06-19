#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include "config.hh"
#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "xml2ast.hh"
#include "ast2xml.hh"
#include "temp.hh"
#include "treep.hh"
#include "namemaps.hh"
#include "semant.hh"
#include "ast2tree.hh"
#include "tree2xml.hh"
#include "xml2tree.hh"
#include "canon.hh"
#include "quad.hh"
#include "tree2quad.hh"

using namespace std;
using namespace tinyxml2;

#define with_location_info false
// false means no location info in the AST XML files

fdmj::Program *prog();

#define ANSI_GREEN "\033[32m"
#define ANSI_RED "\033[31m"
#define ANSI_RESET "\033[0m"

void fmj2ast(const string &file) {
    string file_fmj = "fmj/" + file + ".fmj";
    string file_ast = "ast/" + file + ".2.ast";

    try {
        std::ifstream fmjfile(file_fmj);
        if (!fmjfile.is_open()) {
            throw runtime_error("Failed to open source file: " + file_fmj);
        }

        fdmj::Program *root = fdmjParser(fmjfile, false);
        if (root == nullptr) {
            throw runtime_error("AST is not valid!");
        }

        XMLDocument *x = ast2xml(root, nullptr, with_location_info, false);
        x->SaveFile(file_ast.c_str());
        if (x->Error()) {
            throw runtime_error("Failed to save AST to file: " + file_ast);
        }

        delete root;
        cout << ANSI_GREEN << "Successfully compiled " << file_fmj << " to " << file_ast << ANSI_RESET << endl;
    } catch (const exception &e) {
        cerr << ANSI_RED << "Error in compileFmjToAst: " << e.what() << ANSI_RESET << endl;
        exit(EXIT_FAILURE);
    }
}

void ast2semant(const string &file) {
    string file_ast = "ast/" + file + ".2.ast";
    string file_ast_semant = "semant/" + file + ".2-semant.ast";

    try {
        XMLDocument x;
        x.LoadFile(file_ast.c_str());
        if (x.Error()) {
            throw runtime_error("Failed to load AST file: " + file_ast);
        }

        fdmj::Program *root = xml2ast(x.FirstChildElement());
        if (root == nullptr) {
            throw runtime_error("AST from file is not valid!");
        }

        Name_Maps *name_maps = makeNameMaps(root);
        AST_Semant_Map *semant_map = semant_analyze(root);

        // XMLDocument *x_semant = ast2xml(root, semant_map, with_location_info, true);
        XMLDocument *x_semant = ast_with_maps2xml(root, name_maps, semant_map, with_location_info, true);
        x_semant->SaveFile(file_ast_semant.c_str());
        if (x_semant->Error()) {
            throw runtime_error("Failed to save semantic AST to file: " + file_ast_semant);
        }

        delete root;
        cout << ANSI_GREEN << "Successfully compiled " << file_ast << " to " << file_ast_semant << ANSI_RESET << endl;
    } catch (const exception &e) {
        cerr << ANSI_RED << "Error in compileAstToSemant: " << e.what() << ANSI_RESET << endl;
        exit(EXIT_FAILURE);
    }
}

void semant2ir(const string &file) {
    string file_ast = "semant/" + file + ".2-semant.ast";
    string file_ir = "ir/" + file + ".3.irp";

    try {
        // 加载语义分析后的 AST 文件
        XMLDocument x;
        x.LoadFile(file_ast.c_str());
        if (x.Error()) {
            throw runtime_error("Failed to load semantic AST file: " + file_ast);
        }

        // 将 XML 转换为 AST
        AST_Semant_Map *semant_map = new AST_Semant_Map();
        fdmj::Program *root = xml2ast(file_ast, &semant_map);
        if (root == nullptr) {
            throw runtime_error("Semantic AST from file is not valid!");
        }

        // 打印名称映射信息
        semant_map->getNameMaps()->print();

        // 将 AST 转换为 IR
        tree::Program *ir = ast2tree(root, semant_map);

        // 保存 IR 到文件
        XMLDocument *x_ir = tree2xml(ir);
        x_ir->SaveFile(file_ir.c_str());
        if (x_ir->Error()) {
            throw runtime_error("Failed to save IR to file: " + file_ir);
        }

        delete root;
        cout << ANSI_GREEN << "Successfully compiled " << file_ast << " to " << file_ir << ANSI_RESET << endl;
    } catch (const exception &e) {
        cerr << ANSI_RED << "Error in semant2ir: " << e.what() << ANSI_RESET << endl;
        exit(EXIT_FAILURE);
    }
}

void ir2quad(const string &file) {
    string file_irp = "ir/" + file + ".3.irp";
    string file_irp_canon = "ir/" + file + ".3-canon.irp";
    string file_quad = "quad/" + file + ".4.quad";

    try {
        // 读取 IR 文件
        cout << "Reading IR (XML) from: " << file_irp << endl;
        tree::Program *ir = xml2tree(file_irp);
        if (ir == nullptr) {
            throw runtime_error("Error: " + file_irp + " not found or IR ill-formed");
        }

        // 规范化 IR
        cout << "Canonicalization..." << endl;
        tree::Program *ir_canon = canon(ir);

        // 保存规范化后的 IR
        cout << "Writing Canonicalized IR to: " << file_irp_canon << endl;
        XMLDocument *doc = tree2xml(ir_canon);
        doc->SaveFile(file_irp_canon.c_str());
        if (doc->Error()) {
            throw runtime_error("Failed to save canonicalized IR to file: " + file_irp_canon);
        }

        // 转换 IR 为 Quad
        cout << "Converting IR to Quad..." << endl;
        QuadProgram *qd = tree2quad(ir_canon);
        if (qd == nullptr) {
            throw runtime_error("Error converting IR to Quad");
        }

        // 保存 Quad 到文件
        cout << "Writing Quad to: " << file_quad << endl;
        string temp_str;
        temp_str.reserve(50000);
        qd->print(temp_str, 0, true);
        ofstream qo(file_quad);
        if (!qo) {
            throw runtime_error("Error opening file: " + file_quad);
        }
        qo << temp_str;
        qo.flush();
        qo.close();

        cout << ANSI_GREEN << "Successfully compiled " << file_irp << " to " << file_quad << ANSI_RESET << endl;
    } catch (const exception &e) {
        cerr << ANSI_RED << "Error in ir2quad: " << e.what() << ANSI_RESET << endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <command> [filename]" << endl;
        return EXIT_FAILURE;
    }

    string command = argv[1];
    string file = argc > 2 ? argv[2] : "";

    if (command == "fmj2ast") {
        fmj2ast(file);
    } else if (command == "ast2semant") {
        ast2semant(file);
    } else if (command == "semant2ir") {
        semant2ir(file);
    } else if (command == "ir2quad") {
        ir2quad(file);
    } else {
        cerr << "Unknown command: " << command << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}