#include "CodeGenVisitor.h"
#include "IR.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
using namespace std;

/*
 * Pour cette implémentation, nous utilisons un CFG global (pour la fonction main).
 * En pratique, vous pouvez encapsuler cela dans une classe plus élaborée.
 */
CFG* cfg = nullptr;

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) 
{
    // Création du GVM (Global Variable Manager)
    gvm = new GVM();
    
    // Le programme est : (decl_stmt)* 'int' 'main' '(' ')' block
    // On traite ici les déclarations globales
    for (auto decl : ctx->decl_stmt())
    {
        this->visit(decl);
    }

    // Création du CFG pour main (l'AST de la fonction est ici nullptr pour simplifier)
    cfg = new CFG(nullptr);

    // On traite ici le bloc principal
    this->visit(ctx->block());
    
    // Pour le return, la génération IR a lieu dans visitReturn_stmt.
    
    // Une fois le CFG construit, on génère le code assembleur.
    gvm->gen_asm(std::cout);
    cfg->gen_asm(std::cout);
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitBlock(ifccParser::BlockContext *ctx)
{
    bool isThisFunctionBlock = (cfg->currentScope == nullptr);

    // TODO: if this block is a function block, we need to create a new cfg for the function

    // Creer un nouveau scope quand on entre un nouveau block
    SymbolTable *parentScope = isThisFunctionBlock ? gvm->getGlobalScope() : cfg->currentScope;
    cfg->currentScope = new SymbolTable(parentScope->getCurrentDeclOffset());
    cfg->currentScope->setParent(parentScope);

    // Créer un nouveau basic block
    string blockName = cfg->new_BB_name();
    BasicBlock* bb = new BasicBlock(cfg, blockName);
    cfg->add_bb(bb);

    // Visiter les statements
    for (auto stmt : ctx->stmt()) {
        if (cfg->current_bb == nullptr)
            break; // Un return a été rencontré : on arrête le traitement du bloc.
        this->visit(stmt);
    }

    // le block est finis, on synchronize le parent et ce scope; on revient vers scope de parent
    cfg->currentScope->getParent()->synchronize(cfg->currentScope);
    cfg->currentScope = cfg->currentScope->getParent();

    if (isThisFunctionBlock) { // si ce bloc est une fonction
        // Ajouter un bloc de fin pour la fonction
        BasicBlock* bbEnd = new BasicBlock(cfg, ".Lepilogue");
        cfg->add_bb(bbEnd);
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDecl_stmt(ifccParser::Decl_stmtContext *ctx)
{
    string varType = ctx->type()->getText();
    for (auto sdecl : ctx->sub_decl())
    {
        visitSub_declWithType(sdecl, varType);
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitSub_declWithType(ifccParser::Sub_declContext *ctx, string varType) {
    bool isGlobalScope = (cfg==nullptr);
    string varName = ctx->VAR()->getText();

    if (isGlobalScope) {
        gvm->addGlobalVariable(varName, varType);

        if (ctx->expr()) {
            string exprCst = any_cast<string>(visit(ctx->expr()));
            Symbol *var = gvm->getGlobalScope()->findVariable(exprCst);
            gvm->setGlobalVariableValue(varName, var->getCstValue());
        }
    } else {
        cfg->currentScope->addLocalVariable(varName, varType);
        if (ctx->expr()) {
            string expr = any_cast<string>(visit(ctx->expr()));
            cfg->current_bb->add_IRInstr(IRInstr::Operation::copy, VarType::INT, {varName, expr});
        }
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssignmentStatement(ifccParser::AssignmentStatementContext *ctx)
{
    // Récupération du contexte de la règle assign_stmt
    auto assign = ctx->assign_stmt();
    string varName = assign->VAR()->getText();
    Symbol *var = cfg->currentScope->findVariable(varName);
    if (var == nullptr) {
        std::cerr << "error: variable " << varName << " not declared.\n";
        exit(1);
    }

    // On suppose que la variable a déjà été déclarée
    string exprResult = any_cast<string>(this->visit(assign->expr()));
    cfg->current_bb->add_IRInstr(IRInstr::copy, VarType::INT, {varName, exprResult});
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    // Évalue l'expression de retour
    std::string exprResult = any_cast<std::string>(this->visit(ctx->expr()));
    // Copie le résultat dans la variable spéciale "ret" ("ret" ne marche pas avec LINUX)
    cfg->current_bb->add_IRInstr(IRInstr::copy, VarType::INT, {"%eax", exprResult});
    // Ajoute un saut vers l'épilogue
    cfg->current_bb->add_IRInstr(IRInstr::jmp, VarType::INT, {".Lepilogue"});
    // Marquer le bloc courant comme terminé en le vidant (ou en le désactivant)
    cfg->current_bb = nullptr; // On n'ajoutera plus d'instructions dans ce bloc.
    return exprResult;
}

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
    
    string tmp = cfg->currentScope->addTempVariable("int");
    cfg->current_bb->add_IRInstr(op, VarType::INT, {tmp, left, right});
    return tmp;
}

antlrcpp::Any CodeGenVisitor::visitMulDivExpression(ifccParser::MulDivExpressionContext *ctx)
{
    string left = any_cast<string>(visit(ctx->expr(0)));
    string right = any_cast<string>(visit(ctx->expr(1)));

    IRInstr::Operation op;
    if (ctx->OPM()->getText() == "*") {
        op = IRInstr::Operation::mul;
    } else if (ctx->OPM()->getText() == "/") {
        op = IRInstr::Operation::div;
    } else if (ctx->OPM()->getText() == "%") {
        op = IRInstr::Operation::mod;
    }
    
    string tmp = cfg->currentScope->addTempVariable("int");
    cfg->current_bb->add_IRInstr(op, VarType::INT, {tmp, left, right});
    return tmp;
}

antlrcpp::Any CodeGenVisitor::visitConstantExpression(ifccParser::ConstantExpressionContext *ctx)
{
    int value = std::stoi(ctx->CONST()->getText());
    string temp;
    if (cfg != nullptr) { // Viste le contexte lorsqu'on est dans une fonction
        temp = cfg->currentScope->addTempConstVariable("int", value);
        cfg->current_bb->add_IRInstr(IRInstr::ldconst, VarType::CHAR, {temp, to_string(value)});
    } else {  // Si on est dans le contexte global
        temp = gvm->addTempConstVariable("int", value);
    }
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitConstantCharExpression(ifccParser::ConstantCharExpressionContext *ctx)
{
    // Traitement des constantes de type char (par exemple, 'a')
    string value = ctx->CONST_CHAR()->getText().substr(1, 1);
    int ascii = (int)value[0];
    // Crée une variable temporaire et charge la constante dedans.
    string temp;
    if (cfg != nullptr) { // Si on est dans le contexte d'une fonction
        temp = cfg->currentScope->addTempConstVariable("char", ascii);
        cfg->current_bb->add_IRInstr(IRInstr::ldconst, VarType::CHAR, {temp, to_string(ascii)});
    } else { // Si on est dans le contexte global
        temp = gvm->addTempConstVariable("char", ascii);
    }
    
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitVariableExpression(ifccParser::VariableExpressionContext *ctx)
{
    // On retourne simplement le nom de la variable.
    string varName = ctx->VAR()->getText();
    Symbol *var = cfg->currentScope->findVariable(varName);
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
    string temp = cfg->currentScope->addTempVariable("int");
    string op = ctx->op->getText();
    if(op == "==") {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_eq, VarType::INT, {temp, left, right});
    } else if (op == "!=") {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_ne, VarType::INT, {temp, left, right});
    } else if(op == "<") {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_lt, VarType::INT, {temp, left, right});
    } else if(op == "<=") {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_le, VarType::INT, {temp, left, right});
    } else if(op == ">") {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_gt, VarType::INT, {temp, left, right});
    } else if(op == ">=") {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_ge, VarType::INT, {temp, left, right});
    }
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitBitwiseExpression(ifccParser::BitwiseExpressionContext *ctx)
{
    // expr op=('&'|'|'|'^') expr
    string left = any_cast<string>(this->visit(ctx->expr(0)));
    string right = any_cast<string>(this->visit(ctx->expr(1)));
    string temp = cfg->currentScope->addTempVariable("int");
    string op = ctx->op->getText();
    if(op == "&") {
        cfg->current_bb->add_IRInstr(IRInstr::bit_and, VarType::INT, {temp, left, right});
    } else if(op == "|") {
        cfg->current_bb->add_IRInstr(IRInstr::bit_or, VarType::INT, {temp, left, right});
    } else if(op == "^") {
        cfg->current_bb->add_IRInstr(IRInstr::bit_xor, VarType::INT, {temp, left, right});
    }
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitUnaryExpression(ifccParser::UnaryExpressionContext *ctx)
{
    // op=('-'|'!') expr
    string expr = any_cast<string>(this->visit(ctx->expr()));
    string temp = cfg->currentScope->addTempVariable("int");
    string op = ctx->op->getText();
    if(op == "-") {
        cfg->current_bb->add_IRInstr(IRInstr::unary_minus, VarType::INT, {temp, expr});
    } else if(op == "!") {
        cfg->current_bb->add_IRInstr(IRInstr::not_op, VarType::INT, {temp, expr});
    }
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitFunctionCallExpression(ifccParser::FunctionCallExpressionContext *ctx)
{
    // Get the function name from the VAR token
    std::string funcName = ctx->VAR()->getText();

    // Get the list of argument expressions
    auto args = ctx->expr();

    // Check if the function call has more than 6 arguments.
    if (args.size() > 6)
    {
        std::cerr << "Error: function call with more than 6 arguments not supported." << std::endl;
        exit(1);
    }

    // Evaluate each argument and move the result into the appropriate register.
    // According to the Linux System V AMD64 ABI, the first six integer arguments go in:
    // 1st: %rdi, 2nd: %rsi, 3rd: %rdx, 4th: %rcx, 5th: %r8, 6th: %r9.
    // Here we assume that the result of an expression is left in %eax, so we move
    // that 32-bit value into the corresponding register (using the "d" suffix for the lower 32 bits).

    std::vector<std::string> argRegs = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};

    for (size_t i = 0; i < args.size(); i++)
    {
        std::string arg = any_cast<std::string>(this->visit(args[i]));
        cfg->current_bb->add_IRInstr(IRInstr::copy, VarType::INT, {argRegs[i], arg});
    }

    // Call the function
    cfg->current_bb->add_IRInstr(IRInstr::call, VarType::INT, {funcName});

    // The result of the function call is left in %eax, so we move it to a temporary variable.
    string temp = cfg->currentScope->addTempVariable("int");
    cfg->current_bb->add_IRInstr(IRInstr::copy, VarType::INT, {temp, "%eax"});

    return temp;
}
