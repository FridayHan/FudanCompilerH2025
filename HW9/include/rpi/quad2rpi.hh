#ifndef __QUAD2RPI_HH
#define __QUAD2RPI_HH
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "quad.hh"
#include "color.hh"

using namespace std;
using namespace quad;

void trace(QuadFuncDecl* func);
void trace(QuadProgram* prog);

bool rpi_isMachineReg(int n);
string quad2rpi(QuadProgram* quadProgram, ColorMap *cm);
void quad2rpi(QuadProgram* quadProgram, ColorMap *cm, string filename);

// Helper function
string loadSpilledTemp(int temp_num, Color* color, int reg_num, int indent);
string storeSpilledTemp(int temp_num, Color* color, int reg_num, int indent);
string getTermStr(QuadTerm *term, Color *color, int temp_reg);
string loadSpillIfNeeded(QuadTerm *term, Color *color, int temp_reg, int indent);
string convertQuadStm(QuadStm* stmt, Color* color, int indent);

#endif // __QUAD2RPI_HH