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
%token ADD MINUS TIMES DIVIDE EQ NE LT LE GT GE AND OR NOT

%left OR
%left AND
%right NOT
%left EQ NE LT LE GT GE
%left ADD MINUS
%left TIMES DIVIDE





//non-terminals, need type information only (not tokens)
%type <intExp> INTEXP
%type <intExpList> INTEXPLIST
%type <intExpList> INTEXPLISTREST
%type <idExp> ID
%type <opExp> OPEXP
%type <boolExp> BOOLEXP
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

%start PROG
%expect 0

%%

INTEXP: NONNEGATIVEINT
  {
#ifdef DEBUG
    cerr << "NonNegativeInt: " << $1 << endl;
#endif
    $$ = new IntExp(p, $1);
  }
  ;

ID: IDENTIFIER
  {
#ifdef DEBUG
    cerr << "Identifier: " << $1 << endl;
#endif
    $$ = new IdExp(p, $1);
  }
  ;

OPEXP: ADD
  {
#ifdef DEBUG
    cerr << "Operator: +" << endl;
#endif
    $$ = new OpExp(p, "+");
  }
  |
  MINUS
  {
#ifdef DEBUG
    cerr << "Operator: -" << endl;
#endif
    $$ = new OpExp(p, "-");
  }
  |
  TIMES
  {
#ifdef DEBUG
    cerr << "Operator: *" << endl;
#endif
    $$ = new OpExp(p, "*");
  }
  |
  DIVIDE
  {
#ifdef DEBUG
    cerr << "Operator: /" << endl;
#endif
    $$ = new OpExp(p, "/");
  }
  |
  EQ
  {
#ifdef DEBUG
    cerr << "Operator: ==" << endl;
#endif
    $$ = new OpExp(p, "==");
  }
  |
  NE
  {
#ifdef DEBUG
    cerr << "Operator: !=" << endl;
#endif
    $$ = new OpExp(p, "!=");
  }
  |
  LT
  {
#ifdef DEBUG
    cerr << "Operator: <" << endl;
#endif
    $$ = new OpExp(p, "<");
  }
  |
  LE
  {
#ifdef DEBUG
    cerr << "Operator: <=" << endl;
#endif
    $$ = new OpExp(p, "<=");
  }
  |
  GT
  {
#ifdef DEBUG
    cerr << "Operator: >" << endl;
#endif
    $$ = new OpExp(p, ">");
  }
  |
  GE
  {
#ifdef DEBUG
    cerr << "Operator: >=" << endl;
#endif
    $$ = new OpExp(p, ">=");
  }
  |
  AND
  {
#ifdef DEBUG
    cerr << "Operator: &&" << endl;
#endif
    $$ = new OpExp(p, "&&");
  }
  |
  OR
  {
#ifdef DEBUG
    cerr << "Operator: ||" << endl;
#endif
    $$ = new OpExp(p, "||");
  }
  |
  NOT
  {
#ifdef DEBUG
    cerr << "Operator: !" << endl;
#endif
    $$ = new OpExp(p, "!");
  }
  ;

BOOLEXP: TRUE
  {
#ifdef DEBUG
    cerr << "Boolean: true" << endl;
#endif
    $$ = new BoolExp(p, true);
  }
  |
  FALSE
  {
#ifdef DEBUG
    cerr << "Boolean: false" << endl;
#endif
    $$ = new BoolExp(p, false);
  }
  ;

INTEXPLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "IntExpList empty" << endl;
#endif
    $$ = new vector<IntExp*>();
  }
  |
  INTEXP INTEXPLISTREST
  {
#ifdef DEBUG
    cerr << "IntExp IntExpList" << endl;
#endif
    vector<IntExp*> *v = $2;
    v->insert(v->begin(), $1);
    $$ = v;
  }
  ;

