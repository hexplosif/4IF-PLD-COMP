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
    for (auto stmt : ctx->stmt()) {
        if (cfg->current_bb == nullptr)
            break; // Un return a été rencontré : on arrête le traitement du bloc.
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
            string res = any_cast<string>(this->visit(sub->expr()));
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
    string exprResult = any_cast<string>(this->visit(assign->expr()));
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {varName, exprResult});
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    // Évalue l'expression de retour
    std::string exprResult = any_cast<std::string>(this->visit(ctx->expr()));
    // Copie le résultat dans la variable spéciale "ret"
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {"ret", exprResult});
    // Ajoute un saut vers l'épilogue
    cfg->current_bb->add_IRInstr(IRInstr::jmp, Type::INT, {".Lepilogue"});
    // Marquer le bloc courant comme terminé en le vidant (ou en le désactivant)
    cfg->current_bb = nullptr; // On n'ajoutera plus d'instructions dans ce bloc.
    return exprResult;
}

antlrcpp::Any CodeGenVisitor::visitAddSubExpression(ifccParser::AddSubExpressionContext *ctx)
{
    // expr op=('+'|'-') expr
    string left = any_cast<string>(this->visit(ctx->expr(0)));
    string right = any_cast<string>(this->visit(ctx->expr(1)));
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
    string left = any_cast<string>(this->visit(ctx->expr(0)));
    string right = any_cast<string>(this->visit(ctx->expr(1)));
    string temp = cfg->create_new_tempvar(Type::INT);
    string op = ctx->OPM()->getText();
    if(op == "*") {
        cfg->current_bb->add_IRInstr(IRInstr::mul, Type::INT, {temp, left, right});
    }
    else if(op == "/") {
        cfg->current_bb->add_IRInstr(IRInstr::div, Type::INT, {temp, left, right});
    }
    else if(op == "%") {
        cfg->current_bb->add_IRInstr(IRInstr::mod, Type::INT, {temp, left, right});
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

antlrcpp::Any CodeGenVisitor::visitConstantCharExpression(ifccParser::ConstantCharExpressionContext *ctx)
{
    // Traitement des constantes de type char (par exemple, 'a')
    string value = ctx->CONST_CHAR()->getText().substr(1, 1);
    int ascii = (int)value[0];
    // Crée une variable temporaire et charge la constante dedans.
    string temp = cfg->create_new_tempvar(Type::CHAR);
    cfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::CHAR, {temp, to_string(ascii)});
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

antlrcpp::Any CodeGenVisitor::visitComparisonExpression(ifccParser::ComparisonExpressionContext *ctx)
{
    // expr op=('=='|'<'|'<='|'>'|'>=') expr
    string left = any_cast<string>(this->visit(ctx->expr(0)));
    string right = any_cast<string>(this->visit(ctx->expr(1)));
    string temp = cfg->create_new_tempvar(Type::INT);
    string op = ctx->op->getText();
    if(op == "==") {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_eq, Type::INT, {temp, left, right});
    } else if (op == "!=") {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_ne, Type::INT, {temp, left, right});
    } else if(op == "<") {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_lt, Type::INT, {temp, left, right});
    } else if(op == "<=") {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_le, Type::INT, {temp, left, right});
    } else if(op == ">") {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_gt, Type::INT, {temp, left, right});
    } else if(op == ">=") {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_ge, Type::INT, {temp, left, right});
    }
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitBitwiseExpression(ifccParser::BitwiseExpressionContext *ctx)
{
    // expr op=('&'|'|'|'^') expr
    string left = any_cast<string>(this->visit(ctx->expr(0)));
    string right = any_cast<string>(this->visit(ctx->expr(1)));
    string temp = cfg->create_new_tempvar(Type::INT);
    string op = ctx->op->getText();
    if(op == "&") {
        cfg->current_bb->add_IRInstr(IRInstr::bit_and, Type::INT, {temp, left, right});
    } else if(op == "|") {
        cfg->current_bb->add_IRInstr(IRInstr::bit_or, Type::INT, {temp, left, right});
    } else if(op == "^") {
        cfg->current_bb->add_IRInstr(IRInstr::bit_xor, Type::INT, {temp, left, right});
    }
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitUnaryExpression(ifccParser::UnaryExpressionContext *ctx)
{
    // op=('-'|'!') expr
    string expr = any_cast<string>(this->visit(ctx->expr()));
    string temp = cfg->create_new_tempvar(Type::INT);
    string op = ctx->op->getText();
    if(op == "-") {
        cfg->current_bb->add_IRInstr(IRInstr::unary_minus, Type::INT, {temp, expr});
    } else if(op == "!") {
        cfg->current_bb->add_IRInstr(IRInstr::not_op, Type::INT, {temp, expr});
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
        cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {argRegs[i], arg});
    }

    // Call the function
    cfg->current_bb->add_IRInstr(IRInstr::call, Type::INT, {funcName});

    // The result of the function call is left in %eax, so we move it to a temporary variable.
    std::string temp = cfg->create_new_tempvar(Type::INT);
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {temp, "%eax"});

    return temp;
}
