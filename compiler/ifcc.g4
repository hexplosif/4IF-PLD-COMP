grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' block ;

block : '{' stmt* '}' ;

stmt 
    : decl_stmt              # DeclarationStatement
    | assign_stmt            # AssignmentStatement
    | expr ';'               # ExpressionStatement
    | return_stmt            # ReturnStatement
    | block                  # BlockStatement
    ;

decl_stmt : 'int' VAR ('=' expr)? ';' ;  // Déclaration avec ou sans affectation
assign_stmt : VAR '=' expr ';' ;         // Affectation
return_stmt : 'return' expr ';' ;         // On retourne une expression

expr 
    : op=('-'|'!') expr                             # UnaryLogicalNotExpression
    | expr '*' expr                                 # MulExpression
    | expr op=('+'|'-') expr                        # AddSubExpression
    | expr op=('=='|'!='|'<'|'>'|'<='|'>=') expr     # ComparisonExpression
    | expr '&' expr                                 # BitwiseAndExpression
    | expr '^' expr                                 # BitwiseXorExpression
    | expr '|' expr                                 # BitwiseOrExpression
    | '(' expr ')'                                  # ParenthesisExpression
    | VAR                                           # VariableExpression
    | CONST                                         # ConstantExpression
    ;

VAR   : [a-zA-Z_][a-zA-Z_0-9]* ;  // Identifiants pour les variables
CONST : [0-9]+ ;                 // Constantes entières

COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
