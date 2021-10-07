
grammar Tmplang;

file: (function)+ ;

function: 'fn' identifier '(' functionParams? ')' functionReturnTypeDecl? blockStatement ;

functionParams: functionParamDecl (',' functionParamDecl)* ;

functionParamDecl: type identifier ;

functionReturnTypeDecl: ':' type ;

type: 'int' | 'float' | 'char' | 'bool';

statement
    : ifStatement
    | blockStatement
    | varDeclStatement
    | returnStatement
    | assignStatement
    | normalStatement
    ;

blockStatement
    : '{' (statement)* '}'
    ;

ifStatement
    : 'if' '(' expr ')' blockStatement
      (
        'else' (blockStatement | ifStatement)
      )?
    ;

varDeclStatement
    : 'let' type? identifier ('=' expr)? ';'
    ;

assignStatement
    : identifier '=' expr ';'
    ;

returnStatement
    : 'return' expr? ';'
    ;

normalStatement
    : expr ';'
    ;

expr
    : expr '(' exprList? ')' # FunctionCallExpr
    | '-' expr               # NegateExpr
    | '!' expr               # NotExpr
    | expr ('*'|'/') expr    # MulDivExpr
    | expr ('+'|'-') expr    # PlusMinusExpr
    | expr '==' expr         # EqualExpr
    | identifier             # VarRefExpr
    | literal                # LiteralExpr
    | '(' expr ')'           # ParenExpr
    ;

literal
    : IntegerLiteral
    | CharacterLiteral
    | BoolLiteral
    ;

exprList: expr (',' expr)* ;

identifier: ID ;


IntegerLiteral
    : DecimalLiteral
    | HexadecimalLiteral
    ;

DecimalLiteral: NONZERODIGIT (DIGIT)* ;
HexadecimalLiteral: '0x' (HEXADECIMALDIGIT)+ ;

CharacterLiteral
    : '\'' ~['\\\r\n] '\'' ;

BoolLiteral: ('true' | 'false');


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
