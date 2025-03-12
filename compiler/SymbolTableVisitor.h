#ifndef SYMBOLTABLEVISITOR_H
#define SYMBOLTABLEVISITOR_H

#include "generated/ifccBaseVisitor.h"
#include "SymbolTable.h"

class SymbolTableVisitor : public ifccBaseVisitor {
public:
    SymbolTable symbols;

    antlrcpp::Any visitDeclaration(ifccParser::DeclarationContext *ctx) override;
    antlrcpp::Any visitAssignment(ifccParser::AssignmentContext *ctx) override;
    antlrcpp::Any visitExpr(ifccParser::ExprContext *ctx) override;
};

#endif
