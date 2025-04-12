grammar ifcc;

axiom : prog EOF ;

prog : (decl_stmt | function_definition)*;

// Function
function_definition
    : retType VAR '(' parameterList? ')' block
    ;

retType : type | 'void' ;
parameterList : parameter (',' parameter)* ;
parameter : type VAR ;

block : '{' stmt* '}' ;

// Statements

stmt 
    : decl_stmt              # DeclarationStatement
    | assign_stmt            # AssignmentStatement
    | expr ';'               # ExpressionStatement
    | return_stmt            # ReturnStatement
    | if_stmt                # IfStatement
    | block                  # BlockStatement
    | while_stmt             # WhileStatement
    ;


decl_stmt : type sub_decl (',' sub_decl)* ';' ;         // Déclaration avec ou sans affectation
sub_decl : (VAR ('=' expr)?) | (VAR '[' CONST ']' ('=' ('{' expr(',' expr)* '}' | CONST_STRING) )?);         // Sub-règle pour les déclarations
assign_stmt : (VAR | VAR '[' expr ']') op_assign expr ';';                   // Affectation
op_assign: '=' | '+=' | '-=' | '*=' | '/=' | '%=';
return_stmt : 'return' (expr)? ';' ;                       // On retourne une expression
if_stmt : 'if' '(' expr ')' stmt ('else' stmt)? ;       // If statement
while_stmt : 'while' '(' expr ')' stmt ;                // While statement

// Expressions

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
    | VAR '[' expr ']'                              # ArrayAccessExpression
    | CONST                                         # ConstantExpression
    | CONST_DOUBLE                                  # ConstantDoubleExpression
    | CONST_CHAR                                    # ConstantCharExpression
    | CONST_STRING                                  # ConstantStringExpression
    | VAR '++'                                      # PostIncrementExpression
    | VAR '--'                                      # PostDecrementExpression
    ;

type : 'int' | 'char' | 'float' ;

VAR   : [a-zA-Z_][a-zA-Z_0-9]* ;  // Identifiants pour les variables
CONST : [0-9]+ ;                 // Constantes entières
CONST_DOUBLE : [0-9]+ '.' [0-9]+ ('f')?; // Constantes flottantes
CONST_CHAR : '\'' [ -~] '\'' ;
CONST_STRING : '"' ([ -~])*? '"' ;

OPM: '*' | '/' | '%' ; // Opérateurs multiplicatifs 

COMMENT : '/*' .*? '*/' -> skip ;
COMMENT_LINE : '//' .*? '\n' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
