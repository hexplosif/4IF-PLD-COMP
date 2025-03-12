grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' stmt* return_stmt '}' ;

stmt : declaration 
     | assignment
     ;

declaration : 'int' var_decl_list ';' ;

var_decl_list : var_decl (',' var_decl)* ;

var_decl : VAR ('=' expr)? ;

assignment : assign_expr (',' assign_expr)* ';' ;

// Assignment (lowest precedence)
assign_expr : VAR '=' expr ;

// New rule for return statement
return_stmt : RETURN expr ';' ;

// New top-level expression rule (non-assignment)
expr : additionExpr ;

// Addition/subtraction (left-associative)
additionExpr 
    : multiplicationExpr ( (op='+' | op='-') multiplicationExpr )* 
    ;

// Multiplication (left-associative)
multiplicationExpr 
    : primaryExpr ( (op='*') primaryExpr )* 
    ;

// Primary expressions
primaryExpr 
    : CONST
    | VAR
    | '(' expr ')'
    | assign_expr  // allow assignment as an expression
    ;

RETURN : 'return' ;
CONST : [0-9]+ ;
VAR   : [a-zA-Z_][a-zA-Z_0-9]* ;

COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
