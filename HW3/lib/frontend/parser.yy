%skeleton "lalr1.cc"
%require  "3.2"
%language "c++"
%locations

%code requires {
  #define DEBUG
  #undef DEBUG
  #include <iostream>
  #include <vector>
  #include <algorithm>
  #include <string>
  #include <variant>
  #include "ast_location.hh"
  #include "ASTLexer.hh"
  #include "ASTheader.hh"
  #include "FDMJAST.hh"

  using namespace std;
  using namespace fdmj;
}

%{
#include <iostream>
#include <string>

#ifdef DEBUG
#define DEBUG_PRINT(msg) do { std::cerr << msg << std::endl; } while (0)
#define DEBUG_PRINT2(msg, val) do { std::cerr << msg << " " << val << std::endl; } while (0)
#else
#define DEBUG_PRINT(msg) do { } while (0)
#define DEBUG_PRINT2(msg, val) do { } while (0)
#endif
%}

%define api.namespace {fdmj}
%define api.parser.class {ASTParser}
%define api.value.type {AST_YYSTYPE}
%define api.location.type {ast_location}

%define parse.error detailed
%define parse.trace

%header
%verbose

%parse-param {ASTLexer &lexer}
%parse-param {const bool debug}
%parse-param {AST_YYSTYPE* result}

%initial-action
{
    #if YYDEBUG != 0
        set_debug_level(debug);
    #endif
};

%code {
    namespace fdmj 
    {
        template<typename RHS>
        void calcLocation(location_t &current, const RHS &rhs, const std::size_t n);
    }
    
    #define YYLLOC_DEFAULT(Cur, Rhs, N) calcLocation(Cur, Rhs, N)
    #define yylex lexer.yylex
    Pos *p;
}

//terminals with no value 
%token PUBLIC INT MAIN RETURN 
%token IF ELSE WHILE CONTINUE BREAK PUTINT PUTCH PUTARRAY STARTTIME STOPTIME
%token TRUE FALSE LENGTH GETINT GETCH GETARRAY THIS
%token CLASS EXTENDS
%token<i> NONNEGATIVEINT
%token<s> IDENTIFIER
%token '(' ')' '[' ']' '{' '}' '=' ',' ';' '.' 
%token ADD MINUS TIMES DIVIDE UMINUS EQ NE LT LE GT GE AND OR NOT

%left OR
%left AND
%left EQ NE
%left LT LE GT GE
%left ADD MINUS
%left TIMES DIVIDE
%right UMINUS
%right NOT
%left '.'
%left '[' ']'
%left '(' ')'
%nonassoc IFX
%nonassoc ELSE

//non-terminals, need type information only (not tokens)
%type <intExp> NUM
%type <intExp> CONST
%type <intExpList> CONSTLIST
%type <intExpList> CONSTREST
%type <idExp> ID
// %type <opExp> OPEXP
// %type <boolExp> BOOLEXP
%type <program> PROG
%type <mainMethod> MAINMETHOD
%type <classDecl> CLASSDECL
%type <classDeclList> CLASSDECLLIST
%type <type> TYPE
%type <varDecl> VARDECL
%type <varDeclList> VARDECLLIST
%type <methodDecl> METHODDECL
%type <methodDeclList> METHODDECLLIST
%type <formalList> FORMALLIST
%type <formalList> FORMALREST
%type <stm> STM
%type <stmList> STMLIST
%type <exp> EXP
%type <expList> EXPLIST
%type <expList> EXPREST

%start PROG
%expect 0

%%

NUM: NONNEGATIVEINT
  {
    DEBUG_PRINT2("NonNegativeInt: ", $1);
    $$ = new IntExp(p, $1);
  }
  ;

CONST: NUM
  {
    DEBUG_PRINT("Const");
    $$ = $1;
  }
  |
  MINUS NUM %prec UMINUS
  {
    DEBUG_PRINT("Negative Const");
    $$ = new IntExp(p, -$2->val);
  }
  ;

CONSTLIST: /* empty */
  {
    DEBUG_PRINT("ConstList empty");
    $$ = new vector<IntExp*>();
  }
  |
  CONST CONSTREST
  {
    DEBUG_PRINT("Const ConstList");
    vector<IntExp*> *v = $2;
    v->insert(v->begin(), $1);
    $$ = v;
  }
  ;

CONSTREST: /* empty */
  {
    DEBUG_PRINT("ConstRest empty");
    $$ = new vector<IntExp*>();
  }
  |
  ',' CONST CONSTREST
  {
    DEBUG_PRINT("ConstRest");
    vector<IntExp*> *v = $3;
    v->insert(v->begin(), $2);
    $$ = v;
  }
  ;

