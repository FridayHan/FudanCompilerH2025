#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "MinusIntConverter.hh"
#include "ast2xml.hh"
#include "constantPropagation.hh"
#include "executor.hh"
#include "xml2ast.hh"

using namespace std;
using namespace fdmj;
using namespace tinyxml2;

#define with_location_info false
// false means no location info in the AST XML files

Program *prog();

int main(int argc, const char *argv[]) {
  string file;

  const bool debug = argc > 1 && std::strcmp(argv[1], "--debug") == 0;

  if ((!debug && argc != 2) || (debug && argc != 3)) {
    cerr << "Usage: " << argv[0] << " [--debug] filename" << endl;
    return EXIT_FAILURE;
  }
  file = argv[argc - 1];

  // boilerplate output filenames (used throutghout the compiler pipeline)
  string file_fmj = file + ".fmj";  // input source file
  string file_out = file + ".out";
  string file_src = file + ".src";
  string file_src2 = file + ".1.debug.src";  // debug source file
  string file_ast = file + ".2.ast";         // ast in xml
  string file_ast2 = file + ".2-debug.ast";
  string file_ast3 = file + ".2-debug3.ast";
  string file_ast4 = file + ".2-debug4.ast";
  string file_irp = file + ".3.irp";
  string file_stm = file + ".4.stm";
  string file_liv = file + ".5.liv";

#ifdef DEBUG
  cout << "------Parsing fmj source file: " << file_fmj << "------------" << endl;
#endif
  std::ifstream fmjfile(file_fmj);
  Program *root = fdmjParser(fmjfile, false);  // false means no debug info from parser
  if (root == nullptr) {
    std::cout << "AST is not valid!" << endl;
    return EXIT_FAILURE;
  }
#ifdef DEBUG
  cout << "Convert AST  to XML..." << endl;
#endif
  XMLDocument *x = ast2xml(root, with_location_info);
  delete root;
  if (x->Error()) {
    std::cout << "AST is not valid!" << endl;
    return EXIT_FAILURE;
  }
#ifdef DEBUG
  cout << "Saving AST (XML) to: " << file_ast << endl;
#endif
  x->SaveFile(file_ast.c_str());
#ifdef DEBUG
  cout << "Loading AST (XML) from: " << file_ast << endl;
#endif
  delete x;
  x = new XMLDocument();
  x->LoadFile(file_ast.c_str());
  Program *root2 = xml2ast(x->RootElement());
  if (root2 == nullptr) {
    std::cout << "AST is not valid!" << endl;
    return EXIT_FAILURE;
  }
  XMLDocument *y = ast2xml(root2, with_location_info);
  delete root2;
#ifdef DEBUG
  cout << "Saving AST (XML) to: " << file_ast2 << endl;
#endif
  y->SaveFile(file_ast2.c_str());
#ifdef DEBUG
  cout << "Loading AST (XML) from: " << file_ast2 << endl;
#endif
  XMLDocument *z = new XMLDocument();
  z->LoadFile(file_ast2.c_str());
  root2 = xml2ast(z->RootElement());
#ifdef DEBUG
  cout << "Clone it ..." << endl;
#endif
  Program *root3 = root2->clone();
  delete root2;
#ifdef DEBUG
  cout << "Convert cloned AST to XML..." << endl;
#endif
  XMLDocument *w = ast2xml(root3, with_location_info);
#ifdef DEBUG
  cout << "Saving cloned AST (XML) to: " << file_ast3 << endl;
#endif
  w->SaveFile(file_ast3.c_str());
#ifdef DEBUG
  cout << "Rewriting AST..." << endl;
#endif
  Program *root4 = minusIntRewrite(root3);
#ifdef DEBUG
  cout << "Convert rewrote AST to XML..." << endl;
#endif
  w = ast2xml(root4, with_location_info);
#ifdef DEBUG
  cout << "Convert rewrote AST to XML..." << endl;
#endif
  Program *root5 = constantPropagate(root4);
  w = ast2xml(root5, with_location_info);
  Program *root6 = execute(root5);
  w = ast2xml(root6, with_location_info);
#ifdef DEBUG
  cout << "Saving AST (XML) to: " << file_ast4 << endl;
#endif
  w->SaveFile(file_ast4.c_str());
#ifdef DEBUG
  cout << "-----Done---" << endl;
#endif
  return EXIT_SUCCESS;
}