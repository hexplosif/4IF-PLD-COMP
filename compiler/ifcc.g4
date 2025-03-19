grammar ifcc;

axiom: prog EOF;

prog: (decl_stmt)* 'int' 'main' '(' ')' block;

block: '{' stmt* '}';

stmt:
	decl_stmt		# DeclarationStatement
	| assign_stmt	# AssignmentStatement
	| expr ';'		# ExpressionStatement
	| return_stmt	# ReturnStatement
	| block			# BlockStatement;

decl_stmt:
	type sub_decl (',' sub_decl)* ';'; // Déclaration avec ou sans affectation
sub_decl: VAR ('=' expr)?; // Sub-règle pour les déclarations
assign_stmt: VAR op_assign expr ';';
op_assign: '=' | '+=' | '-=' | '*=' | '/=' | '%=';
return_stmt: 'return' expr ';'; // On retourne une expression

// réf: C Operator Precedence: https://en.cppreference.com/w/c/language/operator_precedence
expr:
	// Expression atomique:
	'(' expr ')'	# ParenthesisExpression
	| VAR			# VariableExpression
	| CONST			# ConstantExpression
	| CONST_CHAR	# ConstantCharExpression
	// Precedence 1:
	| op = ('-' | '!') expr				# UnaryLogicalNotExpression
	| VAR '++'							# PostIncrementExpression
	| VAR '--'							# PostDecrementExpression
	| VAR '(' (expr (',' expr)*)? ')'	# FunctionCallExpression
	// Precedence 2:

	// Precedence 3:
	| expr OPM expr # MulDivExpression
	// Precedence 4:
	| expr op = ('+' | '-') expr # AddSubExpression
	// Precedence 5:

	// Precedence 6,7:
	| expr op = ('==' | '!=' | '<' | '>' | '<=' | '>=') expr # ComparisonExpression
	// Precedence 8:
	| expr '&' expr # BitwiseAndExpression
	// Precedence 9:
	| expr '^' expr # BitwiseXorExpression
	// Precedence 10:
	| expr '|' expr # BitwiseOrExpression
	// Precedence 11,12:
	| expr op = ('||' | '&&') expr # LogiqueParesseuxExpression;

type: 'int' | 'char';

VAR: [a-zA-Z_][a-zA-Z_0-9]*; // Identifiants pour les variables
CONST: [0-9]+; // Constantes entières
CONST_CHAR: '\'' [ -~]'\'';

OPM: '*' | '/' | '%'; // Opérateurs multiplicatifs 

COMMENT: '/*' .*? '*/' -> skip;
DIRECTIVE: '#' .*? '\n' -> skip;
WS: [ \t\r\n] -> channel(HIDDEN);