ID: IDENTIFIER
  {
    DEBUG_PRINT2("Identifier: ", $1);
    $$ = new IdExp(p, $1);
  }
  ;

// OPEXP: ADD
//   {
// #ifdef DEBUG
//     cerr << "Operator: +" << endl;
// #endif
//     $$ = new OpExp(p, "+");
//   }
//   |
//   MINUS
//   {
// #ifdef DEBUG
//     cerr << "Operator: -" << endl;
// #endif
//     $$ = new OpExp(p, "-");
//   }
//   |
//   TIMES
//   {
// #ifdef DEBUG
//     cerr << "Operator: *" << endl;
// #endif
//     $$ = new OpExp(p, "*");
//   }
//   |
//   DIVIDE
//   {
// #ifdef DEBUG
//     cerr << "Operator: /" << endl;
// #endif
//     $$ = new OpExp(p, "/");
//   }
//   |
//   EQ
//   {
// #ifdef DEBUG
//     cerr << "Operator: ==" << endl;
// #endif
//     $$ = new OpExp(p, "==");
//   }
//   |
//   NE
//   {
// #ifdef DEBUG
//     cerr << "Operator: !=" << endl;
// #endif
//     $$ = new OpExp(p, "!=");
//   }
//   |
//   LT
//   {
// #ifdef DEBUG
//     cerr << "Operator: <" << endl;
// #endif
//     $$ = new OpExp(p, "<");
//   }
//   |
//   LE
//   {
// #ifdef DEBUG
//     cerr << "Operator: <=" << endl;
// #endif
//     $$ = new OpExp(p, "<=");
//   }
//   |
//   GT
//   {
// #ifdef DEBUG
//     cerr << "Operator: >" << endl;
// #endif
//     $$ = new OpExp(p, ">");
//   }
//   |
//   GE
//   {
// #ifdef DEBUG
//     cerr << "Operator: >=" << endl;
// #endif
//     $$ = new OpExp(p, ">=");
//   }
//   |
//   AND
//   {
// #ifdef DEBUG
//     cerr << "Operator: &&" << endl;
// #endif
//     $$ = new OpExp(p, "&&");
//   }
//   |
//   OR
//   {
// #ifdef DEBUG
//     cerr << "Operator: ||" << endl;
// #endif
//     $$ = new OpExp(p, "||");
//   }
//   |
//   NOT
//   {
// #ifdef DEBUG
//     cerr << "Operator: !" << endl;
// #endif
//     $$ = new OpExp(p, "!");
//   }
//   ;

// BOOLEXP: TRUE
//   {
// #ifdef DEBUG
//     cerr << "Boolean: true" << endl;
// #endif
//     $$ = new BoolExp(p, true);
//   }
//   |
//   FALSE
//   {
// #ifdef DEBUG
//     cerr << "Boolean: false" << endl;
// #endif
//     $$ = new BoolExp(p, false);
//   }
//   ;

PROG: MAINMETHOD CLASSDECLLIST
  { 
    DEBUG_PRINT("Program");
    result->root = new Program(p, $1, $2);
  }
  ;

MAINMETHOD: PUBLIC INT MAIN '(' ')' '{' VARDECLLIST STMLIST '}'
  {
    DEBUG_PRINT("MainMethod");
    $$ = new MainMethod(p, $7, $8);
  }
  ;

CLASSDECLLIST: /* empty */
  {
    DEBUG_PRINT("ClassDeclList empty");
    $$ = new vector<ClassDecl*>();
  }
  |
  CLASSDECL CLASSDECLLIST
  {
    DEBUG_PRINT("ClassDecl ClassDeclList");
    vector<ClassDecl*> *v = $2;
    v->push_back($1);
    $$ = v;
  }
  ;

CLASSDECL: PUBLIC CLASS ID '{' VARDECLLIST METHODDECLLIST '}'
  {
    DEBUG_PRINT("ClassDecl");
    $$ = new ClassDecl(p, $3, nullptr, $5, $6);
  }
  |
  PUBLIC CLASS ID EXTENDS ID '{' VARDECLLIST METHODDECLLIST '}'
  {
    DEBUG_PRINT("ClassDecl with extends");
    $$ = new ClassDecl(p, $3, $5, $7, $8);
  }
  ;

