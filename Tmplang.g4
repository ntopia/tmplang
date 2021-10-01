
grammar Tmplang;

file: (function)+ ;

function: 'fn' ID '(' functionParams? ')' block ;

functionParams: functionParamDecl (',' functionParamDecl)* ;

functionParamDecl: type ID ;

block: '{' (statement)* '}' ;

type: 'int' | 'float' | 'char' ;

statement
    : block                           # BlockStatement
    | 'let' type? ID ('=' expr)? ';'  # VarDeclStatement
    | 'return' expr? ';'              # ReturnStatement
    | expr ';'                        # NormalStatement
    ;

expr
    : ID '(' exprList ')'   # FunctionCallExpr
    | '-' expr              # NegateExpr
    | '!' expr              # NotExpr
    | expr ('*'|'/') expr   # MulDivExpr
    | expr ('+'|'-') expr   # PlusMinusExpr
    | expr '==' expr        # EqualExpr
    | ID                    # VarRefExpr
    | '(' expr ')'          # ParenExpr
    ;

exprList: expr (',' expr)* ;




ID: LETTER (LETTER | DIGIT)* ;

fragment
LETTER: [a-zA-Z] ;
DIGIT: [0-9];

WS: [ \t\n\r]+ -> skip ;

SL_COMMENT: '//' .*? '\n' -> skip ;
