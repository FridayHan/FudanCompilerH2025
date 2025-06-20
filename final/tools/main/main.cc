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
#include "quad2xml.hh"
#include "xml2quad.hh"
#include "prepareregalloc.hh"
#include "regalloc.hh"
#include "blocking.hh"
#include "quadssa.hh"
#include "flowinfo.hh"
#include "color.hh"
#include "quad2rpi.hh"

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
    string file_quad_xml = "quad/" + file + ".4-xml.quad";

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

        // 保存 Quad 到普通文本文件
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

        // 保存 Quad 的 XML 表示
        cout << "Writing Quad XML to: " << file_quad_xml << endl;
        if (!quad2xml(qd, file_quad_xml.c_str())) {
            throw runtime_error("Failed to save Quad XML to file: " + file_quad_xml);
        }

        cout << ANSI_GREEN << "Successfully compiled " << file_irp << " to " << file_quad << " and " << file_quad_xml << ANSI_RESET << endl;
        // // 保存 Quad 到 XML 文件
        // cout << "Writing Quad XML to: " << file_quad_xml << endl;
        // XMLDocument* quad_doc = new XMLDocument();
        // XMLElement* root_element = quad_doc->NewElement("program");
        // quad_doc->InsertFirstChild(root_element);
        
        // // 将 Quad 程序转换为 XML
        // quad2xml(qd, quad_doc, root_element);
        
        // // 保存 XML 文件
        // quad_doc->SaveFile(file_quad_xml.c_str());
        // if (quad_doc->Error()) {
        //     throw runtime_error("Failed to save Quad XML to file: " + file_quad_xml);
        // }
        // delete quad_doc;

        // cout << ANSI_GREEN << "Successfully compiled " << file_irp << " to " << file_quad 
        //      << " and " << file_quad_xml << ANSI_RESET << endl;
    } catch (const exception &e) {
        cerr << ANSI_RED << "Error in ir2quad: " << e.what() << ANSI_RESET << endl;
        exit(EXIT_FAILURE);
    }
}

void quad2ssa(const string &file) {
    string file_quad = "quad/" + file + ".4.quad";
    string file_quad_xml = "quad/" + file + ".4-xml.quad";
    string file_quad_block = "quad/" + file + ".4-block.quad";
    string file_quad_ssa = "ssa/" + file + ".4-ssa.quad";
    string file_quad_ssa_xml = "ssa/" + file + ".4-ssa-xml.quad";

    try {
        // 读取XML格式的四元组文件
        cout << "Reading Quad from xml: " << file_quad_xml << endl;
        quad::QuadProgram *quad = xml2quad(file_quad_xml.c_str());
        if (quad == nullptr) {
            throw runtime_error("Error: " + file_quad_xml + " not found or Quad ill-formed");
        }

        // 输出文本格式的四元组文件（可选步骤）
        cout << "Writing Quad (text) to: " << file_quad << endl;
        string temp_str;
        temp_str.reserve(50000);
        quad->print(temp_str, 0, true);
        ofstream out_quad(file_quad);
        if (!out_quad) {
            throw runtime_error("Error opening file: " + file_quad);
        }
        out_quad << temp_str;
        out_quad.flush();
        out_quad.close();

        // 对四元组进行基本块划分
        cout << "Blocking the quad..." << endl;
        quad::QuadProgram *blocked_quad = blocking(quad);
        if (blocked_quad == nullptr) {
            throw runtime_error("Error blocking Quad");
        }

        // 保存分块后的四元组
        cout << "Writing blocked Quad to: " << file_quad_block << endl;
        temp_str.clear();
        temp_str.reserve(50000);
        blocked_quad->print(temp_str, 0, true);
        ofstream out_block(file_quad_block);
        if (!out_block) {
            throw runtime_error("Error opening file: " + file_quad_block);
        }
        out_block << temp_str;
        out_block.flush();
        out_block.close();

        // 转换为SSA格式
        cout << "Converting Quad to Quad-SSA..." << endl;
        quad::QuadProgram *ssa = quad2ssa(blocked_quad);  // quad2ssa函数需要分块后的四元组
        if (ssa == nullptr) {
            throw runtime_error("Error converting Quad to Quad-SSA");
        }

        // 保存SSA格式的四元组
        cout << "Writing Quad-SSA to: " << file_quad_ssa << endl;
        temp_str.clear();
        temp_str.reserve(50000);
        ssa->print(temp_str, 0, true);
        ofstream out_ssa(file_quad_ssa);
        if (!out_ssa) {
            throw runtime_error("Error opening file: " + file_quad_ssa);
        }
        out_ssa << temp_str;
        out_ssa.flush();
        out_ssa.close();

        // 保存SSA格式四元组的XML表示
        cout << "Writing Quad-SSA XML to: " << file_quad_ssa_xml << endl;
        if (!quad2xml(ssa, file_quad_ssa_xml.c_str())) {
            throw runtime_error("Failed to save SSA Quad XML to file: " + file_quad_ssa_xml);
        }

        // 释放内存
        delete quad;
        delete blocked_quad;
        delete ssa;

        cout << ANSI_GREEN << "Successfully compiled " << file_quad_xml << " to " << file_quad_ssa << " and " << file_quad_ssa_xml << ANSI_RESET << endl;
    } catch (const exception &e) {
        cerr << ANSI_RED << "Error in quad2ssa: " << e.what() << ANSI_RESET << endl;
        exit(EXIT_FAILURE);
    }
}

