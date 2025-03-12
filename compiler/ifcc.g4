grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' stmt* return_stmt '}' ;

stmt : declaration 
     | assignment 
     ;

declaration : 'int' VAR ('=' expr)? ';' ;

assignment : VAR '=' expr ';' ;

return_stmt: 'return' expr ';' ;

expr : CONST 
     | VAR
     ;

RETURN : 'return' ;
CONST : [0-9]+ ;
VAR   : [a-zA-Z_][a-zA-Z_0-9]* ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
