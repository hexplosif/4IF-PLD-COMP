#pragma once

#include "generated/ifccBaseVisitor.h"
#include <iostream>
#include <map>
#include <string>

class CodeGenVisitor : public ifccBaseVisitor {
private:
    std::map<std::string, int> variables;
    int totalVars = 0;
    int currentDeclIndex = 0; // Compteur de déclaration (1ère déclaration, 2ème, etc.)

public:
    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;

    // ==============================================================
    //                          Statements
    // ==============================================================
    
    virtual antlrcpp::Any visitDecl_stmt(ifccParser::Decl_stmtContext *ctx) override;
    virtual antlrcpp::Any visitAssign_stmt(ifccParser::Assign_stmtContext *ctx) override;
    virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;

    // ==============================================================
    //                          Expressions
    // ==============================================================

    virtual antlrcpp::Any visitAddExpression(ifccParser::AddExpressionContext *ctx) override;
    virtual antlrcpp::Any visitSubExpression(ifccParser::SubExpressionContext *ctx) override;
    virtual antlrcpp::Any visitMulDivExpression(ifccParser::MulDivExpressionContext *ctx) override;
    virtual antlrcpp::Any visitVariableExpression(ifccParser::VariableExpressionContext *ctx) override;
    virtual antlrcpp::Any visitConstantExpression(ifccParser::ConstantExpressionContext *ctx) override;
    virtual antlrcpp::Any visitComparisonExpression(ifccParser::ComparisonExpressionContext *ctx) override;
};
