#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include <map>
#include <string>

// Déclaration de la classe CodeGenVisitor
class CodeGenVisitor : public ifccBaseVisitor {
    public:
        // Méthodes de génération du code
        virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
        virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
        virtual antlrcpp::Any visitDeclaration(ifccParser::DeclarationContext *ctx) override;
        virtual antlrcpp::Any visitAssignment(ifccParser::AssignmentContext *ctx) override;
        virtual antlrcpp::Any visitExpr(ifccParser::ExprContext *ctx) override;

        // Table des symboles pour stocker les variables et leur offset
        static std::map<std::string, int> symbolTable;

    private:
        // Offset pour les variables (géré comme un entier 32 bits)
        static int stackOffset;
};
