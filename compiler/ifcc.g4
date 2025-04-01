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
    | if_stmt                # IfStatement
    | while_stmt             # WhileStatement
    ;


decl_stmt : type sub_decl (',' sub_decl)* ';' ;     // Déclaration avec ou sans affectation
sub_decl : VAR ('=' expr)? ;                        // Sub-règle pour les déclarations
assign_stmt : VAR op_assign expr ';';                    // Affectation
op_assign: '=' | '+=' | '-=' | '*=' | '/=' | '%=';
return_stmt : 'return' expr ';' ;                   // On retourne une expression
if_stmt : 'if' '(' expr ')' stmt ('else' stmt)? ;   // If statement
while_stmt : 'while' '(' expr ')' stmt ;            // While statement

expr
    : VAR '++'							            # PostIncrementExpression
	| VAR '--'							            # PostDecrementExpression
    | op=('-'|'!') expr                             # UnaryExpression
    | expr OPM expr                                 # MulDivExpression
    | expr op=('+'|'-') expr                        # AddSubExpression
    | expr op=('=='|'!='|'<'|'>'|'<='|'>=') expr    # ComparisonExpression
    | expr op=('||'|'&&') expr                      # LogiqueParesseuxExpression
    | expr op=('&'|'^'|'|') expr                    # BitwiseExpression
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
