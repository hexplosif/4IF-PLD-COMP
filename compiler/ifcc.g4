grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' stmt* return_stmt '}' ;

stmt : declaration 
     | assignment
     ;

declaration : 'int' var_decl_list ';' ;  // Liste de déclarations

var_decl_list : var_decl (',' var_decl)* ; // Séparées par des virgules

var_decl : VAR ('=' expr)? ;  // Une variable avec option d'initialisation

assignment : assign_expr (',' assign_expr)* ';' ; // Assignations multiples

assign_expr : VAR '=' expr ;  // Expression d'affectation

return_stmt: 'return' expr ';' ;

expr : CONST
     | VAR
     | assign_expr  // Autorise l'affectation en tant qu'expression
     ;

RETURN : 'return' ;
CONST : [0-9]+ ;
VAR   : [a-zA-Z_][a-zA-Z_0-9]* ;

COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
