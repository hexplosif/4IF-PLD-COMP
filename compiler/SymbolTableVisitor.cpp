#include "SymbolTableVisitor.h"

antlrcpp::Any SymbolTableVisitor::visitDeclaration(ifccParser::DeclarationContext *ctx) {
    for (auto varDecl : ctx->var_decl_list()->var_decl()) {  
        std::string varName = varDecl->VAR()->getText();
        if (!symbols.addVariable(varName)) {
            std::cerr << "Erreur : Variable '" << varName << "' déjà déclarée." << std::endl;
            exit(1);
        }
    }
    return 0;
}


antlrcpp::Any SymbolTableVisitor::visitAssignment(ifccParser::AssignmentContext *ctx) {
    for (size_t i = 0; i < ctx->assign_expr().size(); i++) {
        // Récupérer le nom de la variable de l'affectation
        std::string varName = ctx->assign_expr(i)->VAR()->getText();
        
        // Vérifier si la variable est déclarée
        if (!symbols.exists(varName)) {
            std::cerr << "Erreur : Variable '" << varName << "' utilisée avant déclaration." << std::endl;
            exit(1);
        }

        // Vérifier également si l'expression d'assignation est valide
        visit(ctx->assign_expr(i)->expr()); // Appel à visit pour l'expression
    }
    return 0;
}



antlrcpp::Any SymbolTableVisitor::visitExpr(ifccParser::ExprContext *ctx) {
    if (ctx->VAR()) {  // Vérifier si c'est une variable
        std::string varName = ctx->VAR()->getText();
        if (!symbols.exists(varName)) {
            std::cerr << "Erreur : Variable '" << varName << "' utilisée avant déclaration." << std::endl;
            exit(1);
        }
    }
    return 0;
}

