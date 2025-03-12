#include "SymbolTableVisitor.h"

antlrcpp::Any SymbolTableVisitor::visitDeclaration(ifccParser::DeclarationContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    if (!symbols.addVariable(varName)) {
        exit(1); // Stop en cas d'erreur
    }
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitAssignment(ifccParser::AssignmentContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    if (!symbols.exists(varName)) {
        std::cerr << "Erreur : Variable '" << varName << "' utilisée avant déclaration." << std::endl;
        exit(1);
    }
    return visit(ctx->expr());
}

antlrcpp::Any SymbolTableVisitor::visitExpr(ifccParser::ExprContext *ctx) {
    if (ctx->VAR()) {
        std::string varName = ctx->VAR()->getText();
        if (!symbols.exists(varName)) {
            std::cerr << "Erreur : Variable '" << varName << "' utilisée avant déclaration." << std::endl;
            exit(1);
        }
    }
    return 0;
}
