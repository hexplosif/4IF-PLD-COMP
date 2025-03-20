#pragma once

#include <iostream>
#include <map>
#include <string>

#include "SymbolTable.h"
#include "generated/ifccBaseVisitor.h"

class CodeValidationVisitor : public ifccBaseVisitor
{
  private:
    SymbolTable *currentScope = new SymbolTable(0); //  global scope

    std::unordered_map<std::string, bool> usageMap;

    void genUsedBeforeDeclarationErrorMessage(int line, int column, std::string &var) const;
    void genRedeclarationErrorMessage(int line, int column, std::string &var) const;
    void genNotUsedVariableWarningMessage(std::string &var) const;
    void genInvalidInitialiseGlobalVariableErrorMessage(int line, int column) const;

    bool isExprIsConstant(ifccParser::ExprContext *ctx);

  public:
    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
    virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;
    virtual antlrcpp::Any visitDecl_stmt(ifccParser::Decl_stmtContext *ctx) override;
    virtual antlrcpp::Any visitSub_declWithType(ifccParser::Sub_declContext *ctx, std::string varType);
    virtual antlrcpp::Any visitAssign_stmt(ifccParser::Assign_stmtContext *ctx) override;
    virtual antlrcpp::Any visitVariableExpression(ifccParser::VariableExpressionContext *ctx) override;

    // destructor to free memory
    ~CodeValidationVisitor() {
      while (currentScope) {
          SymbolTable* parent = currentScope->getParent();
          delete currentScope;
          currentScope = parent;
      }
    } 
};
