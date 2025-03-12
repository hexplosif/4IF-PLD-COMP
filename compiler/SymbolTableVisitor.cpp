#include "SymbolTableVisitor.h"
#include "generated/ifccParser.h"
#include "antlr4-runtime.h"

antlrcpp::Any SymbolTableVisitor::visitDeclaration(ifccParser::DeclarationContext *ctx)
{
    for (auto varDecl : ctx->var_decl_list()->var_decl())
    {
        std::string varName = varDecl->VAR()->getText();
        if (!symbols.addVariable(varName))
        {
            std::cerr << "Erreur : Variable '" << varName << "' déjà déclarée." << std::endl;
            exit(1);
        }
    }
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitAssignment(ifccParser::AssignmentContext *ctx)
{
    for (size_t i = 0; i < ctx->assign_expr().size(); i++)
    {
        std::string varName = ctx->assign_expr(i)->VAR()->getText();
        if (!symbols.exists(varName))
        {
            std::cerr << "Erreur : Variable '" << varName << "' utilisée avant déclaration." << std::endl;
            exit(1);
        }
        visit(ctx->assign_expr(i)->expr());
    }
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitExpr(ifccParser::ExprContext *ctx)
{
    return visitChildren(ctx);
}
