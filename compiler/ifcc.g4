grammar ifcc;

axiom : prog EOF ;

prog : (decl_stmt)* 'int' 'main' '(' ')' block ;

block : '{' stmt* '}' ;

stmt 
    : decl_stmt              # DeclarationStatement
    | assign_stmt            # AssignmentStatement
    | expr ';'               # ExpressionStatement
    | return_stmt            # ReturnStatement
    | block                  # BlockStatement
    ;

decl_stmt : type VAR ('=' expr)? ';' ;  // Déclaration avec ou sans affectation
assign_stmt : VAR '=' expr ';' ;         // Affectation
return_stmt : 'return' expr ';' ;         // On retourne une expression

expr
    : op=('-'|'!') expr                             # UnaryLogicalNotExpression
    | expr OPM expr                                 # MulDivExpression
    | expr op=('+'|'-') expr                        # AddSubExpression
    | expr op=('=='|'!='|'<'|'>'|'<='|'>=') expr    # ComparisonExpression
    | expr op=('||'|'&&') expr                      # LogiqueParesseuxExpression
    | expr '&' expr                                 # BitwiseAndExpression
    | expr '^' expr                                 # BitwiseXorExpression
    | expr '|' expr                                 # BitwiseOrExpression
    | '(' expr ')'                                  # ParenthesisExpression
    | VAR '(' (expr (',' expr)*)? ')'               # FunctionCallExpression
    | VAR                                           # VariableExpression
    | CONST                                         # ConstantExpression
    | CONST_CHAR                                    # ConstantCharExpression
    ;

type : 'int' | 'char' ;

VAR   : [a-zA-Z_][a-zA-Z_0-9]* ;  // Identifiants pour les variables
CONST : [0-9]+ ;                 // Constantes entières
CONST_CHAR : '\''[ -~]'\'' ;

OPM: '*' | '/' | '%' ; // Opérateurs multiplicatifs 

COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
