#pragma once

#define NOT_CONST_OPTI "$__not_constant__"

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "IR.h"
#include <vector>

// According to the Linux System V AMD64 ABI, the first six integer arguments go in:
// 1st: %rdi, 2nd: %rsi, 3rd: %rdx, 4th: %rcx, 5th: %r8, 6th: %r9.
// Here we assume that the result of an expression is left in %eax, so we move
// that 32-bit value into the corresponding register (using the "d" suffix for the lower 32 bits).
static std::vector<std::string> argRegs = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};

class CodeGenVisitor : public ifccBaseVisitor {
private:
    GVM* gvm; // Global Variable Manager
    std::vector<CFG*> cfgs; // List of CFGs
    CFG* currentCfg = nullptr; // Control Flow Graph
    
    Symbol* findVariable(std::string varName);
    std::string addTempConstVariable(std::string type, int value);
    void freeLastTempVariable(int inbVar);

    std::string constantOptimizeBinaryOp(std::string &left, std::string &right, IRInstr::Operation op);
    std::string constantOptimizeUnaryOp(std::string &left, IRInstr::Operation op);
    int getConstantResultBinaryOp(int leftValue, int rightValue, IRInstr::Operation op);
    int getConstantResultUnaryOp(int leftValue, IRInstr::Operation op);

    void enterNewScope();
    void exitCurrentScope();

public:
    void gen_asm(std::ostream& o);

    //================================ Program ===============================
    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
    virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;
    virtual antlrcpp::Any visitFunction_definition(ifccParser::Function_definitionContext *ctx) override;

    //=============================== Statements ===============================
    virtual antlrcpp::Any visitDecl_stmt(ifccParser::Decl_stmtContext *ctx) override;
    virtual antlrcpp::Any visitSub_declWithType(ifccParser::Sub_declContext *ctx, std::string varType);
    virtual antlrcpp::Any visitAssignmentStatement(ifccParser::AssignmentStatementContext *ctx) override;
    virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
    virtual antlrcpp::Any visitBlockStatement(ifccParser::BlockStatementContext *ctx) override;
    virtual antlrcpp::Any visitIf_stmt(ifccParser::If_stmtContext *ctx) override;
    virtual antlrcpp::Any visitWhile_stmt(ifccParser::While_stmtContext *ctx) override;

    // =============================== Expressions ===============================
    virtual antlrcpp::Any visitAddSubExpression(ifccParser::AddSubExpressionContext *ctx) override;
    virtual antlrcpp::Any visitMulDivExpression(ifccParser::MulDivExpressionContext *ctx) override;
    virtual antlrcpp::Any visitConstantExpression(ifccParser::ConstantExpressionContext *ctx) override;
    virtual antlrcpp::Any visitConstantCharExpression(ifccParser::ConstantCharExpressionContext *ctx) override;
    virtual antlrcpp::Any visitVariableExpression(ifccParser::VariableExpressionContext *ctx) override;
    virtual antlrcpp::Any visitParenthesisExpression(ifccParser::ParenthesisExpressionContext *ctx) override;
    virtual antlrcpp::Any visitComparisonExpression(ifccParser::ComparisonExpressionContext *ctx) override;
    virtual antlrcpp::Any visitBitwiseExpression(ifccParser::BitwiseExpressionContext *ctx) override;
    virtual antlrcpp::Any visitUnaryExpression(ifccParser::UnaryExpressionContext *ctx) override;
    virtual antlrcpp::Any visitLogiqueParesseuxExpression(ifccParser::LogiqueParesseuxExpressionContext *ctx) override;
    virtual antlrcpp::Any visitPostIncrementExpression(ifccParser::PostIncrementExpressionContext *ctx) override;
    virtual antlrcpp::Any visitPostDecrementExpression(ifccParser::PostDecrementExpressionContext *ctx) override;
    virtual antlrcpp::Any visitArrayAccessExpression(ifccParser::ArrayAccessExpressionContext *ctx) override;
    virtual antlrcpp::Any visitFunctionCallExpression(ifccParser::FunctionCallExpressionContext *ctx) override;
};