void ssa2clr(const string &file, int number_of_colors = 9) {
    string file_quad_ssa_xml = "ssa/" + file + ".4-ssa-xml.quad";
    string file_quad_prepared = "clr/" + file + ".4-prepared.quad";
    string file_quad_prepared_xml = "clr/" + file + ".4-prepared-xml.quad";
    string file_quad_color_xml = "clr/" + file + ".4-xml.clr";

    try {
        cout << "Number of colors to use = " << number_of_colors << endl;
        
        cout << "Reading Quad from xml: " << file_quad_ssa_xml << endl;
        quad::QuadProgram *quad_ssa = xml2quad(file_quad_ssa_xml.c_str());
        if (quad_ssa == nullptr) {
            throw runtime_error("Error reading Quad from xml: " + file_quad_ssa_xml);
        }

        // 准备寄存器分配
        cout << "Preparing for register allocation..." << endl;
        quad::QuadProgram *quad_prepared = prepareRegAlloc(quad_ssa);
        if (quad_prepared == nullptr) {
            throw runtime_error("Error preparing quad for register allocation");
        }

        // 保存准备后的四元组（文本格式）
        cout << "Writing prepared Quad to: " << file_quad_prepared << endl;
        string temp_str;
        temp_str.reserve(50000);
        quad_prepared->print(temp_str, 0, true);
        ofstream out_prepared(file_quad_prepared);
        if (!out_prepared) {
            throw runtime_error("Error opening file: " + file_quad_prepared);
        }
        out_prepared << temp_str;
        out_prepared.flush();
        out_prepared.close();

        // 保存准备后的四元组（XML格式）
        cout << "Writing prepared Quad XML to: " << file_quad_prepared_xml << endl;
        if (!quad2xml(quad_prepared, file_quad_prepared_xml.c_str())) {
            throw runtime_error("Failed to save prepared Quad XML to file: " + file_quad_prepared_xml);
        }

        // 执行寄存器分配
        cout << "Performing register allocation..." << endl;
        bool print_ig = false; // 不打印干扰图以减少输出
        XMLDocument *color_doc = coloring(quad_prepared, number_of_colors, print_ig);
        if (color_doc == nullptr) {
            throw runtime_error("Error performing register allocation");
        }

        // 保存颜色映射
        cout << "Writing color map to: " << file_quad_color_xml << endl;
        color_doc->SaveFile(file_quad_color_xml.c_str());
        if (color_doc->Error()) {
            throw runtime_error("Failed to save color map to file: " + file_quad_color_xml);
        }

        // 释放内存 - 注意：不要手动删除quad_ssa和quad_prepared，因为它们可能被库函数管理
        // XMLDocument会自动管理内存
        
        cout << ANSI_GREEN << "Successfully compiled " << file_quad_ssa_xml << " to " << file_quad_prepared_xml << " and " << file_quad_color_xml << ANSI_RESET << endl;
    } catch (const exception &e) {
        cerr << ANSI_RED << "Error in ssa2clr: " << e.what() << ANSI_RESET << endl;
        exit(EXIT_FAILURE);
    }
}

void clr2asm(const string &file) {
    string file_quad_prepared_xml = "clr/" + file + ".4-prepared-xml.quad";
    string file_quad_color_xml = "clr/" + file + ".4-xml.clr";
    string file_rpi = "asm/" + file + ".s";

    try {
        cout << "Reading prepared quad from xml: " << file_quad_prepared_xml << endl;
        quad::QuadProgram *x3 = xml2quad(file_quad_prepared_xml.c_str());
        if (x3 == nullptr) {
            throw runtime_error("Error reading Quad from xml: " + file_quad_prepared_xml);
        }
        
        cout << "Reading prepared colors for the QuadProgram: " << file_quad_color_xml << endl;
        ColorMap *colormap = xml2colormap(file_quad_color_xml);
        if (colormap == nullptr) {
            throw runtime_error("Error reading ColorMap from xml: " + file_quad_color_xml);
        }
        
        cout << "Writing rpi code to: " << file_rpi << endl;
        quad2rpi(x3, colormap, file_rpi);
        
        delete x3;
        delete colormap;
        
        cout << ANSI_GREEN << "Successfully compiled " << file_quad_prepared_xml << " and " << file_quad_color_xml << " to " << file_rpi << ANSI_RESET << endl;
    } catch (const exception &e) {
        cerr << ANSI_RED << "Error in clr2asm: " << e.what() << ANSI_RESET << endl;
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
    } else if (command == "quad2ssa") {
        quad2ssa(file);
    } else if (command == "ssa2clr") {
        int number_of_colors = 9;
        if (argc > 3 && strcmp(argv[2], "-k") == 0) {
            number_of_colors = stoi(argv[3]);
            file = argc > 4 ? argv[4] : "";
        }
        ssa2clr(file, number_of_colors);
    } else if (command == "clr2asm") {
        clr2asm(file);
    } else {
        cerr << "Unknown command: " << command << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}