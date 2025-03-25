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
    // Création du CFG pour main (l'AST de la fonction est ici nullptr pour simplifier)
    cfg = new CFG(nullptr);
    
    // Le programme est : (decl_stmt)* 'int' 'main' '(' ')' block
    // On traite ici le bloc principal
    this->visit(ctx->block());
    
    // Pour le return, la génération IR a lieu dans visitReturn_stmt.
    
    // Une fois le CFG construit, on génère le code assembleur.
    cfg->gen_asm(std::cout);
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitBlock(ifccParser::BlockContext *ctx)
{
    bool isThisFunctionBlock = currentScope->isGlobalScope();

    // TODO: if this block is a function block, we need to create a new cfg for the function

    // Creer un nouveau scope quand on entre un nouveau block
    SymbolTable *parentScope = currentScope;
    currentScope = new SymbolTable(parentScope->getCurrentDeclOffset());
    currentScope->setParent(parentScope);
    cfg->set_current_scope(currentScope);

    // Créer un nouveau basic block
    string blockName = cfg->new_BB_name();
    BasicBlock* bb = new BasicBlock(cfg, blockName);
    cfg->add_bb(bb);

    // Visiter les statements
    for(auto stmt : ctx->stmt()) {
        this->visit(stmt);
    }

    // le block est finis, on synchronize le parent et ce scope; on revient vers scope de parent
    currentScope->getParent()->synchronize(currentScope);
    currentScope = currentScope->getParent();

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
    string varName = ctx->VAR()->getText();

    if (currentScope->isGlobalScope()) {
        // TODO:
    } else {
        currentScope->addLocalVariable(varName, varType);
        if (ctx->expr()) {
            string expr = visit(ctx->expr()).as<string>();
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
    Symbol *var = currentScope->findVariable(varName);
    if (var == nullptr) {
        std::cerr << "error: variable " << varName << " not declared.\n";
        exit(1);
    }

    // On suppose que la variable a déjà été déclarée
    string exprResult = this->visit(assign->expr()).as<string>();
    cfg->current_bb->add_IRInstr(IRInstr::copy, VarType::INT, {varName, exprResult});
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    // return_stmt : 'return' expr ';'
    // Évalue l'expression de retour
    string exprResult = this->visit(ctx->expr()).as<string>();
    // Pour la gestion du retour, on copie le résultat dans une variable spéciale "ret".
    cfg->current_bb->add_IRInstr(IRInstr::copy, VarType::INT, {"%eax", exprResult});
    return exprResult;
}

antlrcpp::Any CodeGenVisitor::visitAddSubExpression(ifccParser::AddSubExpressionContext *ctx)
{
    string left = visit(ctx->expr(0)).as<string>();
    string right = visit(ctx->expr(1)).as<string>();

    IRInstr::Operation op;
    if (ctx->op->getText() == "+")
    {
        op = IRInstr::Operation::add;
    }
    else if (ctx->op->getText() == "-")
    {
        op = IRInstr::Operation::sub;
    }
    
    string tmp = currentScope->addTempVariable("int");
    cfg->current_bb->add_IRInstr(op, VarType::INT, {tmp, left, right});
    return tmp;
}

antlrcpp::Any CodeGenVisitor::visitMulDivExpression(ifccParser::MulDivExpressionContext *ctx)
{
    string left = visit(ctx->expr(0)).as<string>();
    string right = visit(ctx->expr(1)).as<string>();

    IRInstr::Operation op;
    if (ctx->OPM()->getText() == "*") {
        op = IRInstr::Operation::mul;
    } else if (ctx->OPM()->getText() == "/") {
        op = IRInstr::Operation::div;
    } else if (ctx->OPM()->getText() == "%") {
        op = IRInstr::Operation::rem;
    }
    
    string tmp = currentScope->addTempVariable("int");
    cfg->current_bb->add_IRInstr(op, VarType::INT, {tmp, left, right});
    return tmp;
}

antlrcpp::Any CodeGenVisitor::visitConstantExpression(ifccParser::ConstantExpressionContext *ctx)
{
    int value = std::stoi(ctx->CONST()->getText());
    string var = currentScope->addTempVariable("int");
    cfg->current_bb->add_IRInstr(IRInstr::Operation::ldconst, VarType::INT, {var, to_string(value)});
    return var;
}

antlrcpp::Any CodeGenVisitor::visitVariableExpression(ifccParser::VariableExpressionContext *ctx)
{
    // On retourne simplement le nom de la variable.
    string varName = ctx->VAR()->getText();
    Symbol *var = currentScope->findVariable(varName);
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
