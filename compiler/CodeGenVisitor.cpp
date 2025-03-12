#include "CodeGenVisitor.h"
#include <map>
#include <iostream>

// The symbol table maps variable names to their stack offsets.
std::map<std::string, int> CodeGenVisitor::symbolTable;

// We'll use the static stackOffset variable to track the next available offset.
int CodeGenVisitor::stackOffset = 0;

// Helper function to count all local variables in the program.
static int countVariables(ifccParser::ProgContext *ctx)
{
    int count = 0;
    for (size_t i = 0; i < ctx->stmt().size(); i++)
    {
        std::string stmtText = ctx->stmt(i)->getText();
        if (stmtText.substr(0, 3) == "int")
        { // Declaration starts with "int"
            ifccParser::DeclarationContext *decl = dynamic_cast<ifccParser::DeclarationContext *>(ctx->stmt(i));
            if (decl)
            {
                count += decl->var_decl_list()->var_decl().size();
            }
        }
    }
    return count;
}

//--------------------------------------------------------
// Declaration: allocate stack space for each variable.
// We assume that all variable declarations have been counted so that we
// can set the starting offset to -(totalVars * 4).
antlrcpp::Any CodeGenVisitor::visitDeclaration(ifccParser::DeclarationContext *ctx)
{
    auto varDecls = ctx->var_decl_list()->var_decl();
    // For each variable in the declaration, assign the current offset.
    for (auto varDecl : varDecls)
    {
        std::string varName = varDecl->VAR()->getText();
        int offset = stackOffset;
        symbolTable[varName] = offset;
        // If there is an initializer, generate code to compute it and store it.
        if (varDecl->expr())
        {
            visit(varDecl->expr());
            std::cout << "    movl %eax, " << offset << "(%rbp)" << std::endl;
        }
        stackOffset += 4;
    }
    return 0;
}

//--------------------------------------------------------
// Assignment: visit each assign_expr in a comma-separated assignment.
antlrcpp::Any CodeGenVisitor::visitAssignment(ifccParser::AssignmentContext *ctx)
{
    for (auto assignExpr : ctx->assign_expr())
    {
        visit(assignExpr);
    }
    return 0;
}

//--------------------------------------------------------
// Assignment expression visitor (chain assignments):
// Evaluate the right-hand side and then store the result (in %eax)
// into the left-hand variable’s memory location.
antlrcpp::Any CodeGenVisitor::visitAssign_expr(ifccParser::Assign_exprContext *ctx)
{
    visit(ctx->expr());
    std::string lhs = ctx->VAR()->getText();
    std::cout << "    movl %eax, " << symbolTable[lhs] << "(%rbp)" << std::endl;
    return 0;
}

//--------------------------------------------------------
// Return statement: evaluate the expression to be returned.
antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    visit(ctx->expr());
    return 0;
}

//--------------------------------------------------------
// Expression: delegate evaluation to the additionExpr rule.
antlrcpp::Any CodeGenVisitor::visitExpr(ifccParser::ExprContext *ctx)
{
    return visit(ctx->additionExpr());
}

//--------------------------------------------------------
// Visit addition expressions: handles '+' and '-' operators.
antlrcpp::Any CodeGenVisitor::visitAdditionExpr(ifccParser::AdditionExprContext *ctx)
{
    visit(ctx->multiplicationExpr(0));
    for (size_t i = 1; i < ctx->multiplicationExpr().size(); i++)
    {
        std::cout << "    pushq %rax\n";
        visit(ctx->multiplicationExpr(i));
        std::cout << "    popq %rcx\n";
        std::string op = ctx->op[i - 1].getText();
        if (op == "+")
        {
            std::cout << "    addl %ecx, %eax\n";
        }
        else if (op == "-")
        {
            std::cout << "    subl %eax, %ecx\n";
            std::cout << "    movl %ecx, %eax\n";
        }
    }
    return 0;
}

//--------------------------------------------------------
// Visit multiplication expressions: handles '*' operator.
antlrcpp::Any CodeGenVisitor::visitMultiplicationExpr(ifccParser::MultiplicationExprContext *ctx)
{
    visit(ctx->primaryExpr(0));
    for (size_t i = 1; i < ctx->primaryExpr().size(); i++)
    {
        std::cout << "    pushq %rax\n";
        visit(ctx->primaryExpr(i));
        std::cout << "    popq %rcx\n";
        std::cout << "    imull %ecx, %eax\n";
    }
    return 0;
}

//--------------------------------------------------------
// Visit primary expressions.
antlrcpp::Any CodeGenVisitor::visitPrimaryExpr(ifccParser::PrimaryExprContext *ctx)
{
    if (ctx->CONST())
    {
        int value = std::stoi(ctx->CONST()->getText());
        std::cout << "    movl $" << value << ", %eax\n";
    }
    else if (ctx->VAR())
    {
        std::string varName = ctx->VAR()->getText();
        std::cout << "    movl " << symbolTable[varName] << "(%rbp), %eax\n";
    }
    else if (ctx->expr())
    {
        visit(ctx->expr());
    }
    else if (ctx->assign_expr())
    {
        visit(ctx->assign_expr());
    }
    return 0;
}

//--------------------------------------------------------
// Generate code for the entire program.
antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx)
{
    int totalVars = countVariables(ctx);
    // Set stackOffset so that the first declared variable is at -totalVars*4.
    stackOffset = -totalVars * 4;

    std::cout << ".globl main\n";
    std::cout << " main: \n";

    // Prologue.
    std::cout << "    pushq %rbp\n";
    std::cout << "    movq %rsp, %rbp\n";
    std::cout << "    subq $" << (totalVars * 4) << ", %rsp\n";

    for (auto stmt : ctx->stmt())
    {
        visit(stmt);
    }
    visit(ctx->return_stmt());

    // Epilogue.
    std::cout << "    popq %rbp\n";
    std::cout << "    ret\n";

    return 0;
}
