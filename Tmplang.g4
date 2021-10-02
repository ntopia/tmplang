
grammar Tmplang;

file: (function)+ ;

function: 'fn' ID '(' functionParams? ')' block ;

functionParams: functionParamDecl (',' functionParamDecl)* ;

functionParamDecl: type ID ;

block: '{' (statement)* '}' ;

type: 'int' | 'float' | 'char' | 'bool';

statement
    : block                           # BlockStatement
    | 'let' type? ID ('=' expr)? ';'  # VarDeclStatement
    | 'return' expr? ';'              # ReturnStatement
    | expr ';'                        # NormalStatement
    ;

expr
    : expr '(' exprList ')'   # FunctionCallExpr
    | '-' expr              # NegateExpr
    | '!' expr              # NotExpr
    | expr ('*'|'/') expr   # MulDivExpr
    | expr ('+'|'-') expr   # PlusMinusExpr
    | expr '==' expr        # EqualExpr
    | ID                    # VarRefExpr
    | literal               # LiteralExpr
    | '(' expr ')'          # ParenExpr
    ;

literal
    : IntegerLiteral
    | CharacterLiteral
    ;

exprList: expr (',' expr)* ;


IntegerLiteral
    : DecimalLiteral
    | HexadecimalLiteral
    ;

DecimalLiteral: NONZERODIGIT (DIGIT)* ;
HexadecimalLiteral: '0x' (HEXADECIMALDIGIT)+ ;

CharacterLiteral
    : '\'' [^'\\\r\n] '\'' ;


ID: LETTER (LETTER | DIGIT)* ;

fragment
LETTER: [a-zA-Z_] ;
fragment
DIGIT: [0-9] ;
fragment
NONZERODIGIT: [1-9] ;
fragment
HEXADECIMALDIGIT: [0-9a-fA-F] ;

WS: [ \t\n\r]+ -> skip ;

SL_COMMENT: '//' .*? '\n' -> skip ;
