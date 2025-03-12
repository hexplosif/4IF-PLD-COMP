#include "CodeGenVisitor.h"
#include <map>
#include <iostream>

std::map<std::string, int> CodeGenVisitor::symbolTable;
int CodeGenVisitor::stackOffset = 0;

//--------------------------------------------------------
// Declaration: unchanged, though you might consider using visit() for initializers.
antlrcpp::Any CodeGenVisitor::visitDeclaration(ifccParser::DeclarationContext *ctx)
{
    for (auto varDecl : ctx->var_decl_list()->var_decl())
    {
        std::string varName = varDecl->VAR()->getText();

        // Allocate space on the stack
        stackOffset -= 4;
        symbolTable[varName] = stackOffset;

        // Check if there is an initialization.
        if (varDecl->expr())
        {
            // Instead of manually checking constant/variable, we call visit on the expr.
            visit(varDecl->expr());
            // The result of the expression is assumed to be in %eax.
            std::cout << "    movl %eax, " << stackOffset << "(%rbp)" << std::endl;
        }
    }
    return 0;
}

//--------------------------------------------------------
// New visitor for assignment expressions (assign_expr)
// This will recursively handle nested (chain) assignments.
antlrcpp::Any CodeGenVisitor::visitAssign_expr(ifccParser::Assign_exprContext *ctx)
{
    // Evaluate the right-hand side of the assignment.
    // The expr can itself be an assign_expr (chain assignment).
    if (ctx->expr()->assign_expr() != nullptr)
    {
        visit(ctx->expr()->assign_expr());
    }
    else if (ctx->expr()->CONST() != nullptr)
    {
        int value = std::stoi(ctx->expr()->CONST()->getText());
        std::cout << "    movl $" << value << ", %eax" << std::endl;
    }
    else if (ctx->expr()->VAR() != nullptr)
    {
        std::string sourceVar = ctx->expr()->VAR()->getText();
        std::cout << "    movl " << symbolTable[sourceVar] << "(%rbp), %eax" << std::endl;
    }
    else
    {
        // Fallback in case expr is more complex
        visit(ctx->expr());
    }

    // Now store the value in %eax into the left-hand side variable.
    std::string lhs = ctx->VAR()->getText();
    std::cout << "    movl %eax, " << symbolTable[lhs] << "(%rbp)" << std::endl;
    return 0;
}

//--------------------------------------------------------
// Update assignment visitor to simply visit each assign_expr.
antlrcpp::Any CodeGenVisitor::visitAssignment(ifccParser::AssignmentContext *ctx)
{
    for (auto assignExpr : ctx->assign_expr())
    {
        visit(assignExpr); // This will invoke visitAssign_expr.
    }
    return 0;
}

//--------------------------------------------------------
// Return statement visitor (added a check for nested assignments)
antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    if (ctx->expr()->CONST())
    {
        int retval = std::stoi(ctx->expr()->CONST()->getText());
        std::cout << "    movl $" << retval << ", %eax\n";
    }
    else if (ctx->expr()->VAR())
    {
        std::string varName = ctx->expr()->VAR()->getText();
        int offset = symbolTable[varName];
        std::cout << "    movl " << offset << "(%rbp), %eax\n";
    }
    else if (ctx->expr()->assign_expr() != nullptr)
    {
        visit(ctx->expr()->assign_expr());
    }
    return 0;
}

//--------------------------------------------------------
// Update expression visitor to handle nested assignments.
antlrcpp::Any CodeGenVisitor::visitExpr(ifccParser::ExprContext *ctx)
{
    if (ctx->assign_expr() != nullptr)
    {
        return visit(ctx->assign_expr());
    }
    if (ctx->CONST())
    {
        int value = std::stoi(ctx->CONST()->getText());
        std::cout << "    movl $" << value << ", %eax" << std::endl;
    }
    else if (ctx->VAR())
    {
        std::string varName = ctx->VAR()->getText();
        std::cout << "    movl " << symbolTable[varName] << "(%rbp), %eax" << std::endl;
    }
    return 0;
}

//--------------------------------------------------------
// Generate code for the entire program.
antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx)
{
    std::cout << ".globl main\n";
    std::cout << " main: \n";

    // Prologue
    std::cout << "    pushq %rbp\n";
    std::cout << "    movq %rsp, %rbp\n";

    // Visit declarations and assignments.
    for (auto stmt : ctx->stmt())
    {
        visit(stmt);
    }

    // Visit return statement.
    visit(ctx->return_stmt());

    // Epilogue
    std::cout << "    popq %rbp\n";
    std::cout << "    ret\n";

    return 0;
}