VARDECLLIST: /* empty */
  {
    DEBUG_PRINT("VarDeclList empty");
    $$ = new vector<VarDecl*>();
  }
  |
  VARDECL VARDECLLIST
  {
    DEBUG_PRINT("VarDecl VarDeclList");
    vector<VarDecl*> *v = $2;
    v->push_back($1);
    $$ = v;
  }
  ;

VARDECL: CLASS ID ID ';'
  {
    DEBUG_PRINT("Class VarDecl");
    $$ = new VarDecl(p, new Type(p, $2), $3);
  }
  |
  INT ID ';'
  {
    DEBUG_PRINT("Int VarDecl");
    $$ = new VarDecl(p, new Type(p), $2);
  }
  |
  INT ID '=' CONST ';'
  {
    DEBUG_PRINT("Int VarDecl with initialization");
    $$ = new VarDecl(p, new Type(p), $2, $4);
  }
  |
  INT '[' ']' ID ';'
  {
    DEBUG_PRINT("Array VarDecl");
    $$ = new VarDecl(p, new Type(p, TypeKind::ARRAY, nullptr, new IntExp(p, 0)), $4);
  }
  |
  INT '[' ']' ID '=' '{' CONSTLIST '}' ';'
  {
    DEBUG_PRINT("Array VarDecl with initialization");
    $$ = new VarDecl(p, new Type(p, TypeKind::ARRAY, nullptr, new IntExp(p, 0)), $4, $7); // TODO: 使用 $7->size() 即可获取初始化 vector 的长度
  }
  |
  INT '[' NUM ']' ID ';'
  {
    DEBUG_PRINT("Fixed-size Array VarDecl");
    $$ = new VarDecl(p, new Type(p, TypeKind::ARRAY, nullptr, $3), $5);
  }
  |
  INT '[' NUM ']' ID '=' '{' CONSTLIST '}' ';'
  {
    DEBUG_PRINT("Fixed-size Array VarDecl with initialization");
    $$ = new VarDecl(p, new Type(p, TypeKind::ARRAY, nullptr, $3), $5, $8);
  }
  ;

METHODDECLLIST: /* empty */
  {
    DEBUG_PRINT("MethodDeclList empty");
    $$ = new vector<MethodDecl*>();
  }
  |
  METHODDECL METHODDECLLIST
  {
    DEBUG_PRINT("MethodDecl MethodDeclList");
    vector<MethodDecl*> *v = $2;
    v->push_back($1);
    $$ = v;
  }
  ;

METHODDECL: PUBLIC TYPE ID '(' FORMALLIST ')' '{' VARDECLLIST STMLIST '}'
  {
    DEBUG_PRINT("MethodDecl");
    $$ = new MethodDecl(p, $2, $3, $5, $8, $9);
  }
  ;

TYPE: CLASS ID
  {
    DEBUG_PRINT("Class Type");
    $$ = new Type(p, TypeKind::CLASS, $2, nullptr);
  }
  |
  INT
  {
    DEBUG_PRINT("Int Type");
    $$ = new Type(p);
  }
  |
  INT '[' ']'
  {
    DEBUG_PRINT("Array Type");
    $$ = new Type(p, TypeKind::ARRAY, nullptr, new IntExp(p, 0));
  }
  ;

FORMALLIST: /* empty */
  {
    DEBUG_PRINT("FormalList empty");
    $$ = new vector<Formal*>();
  }
  |
  TYPE ID FORMALREST
  {
    DEBUG_PRINT("FormalList");
    vector<Formal*> *v = $3;
    v->insert(v->begin(), new Formal(p, $1, $2));
    $$ = v;
  }
  ;

FORMALREST: /* empty */
  {
    DEBUG_PRINT("FormalRest empty");
    $$ = new vector<Formal*>();
  }
  |
  ',' TYPE ID FORMALREST
  {
    DEBUG_PRINT("FormalRest");
    vector<Formal*> *v = $4;
    v->insert(v->begin(), new Formal(p, $2, $3));
    $$ = v;
  }
  ;

STMLIST: /* empty */
  {
    DEBUG_PRINT("StmList empty");
    $$ = new vector<Stm*>();
  }
  |
  STM STMLIST
  {
    DEBUG_PRINT("Stm Stmlist");
    vector<Stm*> *v = $2;
    v->push_back($1);
    rotate(v->begin(), v->end() - 1, v->end());
    $$ = v;
  }
  ;

