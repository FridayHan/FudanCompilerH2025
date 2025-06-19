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

        XMLDocument *x_semant = ast2xml(root, semant_map, with_location_info, true);
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

void ast2ir(const string &file) {
    try {
        string file_ast = "semant/" + file + ".2-semant.ast";
        string file_ir = "ir/" + file + ".3.irp";

        cout << "------Reading AST from : " << file_ast << "------------" << endl;
        AST_Semant_Map *semant_map = new AST_Semant_Map();
        fdmj::Program *root = xml2ast(file_ast, &semant_map);
        if (root == nullptr) {
            cerr << "Error reading AST from: " << file_ast << endl;
            return;
        }
        
        semant_map->getNameMaps()->print();
        cout << "------Converting AST to IR------" << endl;
        Compiler_Config::print_config();
        tree::Program *ir = ast2tree(root, semant_map);

        cout << "------Saving IR (XML) to: " << file_ir << "------------" << endl;
        XMLDocument *x = tree2xml(ir);
        x->SaveFile(file_ir.c_str());
        if (x->Error()) {
            throw runtime_error("Failed to save IR to file: " + file_ir);
        }

        cout << "-----Done---" << endl;
    } catch (const exception &e) {
        cerr << ANSI_RED << "Error in ast2ir: " << e.what() << ANSI_RESET << endl;
    } catch (...) {
        cerr << ANSI_RED << "Unknown error occurred in ast2ir" << ANSI_RESET << endl;
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
    } else if (command == "ast2ir") {
        ast2ir(file);
    } else {
        cerr << "Unknown command: " << command << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}