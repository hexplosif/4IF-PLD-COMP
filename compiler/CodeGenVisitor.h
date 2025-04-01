#pragma once

#include <iostream>
#include <map>
#include <string>

#include "SymbolTable.h"
#include "generated/ifccBaseVisitor.h"

class CodeGenVisitor : public ifccBaseVisitor
{
  private:
    SymbolTable *currentScope = new SymbolTable(0); //  global scope
    // Fonction récursive pour compter les déclarations dans l'arbre
    int countDeclarations(antlr4::tree::ParseTree *tree);

    bool isExprIsConstant(ifccParser::ExprContext *ctx);
    int getConstantValueFromExpr(ifccParser::ExprContext *ctx);

    std::string generateLabel();

  public:
    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;

    // ==============================================================
    //                          Statements
    // ==============================================================

    virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;
    virtual antlrcpp::Any visitDecl_stmt(ifccParser::Decl_stmtContext *ctx) override;
    virtual antlrcpp::Any visitSub_declWithType(ifccParser::Sub_declContext *ctx, std::string varType);
    virtual antlrcpp::Any visitAssign_stmt(ifccParser::Assign_stmtContext *ctx) override;
    virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;

    // ==============================================================
    //                          Expressions
    // ==============================================================

    // Expressions atomiques:
    virtual antlrcpp::Any visitVariableExpression(ifccParser::VariableExpressionContext *ctx) override;
    virtual antlrcpp::Any visitConstantExpression(ifccParser::ConstantExpressionContext *ctx) override;
    virtual antlrcpp::Any visitConstantCharExpression(ifccParser::ConstantCharExpressionContext *ctx) override;
    virtual antlrcpp::Any visitArrayAccessExpression(ifccParser::ArrayAccessExpressionContext *ctx) override;
    // Precedence 1:
    virtual antlrcpp::Any visitUnaryLogicalNotExpression(ifccParser::UnaryLogicalNotExpressionContext *ctx) override;
    virtual antlrcpp::Any visitPostIncrementExpression(ifccParser::PostIncrementExpressionContext *ctx) override;
    virtual antlrcpp::Any visitPostDecrementExpression(ifccParser::PostDecrementExpressionContext *ctx) override;
    virtual antlrcpp::Any visitFunctionCallExpression(ifccParser::FunctionCallExpressionContext *ctx) override;
    // Precedence 3:
    virtual antlrcpp::Any visitMulDivExpression(ifccParser::MulDivExpressionContext *ctx) override;
    // Precedence 4:
    virtual antlrcpp::Any visitAddSubExpression(ifccParser::AddSubExpressionContext *ctx) override;
    // Precedence 6,7:
    virtual antlrcpp::Any visitComparisonExpression(ifccParser::ComparisonExpressionContext *ctx) override;
    // Precedence 8:
    virtual antlrcpp::Any visitBitwiseAndExpression(ifccParser::BitwiseAndExpressionContext *ctx) override;
    // Precedence 9:
    virtual antlrcpp::Any visitBitwiseXorExpression(ifccParser::BitwiseXorExpressionContext *ctx) override;
    // Precedence 10:
    virtual antlrcpp::Any visitBitwiseOrExpression(ifccParser::BitwiseOrExpressionContext *ctx) override;
    // Precedence 11,12:
    virtual antlrcpp::Any visitLogiqueParesseuxExpression(ifccParser::LogiqueParesseuxExpressionContext *ctx) override;
};
