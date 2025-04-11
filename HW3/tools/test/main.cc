#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "ast2xml.hh"
#include "xml2ast.hh"
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <cassert>

using namespace std;
using namespace fdmj;
using namespace tinyxml2;

#define with_location_info false
// false means no location info in the AST XML files

Program *prog();

int main(int argc, const char *argv[]) {
    string file;

    const bool debug = argc > 1 && strcmp(argv[1], "--debug") == 0;

    if ((!debug && argc != 2) || (debug && argc != 3)) {
        cerr << "Usage: " << argv[0] << " [--debug] filename" << endl;
        return EXIT_FAILURE;
    }

    file = argv[argc - 1];

    // boilerplate output filenames (used throughout the compiler pipeline)
    string file_fmj = file + ".fmj"; // input source file
    string file_ast = file + ".1.ast";        // ast in xml
    string file_ast_semant = file + ".2.semant.ast";

    ifstream fmjfile(file_fmj);
    Program *root = fdmjParser(fmjfile, false); // false means no debug info from parser
    if (root == nullptr) {
        cout << "AST is not valid!" << endl;
        return EXIT_FAILURE;
    }

    XMLDocument *x = nullptr;

    // ast
    x = ast2xml(root, nullptr, with_location_info, false); // no semant info yet
    assert(!x->Error() && "AST is not valid!");
    x->SaveFile(file_ast.c_str());
    
    // ast with semant
    AST_Semant_Map *semant_map = semant_analyze(root); 
    x = ast2xml(root, semant_map, with_location_info, true); // with semant info
    assert(!x->Error() && "AST is not valid!");
    x->SaveFile(file_ast_semant.c_str());

    return EXIT_SUCCESS;
}