INTEXPLISTREST: /* empty */
  {
#ifdef DEBUG
    cerr << "IntExpListRest empty" << endl;
#endif
    $$ = new vector<IntExp*>();
  }
  |
  ',' INTEXP INTEXPLISTREST
  {
#ifdef DEBUG
    cerr << "IntExpListRest" << endl;
#endif
    vector<IntExp*> *v = $3;
    v->insert(v->begin(), $2);
    $$ = v;
  }
  ;

PROG: MAINMETHOD CLASSDECLLIST
  { 
#ifdef DEBUG
    cerr << "Program" << endl;
#endif
    result->root = new Program(p, $1, $2);
  }
  ;

MAINMETHOD: PUBLIC INT MAIN '(' ')' '{' VARDECLLIST STMLIST '}'
  {
#ifdef DEBUG
    cerr << "MainMethod" << endl;
#endif
    $$ = new MainMethod(p, $7, $8);
  }
  ;

CLASSDECLLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "ClassDeclList empty" << endl;
#endif
    $$ = new vector<ClassDecl*>();
  }
  |
  CLASSDECL CLASSDECLLIST
  {
#ifdef DEBUG
    cerr << "ClassDecl ClassDeclList" << endl;
#endif
    vector<ClassDecl*> *v = $2;
    v->push_back($1);
    $$ = v;
  }
  ;

CLASSDECL: PUBLIC CLASS IDENTIFIER '{' VARDECLLIST METHODDECLLIST '}'
  {
#ifdef DEBUG
    cerr << "ClassDecl" << endl;
#endif
    $$ = new ClassDecl(p, $3, nullptr, $5, $6);
  }
  |
  PUBLIC CLASS IDENTIFIER EXTENDS IDENTIFIER '{' VARDECLLIST METHODDECLLIST '}'
  {
#ifdef DEBUG
    cerr << "ClassDecl with extends" << endl;
#endif
    $$ = new ClassDecl(p, $3, $5, $7, $8);
  }
  ;

VARDECLLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "VarDeclList empty" << endl;
#endif
    $$ = new vector<VarDecl*>();
  }
  |
  VARDECL VARDECLLIST
  {
#ifdef DEBUG
    cerr << "VarDecl VarDeclList" << endl;
#endif
    vector<VarDecl*> *v = $2;
    v->push_back($1);
    $$ = v;
  }
  ;

VARDECL: CLASS IDENTIFIER IDENTIFIER ';'
  {
#ifdef DEBUG
    cerr << "Class VarDecl" << endl;
#endif
    $$ = new VarDecl(p, new Type(p, new IdExp(p, $2)), new IdExp(p, $3));
  }
  |
  INT IDENTIFIER ';'
  {
#ifdef DEBUG
    cerr << "Int VarDecl" << endl;
#endif
    $$ = new VarDecl(p, new Type(p, TypeKind::INT), new IdExp(p, $2));
  }
  |
  INT IDENTIFIER '=' INTEXP ';'
  {
#ifdef DEBUG
    cerr << "Int VarDecl with initialization" << endl;
#endif
    $$ = new VarDecl(p, new Type(p, TypeKind::INT), new IdExp(p, $2), $4);
  }
  |
  INT '[' ']' IDENTIFIER ';'
  {
#ifdef DEBUG
    cerr << "Array VarDecl" << endl;
#endif
    $$ = new VarDecl(p, new Type(p, TypeKind::ARRAY, nullptr, new IntExp(p, 0)), new IdExp(p, $4));
  }
  |
  INT '[' ']' IDENTIFIER '=' '{' INTEXPLIST '}' ';'
  {
#ifdef DEBUG
    cerr << "Array VarDecl with initialization" << endl;
#endif
    $$ = new VarDecl(p, new Type(p, TypeKind::ARRAY, nullptr, new IntExp(p, 0)), new IdExp(p, $4), $7);
  }
  |
  INT '[' NONNEGATIVEINT ']' IDENTIFIER ';'
  {
#ifdef DEBUG
    cerr << "Fixed-size Array VarDecl" << endl;
#endif
    $$ = new VarDecl(p, new Type(p, TypeKind::ARRAY, nullptr, new IntExp(p, $3)), new IdExp(p, $5));
  }
  |
  INT '[' NONNEGATIVEINT ']' IDENTIFIER '=' '{' INTEXPLIST '}' ';'
  {
#ifdef DEBUG
    cerr << "Fixed-size Array VarDecl with initialization" << endl;
#endif
    $$ = new VarDecl(p, new Type(p, TypeKind::ARRAY, nullptr, new IntExp(p, $3)), new IdExp(p, $5), $8);
  }
  ;

METHODDECLLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "MethodDeclList empty" << endl;
#endif
    $$ = new vector<MethodDecl*>();
  }
  |
  METHODDECL METHODDECLLIST
  {
#ifdef DEBUG
    cerr << "MethodDecl MethodDeclList" << endl;
#endif
    vector<MethodDecl*> *v = $2;
    v->push_back($1);
    $$ = v;
  }
  ;

METHODDECL: PUBLIC TYPE IDENTIFIER '(' FORMALLIST ')' '{' VARDECLLIST STMLIST '}'
  {
#ifdef DEBUG
    cerr << "MethodDecl" << endl;
#endif
    $$ = new MethodDecl(p, $2, $3, $5, $8, $9);
  }
  ;

TYPE: CLASS IDENTIFIER
  {
#ifdef DEBUG
    cerr << "Class Type" << endl;
#endif
    $$ = new Type(p, TypeKind::CLASS, new IdExp(p, $2), nullptr);
  }
  |
  INT
  {
#ifdef DEBUG
    cerr << "Int Type" << endl;
#endif
    $$ = new Type(p, TypeKind::INT);
  }
  |
  INT '[' ']'
  {
#ifdef DEBUG
    cerr << "Array Type" << endl;
#endif
    $$ = new Type(p, TypeKind::ARRAY, nullptr, new IntExp(p, 0));
  }
  ;

FORMALLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "FormalList empty" << endl;
#endif
    $$ = new vector<Formal*>();
  }
  |
  TYPE IDENTIFIER FORMALREST
  {
#ifdef DEBUG
    cerr << "FormalList" << endl;
#endif
    vector<Formal*> *v = $3;
    v->insert(v->begin(), new Formal(p, $1, $2));
    $$ = v;
  }
  ;

FORMALREST: /* empty */
  {
#ifdef DEBUG
    cerr << "FormalRest empty" << endl;
#endif
    $$ = new vector<Formal*>();
  }
  |
  ',' TYPE IDENTIFIER FORMALREST
  {
#ifdef DEBUG
    cerr << "FormalRest" << endl;
#endif
    vector<Formal*> *v = $4;
    v->insert(v->begin(), new Formal(p, $2, $3));
    $$ = v;
  }
  ;

STMLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "StmList empty" << endl;
#endif
    $$ = new vector<Stm*>();
  }
  |
  STM STMLIST
  {
#ifdef DEBUG
    cerr << "Stm StmList" << endl;
#endif
    vector<Stm*> *v = $2;
    v->push_back($1);
    rotate(v->begin(), v->end() - 1, v->end());
    $$ = v;
  }
  ;

