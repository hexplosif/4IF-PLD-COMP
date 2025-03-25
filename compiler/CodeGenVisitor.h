#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "IR.h"

class CodeGenVisitor : public ifccBaseVisitor {
private:
    SymbolTable *currentScope = new SymbolTable(0); //  global scope

public:
    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;

    // ==============================================================
    //                          Statements
    // ==============================================================
   
    virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;
    virtual antlrcpp::Any visitDecl_stmt(ifccParser::Decl_stmtContext *ctx) override;
    virtual antlrcpp::Any visitSub_declWithType(ifccParser::Sub_declContext *ctx, std::string varType);
    virtual antlrcpp::Any visitAssignmentStatement(ifccParser::AssignmentStatementContext *ctx) override;
    virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
    
    // ==============================================================
    //                          Expressions
    // ==============================================================

    
    virtual antlrcpp::Any visitAddSubExpression(ifccParser::AddSubExpressionContext *ctx) override;
    virtual antlrcpp::Any visitMulDivExpression(ifccParser::MulDivExpressionContext *ctx) override;
    virtual antlrcpp::Any visitConstantExpression(ifccParser::ConstantExpressionContext *ctx) override;
    virtual antlrcpp::Any visitConstantCharExpression(ifccParser::ConstantCharExpressionContext *ctx) override;
    virtual antlrcpp::Any visitVariableExpression(ifccParser::VariableExpressionContext *ctx) override;
    virtual antlrcpp::Any visitParenthesisExpression(ifccParser::ParenthesisExpressionContext *ctx) override;
    virtual antlrcpp::Any visitComparisonExpression(ifccParser::ComparisonExpressionContext *ctx) override;
    virtual antlrcpp::Any visitBitwiseExpression(ifccParser::BitwiseExpressionContext *ctx) override;
    virtual antlrcpp::Any visitUnaryExpression(ifccParser::UnaryExpressionContext *ctx) override;
};