STM: '{' STMLIST '}'
  {
    DEBUG_PRINT("Block Stm");
    $$ = new Nested(p, $2);
  }
  |
  IF '(' EXP ')' STM ELSE STM
  {
    DEBUG_PRINT("If-Else Stm");
    $$ = new If(p, $3, $5, $7);
  }
  |
  IF '(' EXP ')' STM %prec IFX
  {
    DEBUG_PRINT("If Stm");
    $$ = new If(p, $3, $5);
  }
  |
  WHILE '(' EXP ')' STM
  {
    DEBUG_PRINT("While Stm");
    $$ = new While(p, $3, $5);
  }
  |
  WHILE '(' EXP ')' ';'
  {
    DEBUG_PRINT("While Stm with empty body");
    $$ = new While(p, $3);
  }
  |
  EXP '=' EXP ';'
  {
    DEBUG_PRINT("Assign Stm");
    $$ = new Assign(p, $1, $3);
  }
  |
  EXP '.' ID '(' EXPLIST ')' ';'
  {
    DEBUG_PRINT("Method Call Stm");
    $$ = new CallStm(p, $1, $3, $5);
  }
  |
  CONTINUE ';'
  {
    DEBUG_PRINT("Continue Stm");
    $$ = new Continue(p);
  }
  |
  BREAK ';'
  {
    DEBUG_PRINT("Break Stm");
    $$ = new Break(p);
  }
  |
  RETURN EXP ';'
  {
    DEBUG_PRINT("Return Stm");
    $$ = new Return(p, $2);
  }
  |
  PUTINT '(' EXP ')' ';'
  {
    DEBUG_PRINT("PutInt Stm");
    $$ = new PutInt(p, $3);
  }
  |
  PUTCH '(' EXP ')' ';'
  {
    DEBUG_PRINT("PutCh Stm");
    $$ = new PutCh(p, $3);
  }
  |
  PUTARRAY '(' EXP ',' EXP ')' ';'
  {
    DEBUG_PRINT("PutArray Stm");
    $$ = new PutArray(p, $3, $5);
  }
  |
  STARTTIME '(' ')' ';'
  {
    DEBUG_PRINT("StartTime Stm");
    $$ = new Starttime(p);
  }
  |
  STOPTIME '(' ')' ';'
  {
    DEBUG_PRINT("StopTime Stm");
    $$ = new Stoptime(p);
  }
  ;

EXPLIST: /* empty */
  {
    DEBUG_PRINT("ExpList empty");
    $$ = new vector<Exp*>();
  }
  |
  EXP EXPREST
  {
    DEBUG_PRINT("Exp ExpRest");
    vector<Exp*> *v = $2;
    v->insert(v->begin(), $1);
    $$ = v;
  }
  ;

EXPREST: /* empty */
  {
    DEBUG_PRINT("ExpRest empty");
    $$ = new vector<Exp*>();
  }
  |
  ',' EXP EXPREST
  {
    DEBUG_PRINT("ExpRest");
    vector<Exp*> *v = $3;
    v->insert(v->begin(), $2);
    $$ = v;
  }