STM: '{' STMLIST '}'
  {
#ifdef DEBUG
    cerr << "Block Stm" << endl;
#endif
    $$ = new Nested(p, $2);
  }
  |
  IF '(' EXP ')' STM ELSE STM
  {
#ifdef DEBUG
    cerr << "If-Else Stm" << endl;
#endif
    $$ = new If(p, $3, $5, $7);
  }
  |
  IF '(' EXP ')' STM
  {
#ifdef DEBUG
    cerr << "If Stm" << endl;
#endif
    $$ = new If(p, $3, $5);
  }
  |
  WHILE '(' EXP ')' STM
  {
#ifdef DEBUG
    cerr << "While Stm" << endl;
#endif
    $$ = new While(p, $3, $5);
  }
  |
  WHILE '(' EXP ')' ';'
  {
#ifdef DEBUG
    cerr << "While Stm with empty body" << endl;
#endif
    $$ = new While(p, $3);
  }
  |
  EXP '=' EXP ';'
  {
#ifdef DEBUG
    cerr << "Assign Stm" << endl;
#endif
    $$ = new Assign(p, $1, $3);
  }
  |
  EXP '.' IDENTIFIER '(' EXPLIST ')' ';'
  {
#ifdef DEBUG
    cerr << "Method Call Stm" << endl;
#endif
    $$ = new CallStm(p, $1, $3, $5);
  }
  |
  CONTINUE ';'
  {
#ifdef DEBUG
    cerr << "Continue Stm" << endl;
#endif
    $$ = new Continue(p);
  }
  |
  BREAK ';'
  {
#ifdef DEBUG
    cerr << "Break Stm" << endl;
#endif
    $$ = new Break(p);
  }
  |
  RETURN EXP ';'
  {
#ifdef DEBUG
    cerr << "Return Stm" << endl;
#endif
    $$ = new Return(p, $2);
  }
  |
  PUTINT '(' EXP ')' ';'
  {
#ifdef DEBUG
    cerr << "PutInt Stm" << endl;
#endif
    $$ = new PutInt(p, $3);
  }
  |
  PUTCH '(' EXP ')' ';'
  {
#ifdef DEBUG
    cerr << "PutCh Stm" << endl;
#endif
    $$ = new PutCh(p, $3);
  }
  |
  PUTARRAY '(' EXP ',' EXP ')' ';'
  {
#ifdef DEBUG
    cerr << "PutArray Stm" << endl;
#endif
    $$ = new PutArray(p, $3, $5);
  }
  |
  STARTTIME '(' ')' ';'
  {
#ifdef DEBUG
    cerr << "StartTime Stm" << endl;
#endif
    $$ = new Starttime(p);
  }
  |
  STOPTIME '(' ')' ';'
  {
#ifdef DEBUG
    cerr << "StopTime Stm" << endl;
#endif
    $$ = new Stoptime(p);
  }
  ;

EXPLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "ExpList empty" << endl;
#endif
    $$ = new vector<Exp*>();
  }
  |
  EXP EXPLIST
  {
#ifdef DEBUG
    cerr << "Exp ExpList" << endl;
#endif
    vector<Exp*> *v = $2;
    v->insert(v->begin(), $1);
    $$ = v;
  }
  ;

