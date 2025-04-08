#include "CodeGenVisitor.h"
#include "IR.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

/*
 * Pour cette implémentation, nous utilisons un CFG global (pour la fonction main).
 * En pratique, vous pouvez encapsuler cela dans une classe plus élaborée.
 */

extern std::map<std::string, DefFonction*> predefinedFunctions; /**< map of all functions in the program */

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx)
{
    // Création du GVM (Global Variable Manager)
    gvm = new GVM();
    cfgs = vector<CFG*>();

    for (auto child : ctx->children)
    {
        visit(child);
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunction_definition(ifccParser::Function_definitionContext *ctx)
{
    string funcType = ctx->retType()->getText();
    string funcName = ctx->VAR()->getText(); //TODO: verify if same name is declared twice
    DefFonction *funcDef = new DefFonction(funcName, Symbol::getType(funcType)); //TODO: modify to get the type of the function

    // On crée un CFG pour la fonction
    currentCfg = new CFG(funcDef);
    cfgs.push_back(currentCfg);
    currentCfg->currentScope = new SymbolTable(0);
    currentCfg->currentScope->setParent(gvm->getGlobalScope());

    // Création du bloc d'entrée
    BasicBlock* enterBlock = new BasicBlock(currentCfg, funcName);
    currentCfg->add_bb(enterBlock);
    currentCfg->current_bb = enterBlock;

    // Process the parameter list if present (copy arguments from registers to stack).
    if (ctx->parameterList())
    {
        int paramsSize = (ctx->parameterList()->parameter()).size();
        if (paramsSize > 6)
        {
            std::cerr << "error: function with more than 6 params is not supported! " << "\n";
            exit(1);
        }

        vector<VarType> paramsTypes = {};
        for (int i = 0; i < paramsSize; i ++)
        {
            auto param = ctx->parameterList()->parameter(i);
            string type = param->type()->getText();
            string paramName = param->VAR()->getText();

            VarType argType = Symbol::getType(type);
            currentCfg->currentScope->addLocalVariable(paramName, argType); //TODO: modify to get the type of the params
            currentCfg->current_bb->add_IRInstr(IRInstr::copy, argType, {paramName, argRegs[i]}); //TODO: modify to get the type of the params
            paramsTypes.push_back(argType);
        }

        funcDef->setParameters(paramsTypes);
    }

    // On crée le bloc de début de la fonction
    BasicBlock *bbStart = new BasicBlock(currentCfg, currentCfg->new_BB_name());
    currentCfg->add_bb(bbStart);
    enterBlock->exit_true = bbStart;
    currentCfg->current_bb = bbStart;

    // On traite le bloc de la fonction
    this->visit(ctx->block());

    // On crée le bloc de fin de la fonction
    BasicBlock *bbEnd = new BasicBlock(currentCfg, currentCfg->get_epilogue_label());
    currentCfg->add_bb(bbEnd);
    currentCfg->current_bb->exit_true = bbEnd;

    currentCfg = nullptr; // On remet le CFG à nullptr pour éviter les erreurs de traitement
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitBlock(ifccParser::BlockContext *ctx)
{
    // Visiter les statements
    for (auto stmt : ctx->stmt())
    {
        // if (cfg->current_bb == nullptr)
        //     break; // Un return a été rencontré : on arrête le traitement du bloc.
        this->visit(stmt);
    }
    return 0;
}

// ==============================================================
//                          Statements
// ==============================================================

antlrcpp::Any CodeGenVisitor::visitDecl_stmt(ifccParser::Decl_stmtContext *ctx)
{
    string varType = ctx->type()->getText();
    for (auto sdecl : ctx->sub_decl())
    {
        visitSub_declWithType(sdecl, varType);
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitSub_declWithType(ifccParser::Sub_declContext *ctx, string varType)
{
    VarType type = Symbol::getType(varType);
    bool isGlobalScope = (currentCfg == nullptr);
    string varName = ctx->VAR()->getText();

    if (isGlobalScope)
    {
        gvm->addGlobalVariable(varName, type);

        if (ctx->expr())
        {
            string exprCst = any_cast<string>(visit(ctx->expr()));
            Symbol *exprCstSymbol = findVariable(exprCst);

            if (!exprCstSymbol->isConstant()) {
                std::cerr << "error: global variable " << varName << " must be initialized with a constant expression.\n";
                exit(1);
            }

            if (!SymbolTable::isTypeCompatible(exprCstSymbol->type, type)) {
                std::cerr << "error: type mismatch for global variable " << varName << ". Expected " << varType << ", got " << Symbol::getTypeStr(exprCstSymbol->type) << ".\n";
                exit(1);
            }

            gvm->setGlobalVariableValue(varName, exprCstSymbol->getCstValue());
        }
    }
    else
    {
        if (ctx->CONST() != nullptr)
        { // C'est un tableau
            int size = std::stoi(ctx->CONST()->getText());
            currentCfg->currentScope->addLocalVariable(varName, type, size);
        }
        else
        {
            currentCfg->currentScope->addLocalVariable(varName, type);
            if (ctx->expr()) {
                string expr = any_cast<string>(visit(ctx->expr()));
                Symbol *exprSymbol = findVariable(expr);
                
                if (!SymbolTable::isTypeCompatible(exprSymbol->type, type)) {
                    std::cerr << "error: type mismatch for variable " << varName << ". Expected " << varType << ", got " << Symbol::getTypeStr(exprSymbol->type) << ".\n";
                    exit(1);
                }

                currentCfg->current_bb->add_IRInstr(IRInstr::copy, type, {varName, expr});
                
                if (exprSymbol->isConstant()) {
                    freeLastTempVariable(1);
                }
            }
        }
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssignmentStatement(ifccParser::AssignmentStatementContext *ctx)
{
    // Récupération du contexte de la règle assign_stmt
    auto assign = ctx->assign_stmt();

    string varName = assign->VAR()->getText();
    Symbol *var = findVariable(varName);
    VarType type = var->type;
    if (var == nullptr) {
        std::cerr << "error: variable " << varName << " not declared.\n";
        exit(1);
    }

    // On suppose que la variable a déjà été déclarée
    string op = assign->op_assign()->getText();
    size_t equalsPos = ctx->getText().find('=');
    if (equalsPos != std::string::npos && ctx->getText().substr(0, equalsPos).find('[') != std::string::npos)
    { // C'est un tableau
        string pos = any_cast<string>(this->visit(assign->expr(0)));
        string exprResult = any_cast<string>(this->visit(assign->expr(1)));

        Symbol *exprSymbol = findVariable(exprResult);
        if (!SymbolTable::isTypeCompatible(exprSymbol->type, type)) {
            std::cerr << "error: type mismatch for variable " << varName << ". Expected " << Symbol::getTypeStr(type) << ", got " << Symbol::getTypeStr(exprSymbol->type) << ".\n";
            exit(1);
        }

        if (op == "=")
        {
            currentCfg->current_bb->add_IRInstr(IRInstr::copyTblx, type, {varName, exprResult, pos});
        }
        else if (op == "+=")
        {
            currentCfg->current_bb->add_IRInstr(IRInstr::addTblx, type, {varName, exprResult, pos});
        }
        else if (op == "-=")
        {
            currentCfg->current_bb->add_IRInstr(IRInstr::subTblx, type, {varName, exprResult, pos});
        }
        else if (op == "*=")
        {
            currentCfg->current_bb->add_IRInstr(IRInstr::mulTblx, type, {varName, exprResult, pos});
        }
        else if (op == "/=")
        {
            currentCfg->current_bb->add_IRInstr(IRInstr::copy, type, {"%esi", exprResult});
            currentCfg->current_bb->add_IRInstr(IRInstr::divTblx, type, {varName, "%esi", pos});
        }
        else if (op == "%=")
        {
            currentCfg->current_bb->add_IRInstr(IRInstr::copy, type, {"%esi", exprResult});
            currentCfg->current_bb->add_IRInstr(IRInstr::modTblx, type, {varName, "%esi", pos});
        }

        if (exprSymbol->isConstant()) {
            freeLastTempVariable(1);
        }
    }
    else
    {
        string exprResult = any_cast<string>(this->visit(assign->expr(0)));
        Symbol *exprSymbol = findVariable(exprResult);

        if (!SymbolTable::isTypeCompatible(exprSymbol->type, type)) {
            std::cerr << "error: type mismatch for variable " << varName << ". Expected " << Symbol::getTypeStr(type) << ", got " << Symbol::getTypeStr(exprSymbol->type) << ".\n";
            exit(1);
        }

        if (op == "=")
        {
            currentCfg->current_bb->add_IRInstr(IRInstr::copy, type, {varName, exprResult});
        }
        else if (op == "+=")
        {
            currentCfg->current_bb->add_IRInstr(IRInstr::add, type, {varName, varName, exprResult});
        }
        else if (op == "-=")
        {
            currentCfg->current_bb->add_IRInstr(IRInstr::sub, type, {varName, varName, exprResult});
        }
        else if (op == "*=")
        {
            currentCfg->current_bb->add_IRInstr(IRInstr::mul, type, {varName, varName, exprResult});
        }
        else if (op == "/=")
        {
            currentCfg->current_bb->add_IRInstr(IRInstr::copy, type, {"%ecx", exprResult});
            currentCfg->current_bb->add_IRInstr(IRInstr::div, type, {varName, varName, "%ecx"});
        }
        else if (op == "%=")
        {
            currentCfg->current_bb->add_IRInstr(IRInstr::copy, type, {"%ecx", exprResult});
            currentCfg->current_bb->add_IRInstr(IRInstr::mod, type, {varName, varName, "%ecx"});
        }

        if (exprSymbol->isConstant()) {
            freeLastTempVariable(1);
        }
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    if (ctx->expr() == nullptr)
    {
        // Si la fonction est de type void, on ne fait rien
        if (currentCfg->ast->getType() != VarType::VOID)
        {
            std::cerr << "error: function " << currentCfg->ast->getName() << " must return a value.\n";
            exit(1);
        }
        currentCfg->current_bb->add_IRInstr(IRInstr::jmp, VarType::INT, {currentCfg->get_epilogue_label()});
        return 0;
    }

    // Évalue l'expression de retour
    std::string exprResult = any_cast<std::string>(this->visit(ctx->expr()));
    Symbol *exprSymbol = findVariable(exprResult);

    VarType funcReturnType = currentCfg->ast->getType();
    if (!SymbolTable::isTypeCompatible(exprSymbol->type, funcReturnType)) {
        if (funcReturnType == VarType::VOID) {
            std::cerr << "error: function " << currentCfg->ast->getName() << " can not return a value.\n";
            exit(1);
        }
        std::cerr << "error: type mismatch for return value for function " << currentCfg->ast->getName() << ". Expected " << Symbol::getTypeStr(funcReturnType) << ", got " << Symbol::getTypeStr(exprSymbol->type) << ".\n";
        exit(1);
    }

    currentCfg->current_bb->add_IRInstr(IRInstr::copy, funcReturnType, {"%eax", exprResult});

    if (exprSymbol->isConstant()) {
        freeLastTempVariable(1);
    }
    
    // Ajoute un saut vers l'épilogue
    currentCfg->current_bb->add_IRInstr(IRInstr::jmp, VarType::INT, {currentCfg->get_epilogue_label()});
    return exprResult;
}

// ==============================================================
//                          Expressions
// ==============================================================

antlrcpp::Any CodeGenVisitor::visitAddSubExpression(ifccParser::AddSubExpressionContext *ctx)
{
    string left = any_cast<string>(visit(ctx->expr(0)));
    string right = any_cast<string>(visit(ctx->expr(1)));

    IRInstr::Operation op;
    if (ctx->op->getText() == "+")
    {
        op = IRInstr::Operation::add;
    }
    else if (ctx->op->getText() == "-")
    {
        op = IRInstr::Operation::sub;
    }

    // Verifie si les deux opérandes sont des constantes
    std::string constOpt = constantOptimizeBinaryOp(left, right, op);
    if ( constOpt != NOT_CONST_OPTI) {
        return constOpt;
    }

    VarType type = getTypeExpr(left, right);
    string tmp = currentCfg->currentScope->addTempVariable(type);
    currentCfg->current_bb->add_IRInstr(op, type, {tmp, left, right});
    return tmp;
}

antlrcpp::Any CodeGenVisitor::visitMulDivExpression(ifccParser::MulDivExpressionContext *ctx)
{
    string left = any_cast<string>(visit(ctx->expr(0)));
    string right = any_cast<string>(visit(ctx->expr(1)));

    IRInstr::Operation op;
    if (ctx->OPM()->getText() == "*")
    {
        op = IRInstr::Operation::mul;
    }
    else if (ctx->OPM()->getText() == "/")
    {
        op = IRInstr::Operation::div;
    }
    else if (ctx->OPM()->getText() == "%")
    {
        op = IRInstr::Operation::mod;
    }

    // Verifie si les deux opérandes sont des constantes
    std::string constOpt = constantOptimizeBinaryOp(left, right, op);
    if ( constOpt != NOT_CONST_OPTI) {
        return constOpt;
    }
    
    VarType type = getTypeExpr(left, right);
    string tmp = currentCfg->currentScope->addTempVariable(type);
    if (op == IRInstr::Operation::div || op == IRInstr::Operation::mod)
    {
        currentCfg->current_bb->add_IRInstr(IRInstr::copy, type, {"%ecx", right});
        currentCfg->current_bb->add_IRInstr(op, type, {tmp, left, "%ecx"});
    } else {
        currentCfg->current_bb->add_IRInstr(op, type, {tmp, left, right});
    }
    return tmp;
}

antlrcpp::Any CodeGenVisitor::visitConstantExpression(ifccParser::ConstantExpressionContext *ctx)
{
    int value = std::stoi(ctx->CONST()->getText());
    string temp = addTempConstVariable(VarType::INT, value);
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitConstantCharExpression(ifccParser::ConstantCharExpressionContext *ctx)
{
    // Traitement des constantes de type char (par exemple, 'a')
    string value = ctx->CONST_CHAR()->getText().substr(1, 1);
    int ascii = (int)value[0];
    // Crée une variable temporaire et charge la constante dedans.
    string temp = addTempConstVariable(VarType::CHAR, ascii);
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitVariableExpression(ifccParser::VariableExpressionContext *ctx)
{
    // On retourne simplement le nom de la variable.
    string varName = ctx->VAR()->getText();
    Symbol *var = findVariable(varName);

    if (var == nullptr)
    {
        std::cerr << "error: variable " << varName << " not declared\n";
        exit(1);
    }

    return varName;
}

antlrcpp::Any CodeGenVisitor::visitParenthesisExpression(ifccParser::ParenthesisExpressionContext *ctx)
{
    return this->visit(ctx->expr());
}

antlrcpp::Any CodeGenVisitor::visitComparisonExpression(ifccParser::ComparisonExpressionContext *ctx)
{
    // expr op=('=='|'<'|'<='|'>'|'>=') expr
    string left = any_cast<string>(this->visit(ctx->expr(0)));
    string right = any_cast<string>(this->visit(ctx->expr(1)));

    VarType type = getTypeExpr(left, right);
    string temp = currentCfg->currentScope->addTempVariable(type);

    string op = ctx->op->getText();
    IRInstr::Operation cmpOp;
    if (op == "==") {
        cmpOp = IRInstr::cmp_eq;
    } else if (op == "!=") {
        cmpOp = IRInstr::cmp_ne;
    } else if (op == "<") {
        cmpOp = IRInstr::cmp_lt;
    } else if (op == "<=") {
        cmpOp = IRInstr::cmp_le;
    } else if (op == ">") {
        cmpOp = IRInstr::cmp_gt;
    } else if (op == ">=") {
        cmpOp = IRInstr::cmp_ge;
    }

    // Verifie si les deux opérandes sont des constantes
    std::string constOpt = constantOptimizeBinaryOp(left, right, cmpOp);
    if ( constOpt != NOT_CONST_OPTI) {
        return constOpt;
    }

    currentCfg->current_bb->add_IRInstr(cmpOp, type, {temp, left, right});
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitBitwiseExpression(ifccParser::BitwiseExpressionContext *ctx)
{
    // expr op=('&'|'|'|'^') expr
    string left = any_cast<string>(this->visit(ctx->expr(0)));
    string right = any_cast<string>(this->visit(ctx->expr(1)));
    
    VarType type = getTypeExpr(left, right);
    string temp = currentCfg->currentScope->addTempVariable(type);

    string op = ctx->op->getText();
    IRInstr::Operation bitOp;
    if (op == "&") {
        bitOp = IRInstr::bit_and;
    } else if (op == "|") {
        bitOp = IRInstr::bit_or;
    } else if (op == "^") {
        bitOp = IRInstr::bit_xor;
    }

    // Verifie si les deux opérandes sont des constantes
    std::string constOpt = constantOptimizeBinaryOp(left, right, bitOp);
    if ( constOpt != NOT_CONST_OPTI) {
        return constOpt;
    }

    currentCfg->current_bb->add_IRInstr(bitOp, type, {temp, left, right});
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitUnaryExpression(ifccParser::UnaryExpressionContext *ctx)
{
    // op=('-'|'!') expr
    string expr = any_cast<string>(this->visit(ctx->expr()));
    Symbol *exprSymbol = findVariable(expr);

    VarType type = exprSymbol->type;
    string temp = currentCfg->currentScope->addTempVariable(type);

    string op = ctx->op->getText();
    IRInstr::Operation unaryOp;
    if (op == "-") {
        unaryOp = IRInstr::unary_minus;
    } else if (op == "!") {
        unaryOp = IRInstr::not_op;
    }

    // Verifie si l'opérande est une constante
    std::string constOpt = constantOptimizeUnaryOp(expr, unaryOp);
    if (constOpt != NOT_CONST_OPTI) {
        return constOpt;
    }

    currentCfg->current_bb->add_IRInstr(unaryOp, type, {temp, expr});

    return temp;
}

antlrcpp::Any CodeGenVisitor::visitPostIncrementExpression(ifccParser::PostIncrementExpressionContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    Symbol *var = findVariable(varName);
    if (var == nullptr) {
        std::cerr << "error: variable " << varName << " not declared.\n";
        exit(1);
    }
    if (var->isConstant()) {
        std::cerr << "error: cannot increment a constant variable " << varName << ".\n";
        exit(1);
    }

    currentCfg->current_bb->add_IRInstr(IRInstr::incr, var->type, {varName});
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitPostDecrementExpression(ifccParser::PostDecrementExpressionContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    Symbol *var = findVariable(varName);
    if (var == nullptr) {
        std::cerr << "error: variable " << varName << " not declared.\n";
        exit(1);
    }
    if (var->isConstant()) {
        std::cerr << "error: cannot decrement a constant variable " << varName << ".\n";
        exit(1);
    }

    currentCfg->current_bb->add_IRInstr(IRInstr::decr, var->type, {varName});
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitArrayAccessExpression(ifccParser::ArrayAccessExpressionContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    Symbol *var = findVariable(varName);

    string pos = any_cast<string>(this->visit(ctx->expr()));
    Symbol *posSymbol = findVariable(pos);
    if (posSymbol->type != VarType::INT) {
        std::cerr << "error: array index must be an integer.\n";
        exit(1);
    }
    
    VarType type = var->type;
    string temp = currentCfg->currentScope->addTempVariable(type);
    currentCfg->current_bb->add_IRInstr(IRInstr::getTblx, type, {temp, varName, pos});
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitFunctionCallExpression(ifccParser::FunctionCallExpressionContext *ctx)
{
    // Get the function name from the VAR token
    std::string funcName = ctx->VAR()->getText();
    DefFonction *funcDef = getAstFunction(funcName);
    if (funcDef == nullptr)
    {
        std::cerr << "error: function " << funcName << " not declared.\n";
        exit(1);
    }

    // Get the list of argument expressions
    auto args = ctx->expr();
    if (args.size() > 6)
    {
        std::cerr << "Error: function call with more than 6 arguments not supported." << std::endl;
        exit(1);
    }

    // Evaluate each argument and move the result into the appropriate register.
    vector<std::string> argsExpr = vector<std::string>();
    for (size_t i = 0; i < args.size(); i++)
    {
        argsExpr.push_back(any_cast<std::string>(this->visit(args[i])));
    }

    vector<VarType> funcArgTypes = funcDef->getParameters();
    int funcArgCount = funcArgTypes.size();
    for (size_t i = 0; i < args.size(); i++)
    {
        // Check if the number of arguments is correct
        if (i >= funcArgCount)
        {
            std::cerr << "error: too many arguments for function " << funcName << ".\n";
            exit(1);
        }

        // Check if the argument type matches the function parameter type
        std::string arg = argsExpr[i];
        Symbol *argSymbol = findVariable(arg);
        if (!SymbolTable::isTypeCompatible(argSymbol->type, funcArgTypes[i]))
        {
            std::cerr << "error: type mismatch for argument " << i + 1 << " of function " << funcName << ". Expected " << Symbol::getTypeStr(funcArgTypes[i]) << ", got " << Symbol::getTypeStr(argSymbol->type) << ".\n";
            exit(1);
        }

        currentCfg->current_bb->add_IRInstr(IRInstr::copy, funcArgTypes[i], {argRegs[i], arg});
    }

    VarType funcReturnType = funcDef->getType();
    string temp = currentCfg->currentScope->addTempVariable(funcReturnType);
    currentCfg->current_bb->add_IRInstr(IRInstr::call, funcReturnType, {funcName, temp});

    return temp;
}

antlrcpp::Any CodeGenVisitor::visitLogiqueParesseuxExpression(ifccParser::LogiqueParesseuxExpressionContext *ctx)
{
    // expr op=('&&'|'||') expr
    string left = any_cast<string>(this->visit(ctx->expr(0)));
    string right = any_cast<string>(this->visit(ctx->expr(1)));
    
    VarType type = getTypeExpr(left, right);
    string temp = currentCfg->currentScope->addTempVariable(type);

    string op = ctx->op->getText();
    IRInstr::Operation logOp;
    if (op == "&&")
    {
        logOp = IRInstr::log_and;
    }
    else if (op == "||")
    {
        logOp = IRInstr::log_or;
    }
    
    // Verifie si les deux opérandes sont des constantes
    std::string constOpt = constantOptimizeBinaryOp(left, right, logOp);
    if ( constOpt != NOT_CONST_OPTI) {
        return constOpt;
    }

    currentCfg->current_bb->add_IRInstr(logOp, type, {temp, left, right});

    return temp;
}

antlrcpp::Any CodeGenVisitor::visitIf_stmt(ifccParser::If_stmtContext *ctx)
{
    bool hasElseStmt = ctx->stmt().size() > 1;

    // Évalue la condition
    std::string cond = any_cast<std::string>(visit(ctx->expr()));
    // TODO: Vérifier le type de la condition

    BasicBlock *tmp = currentCfg->current_bb->exit_true;
    // Crée un nouveau bloc pour la suite (join)
    std::string joinLabel = currentCfg->new_BB_name();
    BasicBlock *join_bb = new BasicBlock(currentCfg, joinLabel);
    currentCfg->add_bb(join_bb);

    BasicBlock *current = currentCfg->current_bb;
    current->test_var_name = cond;

    // Création des blocs then et else
    std::string thenLabel = currentCfg->new_BB_name();
    BasicBlock *then_bb = new BasicBlock(currentCfg, thenLabel);
    currentCfg->add_bb(then_bb);

    // Crée le bloc pour la branche "else"
    BasicBlock *else_bb = join_bb;
    if (hasElseStmt)
    {
        std::string elseLabel = currentCfg->new_BB_name();
        else_bb = new BasicBlock(currentCfg, elseLabel);
        currentCfg->add_bb(else_bb);
        else_bb->exit_true = join_bb;
    }

    // Connecter les blocs
    join_bb->exit_true = tmp;
    then_bb->exit_true = join_bb;
    currentCfg->current_bb->exit_true = then_bb;
    currentCfg->current_bb->exit_false = else_bb;

    // Configure le bloc courant pour effectuer un saut conditionnel
    currentCfg->current_bb->test_var_name = cond;
    currentCfg->current_bb->test_var_register = currentCfg->IR_reg_to_asm(cond);

    // Génère la branche "then"
    currentCfg->current_bb = then_bb;
    this->visit(ctx->stmt(0));
    // then_bb->add_IRInstr(IRInstr::jmp, VarType::INT, {joinLabel});

    // Génère la branche "else"
    if (hasElseStmt)
    {
        currentCfg->current_bb = else_bb;
        this->visit(ctx->stmt(1));
        // else_bb->add_IRInstr(IRInstr::jmp, VarType::INT, {joinLabel});
    }

    // Le code après le if se trouve dans join_bb
    currentCfg->current_bb = join_bb;
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitWhile_stmt(ifccParser::While_stmtContext *ctx)
{
    // Crée un bloc pour évaluer la condition
    std::string condLabel = currentCfg->new_BB_name();
    BasicBlock *cond_bb = new BasicBlock(currentCfg, "cond" + condLabel);
    currentCfg->add_bb(cond_bb);
    currentCfg->current_bb->exit_true = cond_bb;
    // cfg->current_bb->add_IRInstr(IRInstr::jmp, VarType::INT, {condLabel}); // Ajoute un saut du bloc courant vers le
    // bloc de condition

    // Génère le bloc de condition
    currentCfg->current_bb = cond_bb;
    std::string cond = any_cast<std::string>(this->visit(ctx->expr()));
    // TODO: Vérifier le type de la condition
    cond_bb->test_var_name = cond;
    cond_bb->test_var_register = currentCfg->IR_reg_to_asm(cond);

    // Crée le bloc du corps de la boucle
    std::string bodyLabel = currentCfg->new_BB_name();
    BasicBlock *body_bb = new BasicBlock(currentCfg, "body" + bodyLabel);
    currentCfg->add_bb(body_bb);

    // Crée le bloc de sortie (après la boucle)
    std::string joinLabel = currentCfg->new_BB_name();
    BasicBlock *join_bb = new BasicBlock(currentCfg, "join" + joinLabel);
    currentCfg->add_bb(join_bb);

    // La condition détermine le chemin
    cond_bb->exit_true = body_bb;
    cond_bb->exit_false = join_bb;
    body_bb->exit_true = cond_bb;

    // Génère le corps de la boucle
    currentCfg->current_bb = body_bb;
    this->visit(ctx->stmt());

    // La suite du code se trouve dans join_bb
    currentCfg->current_bb = join_bb;
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitBlockStatement(ifccParser::BlockStatementContext *ctx)
{
    enterNewScope();
    visit(ctx->block());
    exitCurrentScope();
    return 0;
}

// ==============================================================
//                          Constants Optimization
// ==============================================================
std::string CodeGenVisitor::constantOptimizeBinaryOp(std::string &left, std::string &right, IRInstr::Operation op) {
    //TODO: only support int for now, add other types later
    Symbol *leftSymbol = findVariable(left);
    Symbol *rightSymbol = findVariable(right);

    if (leftSymbol->isConstant() && rightSymbol->isConstant()) {
        // Si les deux opérandes sont des constantes, on peut évaluer l'expression directement.
        int resultValue = getConstantResultBinaryOp(leftSymbol->getCstValue(), rightSymbol->getCstValue(), op);
        freeLastTempVariable(2);

        return addTempConstVariable(VarType::INT, resultValue);
    } 

    // if (leftSymbol->isConstant()) {
    //     cfg->current_bb->add_IRInstr(IRInstr::ldconst, VarType::INT, { left, left });
    // } 
    
    // if (rightSymbol->isConstant()) {
    //     cfg->current_bb->add_IRInstr(IRInstr::ldconst, VarType::INT, { right, right});
    // }

    return NOT_CONST_OPTI;
}

int CodeGenVisitor::getConstantResultBinaryOp(int leftValue, int rightValue, IRInstr::Operation op) {
    switch (op) {
        case IRInstr::Operation::add:
            return leftValue + rightValue;
        case IRInstr::Operation::sub:
            return leftValue - rightValue;
        case IRInstr::Operation::mul:
            return leftValue * rightValue;
        case IRInstr::Operation::div:
            return leftValue / rightValue;
        case IRInstr::Operation::mod:
            return leftValue % rightValue;

        case IRInstr::Operation::cmp_eq:
            return (leftValue == rightValue) ? 1 : 0;
        case IRInstr::Operation::cmp_ne:
            return (leftValue != rightValue) ? 1 : 0;
        case IRInstr::Operation::cmp_lt:
            return (leftValue < rightValue) ? 1 : 0;
        case IRInstr::Operation::cmp_le:
            return (leftValue <= rightValue) ? 1 : 0;
        case IRInstr::Operation::cmp_gt:
            return (leftValue > rightValue) ? 1 : 0;
        case IRInstr::Operation::cmp_ge:
            return (leftValue >= rightValue) ? 1 : 0;

        case IRInstr::Operation::bit_and:
            return leftValue & rightValue;
        case IRInstr::Operation::bit_or:
            return leftValue | rightValue;
        case IRInstr::Operation::bit_xor:
            return leftValue ^ rightValue;

        case IRInstr::Operation::log_and:
            return (leftValue && rightValue) ? 1 : 0;
        case IRInstr::Operation::log_or:
            return (leftValue || rightValue) ? 1 : 0;

        default:
            std::cerr << "error: unsupported operation for constant optimization\n";
            exit(1);
    }
}

std::string CodeGenVisitor::constantOptimizeUnaryOp(std::string &expr, IRInstr::Operation op) {
    //TODO: only support int for now, add other types later
    Symbol *exprSymbol = findVariable(expr);

    if (exprSymbol->isConstant()) {
        // Si l'opérande est une constante, on peut évaluer l'expression directement.
        int resultValue = getConstantResultUnaryOp(exprSymbol->getCstValue(), op);
        freeLastTempVariable(1);
        return addTempConstVariable(VarType::INT, resultValue);
    }

    return NOT_CONST_OPTI;
}

int CodeGenVisitor::getConstantResultUnaryOp(int cstValue, IRInstr::Operation op) {
    switch (op) {
        case IRInstr::Operation::unary_minus:
            return -cstValue;
        case IRInstr::Operation::not_op:
            return !cstValue;
        default:
            std::cerr << "error: unsupported operation for constant optimization\n";
            exit(1);
    }
}

// ==============================================================
//                          Others
// ==============================================================
Symbol* CodeGenVisitor::findVariable(std::string varName)
{
    if (currentCfg == nullptr) {   // Si on est dans le contexte global
        return gvm->getGlobalScope()->findVariable(varName);
    } else {                // Si on est dans le contexte d'une fonction
        return currentCfg->currentScope->findVariable(varName);
    }
}

void CodeGenVisitor::freeLastTempVariable(int nbVar = 1)
{
    for (int i = 0; i < nbVar; i++) {
        if (currentCfg != nullptr) {
            currentCfg->currentScope->freeLastTempVariable();
        } else {
            gvm->getGlobalScope()->freeLastTempVariable();
        }
    }
}

std::string CodeGenVisitor::addTempConstVariable(VarType type, int value)
{
    if (currentCfg != nullptr) {
        return currentCfg->currentScope->addTempConstVariable(type, value);
    } else {
        return gvm->addTempConstVariable(type, value);
    }
}

void CodeGenVisitor::enterNewScope()
{
    // Creer un nouveau scope quand on entre un nouveau block
    SymbolTable *parentScope = currentCfg->currentScope;
    currentCfg->currentScope = new SymbolTable(parentScope->getCurrentDeclOffset());
    currentCfg->currentScope->setParent(parentScope);
}

void CodeGenVisitor::exitCurrentScope()
{
    // le block est finis, on synchronize le parent et ce scope; on revient vers scope de parent
    currentCfg->currentScope->getParent()->synchronize(currentCfg->currentScope);
    currentCfg->currentScope = currentCfg->currentScope->getParent();
}

void CodeGenVisitor::gen_asm(ostream &os)
{
    gvm->gen_asm(os);
    os << "\n//================================================ \n\n";
    for (auto &cfg : cfgs)
    {
        cfg->gen_asm(os);
        os << "\n//================================================= \n\n";
    }
}

VarType CodeGenVisitor::getTypeExpr(string left, string right) {
    VarType leftType = findVariable(left)->type;
    VarType rightType = findVariable(right)->type;

    if (!SymbolTable::isTypeCompatible(leftType, rightType)) {
        std::cerr << "error: type mismatch for addition/subtraction operation.\n";
        exit(1);
    }

    return SymbolTable::getHigherType(leftType, rightType);
}

DefFonction* CodeGenVisitor::getAstFunction(std::string name)
{
    for (auto &cfg : cfgs)
    {
        if (cfg->ast->getName() == name)
        {
            return cfg->ast;
        }
    }

    if (predefinedFunctions.find(name) != predefinedFunctions.end()) {
        return predefinedFunctions[name];
    }

    return nullptr;
}

