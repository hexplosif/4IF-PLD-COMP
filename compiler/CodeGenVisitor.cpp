#include "CodeGenVisitor.h"
#include "IR.h"
#include "type.h"
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
    for(auto stmt : ctx->stmt()) {
        this->visit(stmt);
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclarationStatement(ifccParser::DeclarationStatementContext *ctx)
{
    // Récupération du contexte de la règle decl_stmt
    auto decl = ctx->decl_stmt();
    string varType = decl->type()->getText(); // "int" ou "char"
    Type t = (varType == "int") ? Type::INT : Type::CHAR;
    
    // Pour chaque sous-déclaration, on ajoute la variable dans la table des symboles
    // et, si initialisée, on génère une instruction d'affectation.
    for (auto sub : decl->sub_decl()) {
        string varName = sub->VAR()->getText();
        cfg->add_to_symbol_table(varName, t);
        if(sub->expr()) {
            // Évalue l'expression et récupère le nom de la variable temporaire contenant le résultat
            string res = this->visit(sub->expr()).as<string>();
            cfg->current_bb->add_IRInstr(IRInstr::copy, t, {varName, res});
        }
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssignmentStatement(ifccParser::AssignmentStatementContext *ctx)
{
    // Récupération du contexte de la règle assign_stmt
    auto assign = ctx->assign_stmt();
    string varName = assign->VAR()->getText();
    // On suppose que la variable a déjà été déclarée
    string exprResult = this->visit(assign->expr()).as<string>();
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {varName, exprResult});
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    // return_stmt : 'return' expr ';'
    // Évalue l'expression de retour
    string exprResult = this->visit(ctx->expr()).as<string>();
    // Pour la gestion du retour, on copie le résultat dans une variable spéciale "ret".
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {"ret", exprResult});
    return exprResult;
}

antlrcpp::Any CodeGenVisitor::visitAddSubExpression(ifccParser::AddSubExpressionContext *ctx)
{
    // expr op=('+'|'-') expr
    string left = this->visit(ctx->expr(0)).as<string>();
    string right = this->visit(ctx->expr(1)).as<string>();
    string temp = cfg->create_new_tempvar(Type::INT);
    string op = ctx->op->getText();
    if(op == "+") {
        cfg->current_bb->add_IRInstr(IRInstr::add, Type::INT, {temp, left, right});
    } else {
        cfg->current_bb->add_IRInstr(IRInstr::sub, Type::INT, {temp, left, right});
    }
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitMulDivExpression(ifccParser::MulDivExpressionContext *ctx)
{
    // expr OPM expr – pour l'instant, seule la multiplication est supportée.
    string left = this->visit(ctx->expr(0)).as<string>();
    string right = this->visit(ctx->expr(1)).as<string>();
    string temp = cfg->create_new_tempvar(Type::INT);
    string op = ctx->OPM()->getText();
    if(op == "*") {
        cfg->current_bb->add_IRInstr(IRInstr::mul, Type::INT, {temp, left, right});
    }
    else if(op == "/") {
        // Pour simplifier, on ne gère pas la division ici.
        cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {temp, left});
    }
    else if(op == "%") {
        // Non implémenté.
        cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {temp, left});
    }
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitConstantExpression(ifccParser::ConstantExpressionContext *ctx)
{
    // Traitement des constantes (par exemple, "17")
    string value = ctx->CONST()->getText();
    // Crée une variable temporaire et charge la constante dedans.
    string temp = cfg->create_new_tempvar(Type::INT);
    cfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {temp, value});
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitVariableExpression(ifccParser::VariableExpressionContext *ctx)
{
    // On retourne simplement le nom de la variable.
    return ctx->VAR()->getText();
}

antlrcpp::Any CodeGenVisitor::visitParenthesisExpression(ifccParser::ParenthesisExpressionContext *ctx)
{
    return this->visit(ctx->expr());
}