EXP: NONNEGATIVEINT
  {
#ifdef DEBUG
    cerr << "NonNegativeInt Exp" << endl;
#endif
    $$ = new IntExp(p, $1);
  }
  |
  TRUE
  {
#ifdef DEBUG
    cerr << "True Exp" << endl;
#endif
    $$ = new BoolExp(p, true);
  }
  |
  FALSE
  {
#ifdef DEBUG
    cerr << "False Exp" << endl;
#endif
    $$ = new BoolExp(p, false);
  }
  |
  LENGTH '(' EXP ')'
  {
#ifdef DEBUG
    cerr << "Length Exp" << endl;
#endif
    $$ = new Length(p, $3);
  }
  |
  GETINT '(' ')'
  {
#ifdef DEBUG
    cerr << "GetInt Exp" << endl;
#endif
    $$ = new GetInt(p);
  }
  |
  GETCH '(' ')'
  {
#ifdef DEBUG
    cerr << "GetCh Exp" << endl;
#endif
    $$ = new GetCh(p);
  }
  |
  GETARRAY '(' EXP ')'
  {
#ifdef DEBUG
    cerr << "GetArray Exp" << endl;
#endif
    $$ = new GetArray(p, $3);
  }
  |
  IDENTIFIER
  {
#ifdef DEBUG
    cerr << "Identifier Exp" << endl;
#endif
    $$ = new IdExp(p, $1);
  }
  |
  THIS
  {
#ifdef DEBUG
    cerr << "This Exp" << endl;
#endif
    $$ = new This(p);
  }
  |
  EXP ADD EXP
  {
#ifdef DEBUG
    cerr << "Add Exp" << endl;
#endif
    $$ = new BinaryOp(p, $1, new OpExp(p, "+"), $3);
  }
  |
  EXP MINUS EXP
  {
#ifdef DEBUG
    cerr << "Minus Exp" << endl;
#endif
    $$ = new BinaryOp(p, $1, new OpExp(p, "-"), $3);
  }
  |
  EXP TIMES EXP
  {
#ifdef DEBUG
    cerr << "Times Exp" << endl;
#endif
    $$ = new BinaryOp(p, $1, new OpExp(p, "*"), $3);
  }
  |
  EXP DIVIDE EXP
  {
#ifdef DEBUG
    cerr << "Divide Exp" << endl;
#endif
    $$ = new BinaryOp(p, $1, new OpExp(p, "/"), $3);
  }
  |
  EXP AND EXP
  {
#ifdef DEBUG
    cerr << "And Exp" << endl;
#endif
    $$ = new BinaryOp(p, $1, new OpExp(p, "&&"), $3);
  }
  |
  EXP OR EXP
  {
#ifdef DEBUG
    cerr << "Or Exp" << endl;
#endif
    $$ = new BinaryOp(p, $1, new OpExp(p, "||"), $3);
  }
  |
  EXP EQ EXP
  {
#ifdef DEBUG
    cerr << "Equal Exp" << endl;
#endif
    $$ = new BinaryOp(p, $1, new OpExp(p, "=="), $3);
  }
  |
  EXP NE EXP
  {
#ifdef DEBUG
    cerr << "Not Equal Exp" << endl;
#endif
    $$ = new BinaryOp(p, $1, new OpExp(p, "!="), $3);
  }
  |
  EXP LT EXP
  {
#ifdef DEBUG
    cerr << "Less Than Exp" << endl;
#endif
    $$ = new BinaryOp(p, $1, new OpExp(p, "<"), $3);
  }
  |
  EXP LE EXP
  {
#ifdef DEBUG
    cerr << "Less Equal Exp" << endl;
#endif
    $$ = new BinaryOp(p, $1, new OpExp(p, "<="), $3);
  }
  |
  EXP GT EXP
  {
#ifdef DEBUG
    cerr << "Greater Than Exp" << endl;
#endif
    $$ = new BinaryOp(p, $1, new OpExp(p, ">"), $3);
  }
  |
  EXP GE EXP
  {
#ifdef DEBUG
    cerr << "Greater Equal Exp" << endl;
#endif
    $$ = new BinaryOp(p, $1, new OpExp(p, ">="), $3);
  }
  |
  '!' EXP
  {
#ifdef DEBUG
    cerr << "Not Exp" << endl;
#endif
    $$ = new UnaryOp(p, new OpExp(p, "!"), $2);
  }
  |
  '-' EXP
  {
#ifdef DEBUG
    cerr << "Negate Exp" << endl;
#endif
    $$ = new UnaryOp(p, new OpExp(p, "-"), $2);
  }
  |
  '(' EXP ')'
  {
#ifdef DEBUG
    cerr << "Parenthesized Exp" << endl;
#endif
    $$ = $2;
  }
  |
  '(' '{' STMLIST '}' EXP ')'
  {
#ifdef DEBUG
    cerr << "Block Exp" << endl;
#endif
    $$ = new Esc(p, $3, $5);
  }
  |
  EXP '.' IDENTIFIER
  {
#ifdef DEBUG
    cerr << "Field Access Exp" << endl;
#endif
    $$ = new ClassVar(p, $1, $3);
  }
  |
  EXP '.' IDENTIFIER '(' EXPLIST ')'
  {
#ifdef DEBUG
    cerr << "Method Call Exp" << endl;
#endif
    $$ = new CallExp(p, $1, $3, $5);
  }
  |
  EXP '[' EXP ']'
  {
#ifdef DEBUG
    cerr << "Array Access Exp" << endl;
#endif
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
