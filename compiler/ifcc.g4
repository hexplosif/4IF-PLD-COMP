grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' stmt* return_stmt '}' ;

stmt 
    : decl_stmt    # DeclarationStatement
    | assign_stmt  # AssignmentStatement
    | expr ';'     # ExpressionStatement
    ;

decl_stmt : type VAR ('=' expr)? ';' ;  // Déclaration avec ou sans affectation
assign_stmt : VAR '=' expr ';' ;         // Affectation

return_stmt: 'return' expr ';' ;  // On retourne une expression

expr 
    : expr '&' expr                                 # BitwiseAndExpression
    | expr '^' expr                                 # BitwiseXorExpression
    | expr '|' expr                                 # BitwiseOrExpression
    | expr '*' expr                                 # MulExpression
    | expr '+' expr                                 # AddExpression
    | expr '-' expr                                 # SubExpression
    | expr op=('=='|'!='|'<'|'>'|'<='|'>=') expr    # ComparisonExpression
    | '(' expr ')'                                  # ParenthesisExpression
    | VAR                                           # VariableExpression
    | CONST                                         # ConstantExpression
    | CONST_CHAR                                    # ConstantCharExpression
    ;

type : 'int' | 'char' ;

VAR   : [a-zA-Z_][a-zA-Z_0-9]* ;  // Identifiants pour les variables
CONST : [0-9]+ ;                 // Constantes entières
CONST_CHAR : '\''[ -~]'\'' ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);