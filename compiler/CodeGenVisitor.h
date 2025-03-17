#pragma once

#include <iostream>
#include <map>
#include <string>

#include "generated/ifccBaseVisitor.h"
#include "SymbolTable.h"


class CodeGenVisitor : public ifccBaseVisitor
{
private:
    SymbolTable* currentScope = nullptr; //see SymbolTable.h

    // Fonction récursive pour compter les déclarations dans l'arbre
    int countDeclarations(antlr4::tree::ParseTree *tree);
    bool isExprIsConstant(ifccParser::ExprContext *ctx);
    int getConstantValueFromExpr(ifccParser::ExprContext *ctx);

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
    virtual antlrcpp::Any visitConstantCharExpression(ifccParser::ConstantCharExpressionContext *ctx) override;
    virtual antlrcpp::Any visitComparisonExpression(ifccParser::ComparisonExpressionContext *ctx) override;
    virtual antlrcpp::Any visitBitwiseAndExpression(ifccParser::BitwiseAndExpressionContext *ctx) override;
    virtual antlrcpp::Any visitBitwiseOrExpression(ifccParser::BitwiseOrExpressionContext *ctx) override;
    virtual antlrcpp::Any visitBitwiseXorExpression(ifccParser::BitwiseXorExpressionContext *ctx) override;
    virtual antlrcpp::Any visitUnaryLogicalNotExpression(ifccParser::UnaryLogicalNotExpressionContext *ctx) override;
    virtual antlrcpp::Any visitFunctionCallExpression(ifccParser::FunctionCallExpressionContext *ctx) override;
};