EXP: NUM
  {
    DEBUG_PRINT("IntExp Exp");
    $$ = $1;
  }
  |
  TRUE
  {
    DEBUG_PRINT("Truemr Exp");
    $$ = new BoolExp(p, true);
  }
  |
  FALSE
  {
    DEBUG_PRINT("False Exp");
    $$ = new BoolExp(p, false);
  }
  |
  LENGTH '(' EXP ')'
  {
    DEBUG_PRINT("Length Exp");
    $$ = new Length(p, $3);
  }
  |
  GETINT '(' ')'
  {
    DEBUG_PRINT("GetInt Exp");
    $$ = new GetInt(p);
  }
  |
  GETCH '(' ')'
  {
    DEBUG_PRINT("GetCh Exp");
    $$ = new GetCh(p);
  }
  |
  GETARRAY '(' EXP ')'
  {
    DEBUG_PRINT("GetArray Exp");
    $$ = new GetArray(p, $3);
  }
  |
  ID
  {
    DEBUG_PRINT("Identifier Exp");
    $$ = $1;
  }
  |
  THIS
  {
    DEBUG_PRINT("This Exp");
    $$ = new This(p);
  }
  |
  EXP ADD EXP
  {
    DEBUG_PRINT("Add Exp");
    $$ = new BinaryOp(p, $1, new OpExp(p, "+"), $3);
  }
  |
  EXP MINUS EXP
  {
    DEBUG_PRINT("Minus Exp");
    $$ = new BinaryOp(p, $1, new OpExp(p, "-"), $3);
  }
  |
  EXP TIMES EXP
  {
    DEBUG_PRINT("Times Exp");
    $$ = new BinaryOp(p, $1, new OpExp(p, "*"), $3);
  }
  |
  EXP DIVIDE EXP
  {
    DEBUG_PRINT("Divide Exp");
    $$ = new BinaryOp(p, $1, new OpExp(p, "/"), $3);
  }
  |
  EXP AND EXP
  {
    DEBUG_PRINT("And Exp");
    $$ = new BinaryOp(p, $1, new OpExp(p, "&&"), $3);
  }
  |
  EXP OR EXP
  {
    DEBUG_PRINT("Or Exp");
    $$ = new BinaryOp(p, $1, new OpExp(p, "||"), $3);
  }
  |
  EXP EQ EXP
  {
    DEBUG_PRINT("Equal Exp");
    $$ = new BinaryOp(p, $1, new OpExp(p, "=="), $3);
  }
  |
  EXP NE EXP
  {
    DEBUG_PRINT("Not Equal Exp");
    $$ = new BinaryOp(p, $1, new OpExp(p, "!="), $3);
  }
  |
  EXP LT EXP
  {
    DEBUG_PRINT("Less Than Exp");
    $$ = new BinaryOp(p, $1, new OpExp(p, "<"), $3);
  }
  |
  EXP LE EXP
  {
    DEBUG_PRINT("Less Equal Exp");
    $$ = new BinaryOp(p, $1, new OpExp(p, "<="), $3);
  }
  |
  EXP GT EXP
  {
    DEBUG_PRINT("Greater Than Exp");
    $$ = new BinaryOp(p, $1, new OpExp(p, ">"), $3);
  }
  |
  EXP GE EXP
  {
    DEBUG_PRINT("Greater Equal Exp");
    $$ = new BinaryOp(p, $1, new OpExp(p, ">="), $3);
  }
  |
  NOT EXP
  {
    DEBUG_PRINT("Not Exp");
    $$ = new UnaryOp(p, new OpExp(p, "!"), $2);
  }
  |
  MINUS EXP %prec UMINUS
  {
    DEBUG_PRINT("Negate Exp");
    $$ = new UnaryOp(p, new OpExp(p, "-"), $2);
  }
  |
  '(' EXP ')'
  {
    DEBUG_PRINT("Parenthesized Exp");
    $$ = $2;
  }
  |
  '(' '{' STMLIST '}' EXP ')'
  {
    DEBUG_PRINT("Block Exp");
    $$ = new Esc(p, $3, $5);
  }
  |
  EXP '.' ID
  {
    DEBUG_PRINT("Field Access Exp");
    $$ = new ClassVar(p, $1, $3);
  }
  |
  EXP '.' ID '(' EXPLIST ')'
  {
    DEBUG_PRINT("Method Call Exp");
    $$ = new CallExp(p, $1, $3, $5);
  }
  |
  EXP '[' EXP ']'
  {
    DEBUG_PRINT("Array Access Exp");
    $$ = new ArrayExp(p, $1, $3);
  }
  ;

%%
/*
void yyerror(char *s) {
  fprintf(stderr, "%s\n",s);
}

int yywrap() {
  return(1);
}
*/

//%code 
namespace fdmj 
{
    template<typename RHS>
    inline void calcLocation(location_t &current, const RHS &rhs, const std::size_t n)
    {
        current = location_t(YYRHSLOC(rhs, 1).sline, YYRHSLOC(rhs, 1).scolumn, 
                                    YYRHSLOC(rhs, n).eline, YYRHSLOC(rhs, n).ecolumn);
        p = new Pos(current.sline, current.scolumn, current.eline, current.ecolumn);
    }
    
    void ASTParser::error(const location_t &location, const std::string &message)
    {
        std::cerr << "Error at lines " << location << ": " << message << std::endl;
    }

  Program* fdmjParser(ifstream &fp, const bool debug) {
    fdmj:AST_YYSTYPE result; 
    result.root = nullptr;
    fdmj::ASTLexer lexer(fp, debug);
    fdmj::ASTParser parser(lexer, debug, &result); //set up the parser
    if (parser() ) { //call the parser
      cout << "Error: parsing failed" << endl;
      return nullptr;
    }
    if (debug) cout << "Parsing successful" << endl;
    return result.root;
  }

  Program*  fdmjParser(const string &filename, const bool debug) {
    std::ifstream fp(filename);
    if (!fp) {
      cout << "Error: cannot open file " << filename << endl;
      return nullptr;
    }
    return fdmjParser(fp, debug);
  }
}
