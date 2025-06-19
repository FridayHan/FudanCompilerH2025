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

#ifdef DEBUG
#define DEBUG_PRINT(msg)                                                       \
  do {                                                                         \
    std::cerr << msg << std::endl;                                             \
  } while (0)
#define DEBUG_PRINT2(msg, val)                                                 \
  do {                                                                         \
    std::cerr << msg << " " << val << std::endl;                               \
  } while (0)
#else
#define DEBUG_PRINT(msg)                                                       \
  do {                                                                         \
  } while (0)
#define DEBUG_PRINT2(msg, val)                                                 \
  do {                                                                         \
  } while (0)
#endif

#define CHECK_NULLPTR(node)                                                    \
  if (!node) {                                                                 \
    std::cerr << "Error: Null pointer at " << __FILE__ << ":" << __LINE__      \
              << std::endl;                                                    \
    exit(1);                                                                   \
  }

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
string convertQuadStm(QuadStm* stmt, Color* color, int indent, map<string, bool>& label_emitted);

#endif // __QUAD2RPI_HH