#pragma once

#include "generated/ifccBaseVisitor.h"
#include <iostream>
#include <map>
#include <string>

class CodeGenVisitor : public ifccBaseVisitor {
private:
    // On utilise une table globale pour simplifier, mais sans gestion de scopes
    std::map<std::string, int> variables;
    int totalVars = 0;
    int currentDeclIndex = 0; // Compteur pour le calcul des offsets

    // Fonction récursive pour compter les déclarations dans l'arbre
    int countDeclarations(antlr4::tree::ParseTree *tree);

public:
    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;

    // ==============================================================
    //                          Statements
    // ==============================================================
    
    virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;
    virtual antlrcpp::Any visitDecl_stmt(ifccParser::Decl_stmtContext *ctx) override;
    virtual antlrcpp::Any visitAssign_stmt(ifccParser::Assign_stmtContext *ctx) override;
    virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;

    // ==============================================================
    //                          Expressions
    // ==============================================================

    virtual antlrcpp::Any visitAddSubExpression(ifccParser::AddSubExpressionContext *ctx) override;
    virtual antlrcpp::Any visitMulDivExpression(ifccParser::MulDivExpressionContext *ctx) override;
    virtual antlrcpp::Any visitVariableExpression(ifccParser::VariableExpressionContext *ctx) override;
    virtual antlrcpp::Any visitConstantExpression(ifccParser::ConstantExpressionContext *ctx) override;
    virtual antlrcpp::Any visitComparisonExpression(ifccParser::ComparisonExpressionContext *ctx) override;
    virtual antlrcpp::Any visitBitwiseAndExpression(ifccParser::BitwiseAndExpressionContext *ctx) override;
    virtual antlrcpp::Any visitBitwiseOrExpression(ifccParser::BitwiseOrExpressionContext *ctx) override;
    virtual antlrcpp::Any visitBitwiseXorExpression(ifccParser::BitwiseXorExpressionContext *ctx) override;
    virtual antlrcpp::Any visitUnaryLogicalNotExpression(ifccParser::UnaryLogicalNotExpressionContext *ctx) override;
};
