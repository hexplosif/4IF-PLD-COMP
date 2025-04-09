#include "CodeGenVisitor.h"
#include "IR.h"
#include "FeedbackStyleOutput.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

/*
 * Pour cette implémentation, nous utilisons un CFG global (pour la fonction main).
 * En pratique, vous pouvez encapsuler cela dans une classe plus élaborée.
 */
CFG *cfg = nullptr;

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx)
{
    // Création du GVM (Global Variable Manager)
    gvm = new GVM();

    // Le programme est : (decl_stmt)* 'int' 'main' '(' ')' block
    // On traite ici les déclarations globales
    for (auto decl : ctx->decl_stmt())
    {
        this->visit(decl);
    }

    // Création du CFG pour main (l'AST de la fonction est ici nullptr pour simplifier)
    // TODO: if this block is a function block, we need to create a new cfg and scope for the function
    cfg = new CFG(nullptr);
    cfg->currentScope = new SymbolTable(0);
    cfg->currentScope->setParent(gvm->getGlobalScope());

    BasicBlock *bbStart = new BasicBlock(cfg, cfg->new_BB_name());
    cfg->add_bb(bbStart);
    cfg->current_bb->exit_true = bbStart;
    cfg->current_bb = bbStart;

    // On traite ici le bloc principal
    this->visit(ctx->block());

    BasicBlock *bbEnd = new BasicBlock(cfg, ".Lepilogue");
    cfg->add_bb(bbEnd);
    cfg->current_bb->exit_true = bbEnd;

    // Une fois le CFG construit, on génère le code assembleur.
    gvm->gen_asm(std::cout);
    cfg->gen_asm(std::cout);
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitBlock(ifccParser::BlockContext *ctx)
{
    // Visiter les statements
    for (auto stmt : ctx->stmt())
    {
        // if (cfg->current_bb == nullptr)
        //     break; // Un return a été rencontré : on arrête le traitement du bloc.
        this->visit(stmt);
    }

    cfg->currentScope->checkUnusedVariables();
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDecl_stmt(ifccParser::Decl_stmtContext *ctx)
{
    string varType = ctx->type()->getText();
    for (auto sdecl : ctx->sub_decl())
    {
        visitSub_declWithType(sdecl, varType);
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitSub_declWithType(ifccParser::Sub_declContext *ctx, string varType)
{
    bool isGlobalScope = (cfg == nullptr);
    string varName = ctx->VAR()->getText();

    if (isGlobalScope)
    {
        gvm->addGlobalVariable(varName, varType);

        if (ctx->expr())
        {
            string exprCst = any_cast<string>(visit(ctx->expr()));
            Symbol *var = gvm->getGlobalScope()->findVariable(exprCst);
            gvm->setGlobalVariableValue(varName, var->getCstValue());
        }
    }
    else
    {
        if (ctx->CONST(0) != nullptr) { // C'est un tableaux
            int size = std::stoi(ctx->CONST(0)->getText());
            cfg->currentScope->addLocalVariable(varName, varType, size); 
            int i = 1;
            while(ctx->CONST(i)) {
                string pos = std::to_string(i-1); 
                string tempPos = cfg->currentScope->addTempVariable("int");
                cfg->current_bb->add_IRInstr(IRInstr::ldconst, VarType::INT, {tempPos, pos});
                string value = ctx->CONST(i)->getText();  
                string tempValue = cfg->currentScope->addTempVariable("int");
                cfg->current_bb->add_IRInstr(IRInstr::ldconst, VarType::INT, {tempValue, value});
                cfg->current_bb->add_IRInstr(IRInstr::copyTblx, VarType::INT, {varName, tempValue, tempPos});
                i++;
            }
        }
        else
        {
            cfg->currentScope->addLocalVariable(varName, varType);

            if (ctx->expr())
            {
                string expr = any_cast<string>(visit(ctx->expr()));
                cfg->current_bb->add_IRInstr(IRInstr::Operation::copy, VarType::INT, {varName, expr});
            }
        }
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssignmentStatement(ifccParser::AssignmentStatementContext *ctx)
{
    // Récupération du contexte de la règle assign_stmt
    auto assign = ctx->assign_stmt();
    string varName = assign->VAR()->getText();
    Symbol *var = cfg->currentScope->findVariable(varName);
    if (var == nullptr)
    {
        FeedbackOutputFormat::showFeedbackOutput("error", "variable '" + varName + "' not declared");
        exit(1);
    }

    // On suppose que la variable a déjà été déclarée
    string op = assign->op_assign()->getText();
    size_t equalsPos = ctx->getText().find('=');
    if (equalsPos != std::string::npos && ctx->getText().substr(0, equalsPos).find('[') != std::string::npos)
    { // C'est un tableau
        string pos = any_cast<string>(this->visit(assign->expr(0)));
        if (op == "=")
        {
            string exprResult = any_cast<string>(this->visit(assign->expr(1)));
            cfg->current_bb->add_IRInstr(IRInstr::copyTblx, VarType::INT, {varName, exprResult, pos});
        }
        else if (op == "+=")
        {
            string exprResult = any_cast<string>(this->visit(assign->expr(1)));
            cfg->current_bb->add_IRInstr(IRInstr::addTblx, VarType::INT, {varName, exprResult, pos});
        }
        else if (op == "-=")
        {
            string exprResult = any_cast<string>(this->visit(assign->expr(1)));
            cfg->current_bb->add_IRInstr(IRInstr::subTblx, VarType::INT, {varName, exprResult, pos});
        }
        else if (op == "*=")
        {
            string exprResult = any_cast<string>(this->visit(assign->expr(1)));
            cfg->current_bb->add_IRInstr(IRInstr::mulTblx, VarType::INT, {varName, exprResult, pos});
        }
        else if (op == "/=")
        {
            string exprResult = any_cast<string>(this->visit(assign->expr(1)));
            cfg->current_bb->add_IRInstr(IRInstr::divTblx, VarType::INT, {varName, exprResult, pos});
        }
        else if (op == "%=")
        {
            string exprResult = any_cast<string>(this->visit(assign->expr(1)));
            cfg->current_bb->add_IRInstr(IRInstr::modTblx, VarType::INT, {varName, exprResult, pos});
        }
    }
    else
    {
        if (op == "=")
        {
            string exprResult = any_cast<string>(this->visit(assign->expr(0)));
            cfg->current_bb->add_IRInstr(IRInstr::copy, VarType::INT, {varName, exprResult});
        }
        else if (op == "+=")
        {
            string exprResult = any_cast<string>(this->visit(assign->expr(0)));
            cfg->current_bb->add_IRInstr(IRInstr::add, VarType::INT, {varName, varName, exprResult});
        }
        else if (op == "-=")
        {
            string exprResult = any_cast<string>(this->visit(assign->expr(0)));
            cfg->current_bb->add_IRInstr(IRInstr::sub, VarType::INT, {varName, varName, exprResult});
        }
        else if (op == "*=")
        {
            string exprResult = any_cast<string>(this->visit(assign->expr(0)));
            cfg->current_bb->add_IRInstr(IRInstr::mul, VarType::INT, {varName, varName, exprResult});
        }
        else if (op == "/=")
        {
            string exprResult = any_cast<string>(this->visit(assign->expr(0)));
            cfg->current_bb->add_IRInstr(IRInstr::div, VarType::INT, {varName, varName, exprResult});
        }
        else if (op == "%=")
        {
            string exprResult = any_cast<string>(this->visit(assign->expr(0)));
            cfg->current_bb->add_IRInstr(IRInstr::mod, VarType::INT, {varName, varName, exprResult});
        }
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    // Évalue l'expression de retour
    std::string exprResult = any_cast<std::string>(this->visit(ctx->expr()));
    // Copie le résultat dans la variable spéciale "ret" ("ret" ne marche pas avec LINUX)
    cfg->current_bb->add_IRInstr(IRInstr::copy, VarType::INT, {"%eax", exprResult});
    // Ajoute un saut vers l'épilogue
    cfg->current_bb->add_IRInstr(IRInstr::jmp, VarType::INT, {".Lepilogue"});
    return exprResult;
}

antlrcpp::Any CodeGenVisitor::visitAddSubExpression(ifccParser::AddSubExpressionContext *ctx)
{
    string left = any_cast<string>(visit(ctx->expr(0)));
    string right = any_cast<string>(visit(ctx->expr(1)));

    IRInstr::Operation op;
    if (ctx->op->getText() == "+")
    {
        op = IRInstr::Operation::add;
    }
    else if (ctx->op->getText() == "-")
    {
        op = IRInstr::Operation::sub;
    }

    string tmp = cfg->currentScope->addTempVariable("int");
    cfg->current_bb->add_IRInstr(op, VarType::INT, {tmp, left, right});
    return tmp;
}

antlrcpp::Any CodeGenVisitor::visitMulDivExpression(ifccParser::MulDivExpressionContext *ctx)
{
    string left = any_cast<string>(visit(ctx->expr(0)));
    string right = any_cast<string>(visit(ctx->expr(1)));

    IRInstr::Operation op;
    if (ctx->OPM()->getText() == "*")
    {
        op = IRInstr::Operation::mul;
    }
    else if (ctx->OPM()->getText() == "/")
    {
        op = IRInstr::Operation::div;
    }
    else if (ctx->OPM()->getText() == "%")
    {
        op = IRInstr::Operation::mod;
    }

    string tmp = cfg->currentScope->addTempVariable("int");
    cfg->current_bb->add_IRInstr(op, VarType::INT, {tmp, left, right});
    return tmp;
}

antlrcpp::Any CodeGenVisitor::visitConstantExpression(ifccParser::ConstantExpressionContext *ctx)
{
    int value = std::stoi(ctx->CONST()->getText());
    string temp;
    if (cfg != nullptr)
    { // Viste le contexte lorsqu'on est dans une fonction
        temp = cfg->currentScope->addTempConstVariable("int", value);
        cfg->current_bb->add_IRInstr(IRInstr::ldconst, VarType::CHAR, {temp, to_string(value)});
    }
    else
    { // Si on est dans le contexte global
        temp = gvm->addTempConstVariable("int", value);
    }
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitConstantCharExpression(ifccParser::ConstantCharExpressionContext *ctx)
{
    // Traitement des constantes de type char (par exemple, 'a')
    string value = ctx->CONST_CHAR()->getText().substr(1, 1);
    int ascii = (int)value[0];
    // Crée une variable temporaire et charge la constante dedans.
    string temp;
    if (cfg != nullptr)
    { // Si on est dans le contexte d'une fonction
        temp = cfg->currentScope->addTempConstVariable("char", ascii);
        cfg->current_bb->add_IRInstr(IRInstr::ldconst, VarType::CHAR, {temp, to_string(ascii)});
    }
    else
    { // Si on est dans le contexte global
        temp = gvm->addTempConstVariable("char", ascii);
    }

    return temp;
}

antlrcpp::Any CodeGenVisitor::visitVariableExpression(ifccParser::VariableExpressionContext *ctx)
{
    // On retourne simplement le nom de la variable.
    string varName = ctx->VAR()->getText();
    Symbol *var = cfg->currentScope->findVariable(varName);
    if (var == nullptr)
    {
        FeedbackOutputFormat::showFeedbackOutput("error", "variable '" + varName + "' not declared");
        exit(1);
    }

    var->markUsed();
    return varName;
}

antlrcpp::Any CodeGenVisitor::visitParenthesisExpression(ifccParser::ParenthesisExpressionContext *ctx)
{
    return this->visit(ctx->expr());
}

antlrcpp::Any CodeGenVisitor::visitComparisonExpression(ifccParser::ComparisonExpressionContext *ctx)
{
    // expr op=('=='|'<'|'<='|'>'|'>=') expr
    string left = any_cast<string>(this->visit(ctx->expr(0)));
    string right = any_cast<string>(this->visit(ctx->expr(1)));
    string temp = cfg->currentScope->addTempVariable("int");
    string op = ctx->op->getText();
    if (op == "==")
    {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_eq, VarType::INT, {temp, left, right});
    }
    else if (op == "!=")
    {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_ne, VarType::INT, {temp, left, right});
    }
    else if (op == "<")
    {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_lt, VarType::INT, {temp, left, right});
    }
    else if (op == "<=")
    {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_le, VarType::INT, {temp, left, right});
    }
    else if (op == ">")
    {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_gt, VarType::INT, {temp, left, right});
    }
    else if (op == ">=")
    {
        cfg->current_bb->add_IRInstr(IRInstr::cmp_ge, VarType::INT, {temp, left, right});
    }
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitBitwiseExpression(ifccParser::BitwiseExpressionContext *ctx)
{
    // expr op=('&'|'|'|'^') expr
    string left = any_cast<string>(this->visit(ctx->expr(0)));
    string right = any_cast<string>(this->visit(ctx->expr(1)));
    string temp = cfg->currentScope->addTempVariable("int");
    string op = ctx->op->getText();
    if (op == "&")
    {
        cfg->current_bb->add_IRInstr(IRInstr::bit_and, VarType::INT, {temp, left, right});
    }
    else if (op == "|")
    {
        cfg->current_bb->add_IRInstr(IRInstr::bit_or, VarType::INT, {temp, left, right});
    }
    else if (op == "^")
    {
        cfg->current_bb->add_IRInstr(IRInstr::bit_xor, VarType::INT, {temp, left, right});
    }
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitUnaryExpression(ifccParser::UnaryExpressionContext *ctx)
{
    // op=('-'|'!') expr
    string expr = any_cast<string>(this->visit(ctx->expr()));
    string temp = cfg->currentScope->addTempVariable("int");
    string op = ctx->op->getText();
    if (op == "-")
    {
        cfg->current_bb->add_IRInstr(IRInstr::unary_minus, VarType::INT, {temp, expr});
    }
    else if (op == "!")
    {
        cfg->current_bb->add_IRInstr(IRInstr::not_op, VarType::INT, {temp, expr});
    }
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitPostIncrementExpression(ifccParser::PostIncrementExpressionContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    cfg->current_bb->add_IRInstr(IRInstr::incr, VarType::INT, {varName});
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitPostDecrementExpression(ifccParser::PostDecrementExpressionContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    cfg->current_bb->add_IRInstr(IRInstr::decr, VarType::INT, {varName});
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitArrayAccessExpression(ifccParser::ArrayAccessExpressionContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    string pos = any_cast<string>(this->visit(ctx->expr()));
    string temp = cfg->currentScope->addTempVariable("int");
    cfg->current_bb->add_IRInstr(IRInstr::getTblx, VarType::INT, {temp, varName, pos});
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitFunctionCallExpression(ifccParser::FunctionCallExpressionContext *ctx)
{
    // Get the function name from the VAR token
    std::string funcName = ctx->VAR()->getText();

    // Get the list of argument expressions
    auto args = ctx->expr();

    // Check if the function call has more than 6 arguments.
    if (args.size() > 6)
    {
        FeedbackOutputFormat::showFeedbackOutput("error", "function call with more than 6 arguments not supported");
        exit(1);
    }

    // Evaluate each argument and move the result into the appropriate register.
    // According to the Linux System V AMD64 ABI, the first six integer arguments go in:
    // 1st: %rdi, 2nd: %rsi, 3rd: %rdx, 4th: %rcx, 5th: %r8, 6th: %r9.
    // Here we assume that the result of an expression is left in %eax, so we move
    // that 32-bit value into the corresponding register (using the "d" suffix for the lower 32 bits).

    std::vector<std::string> argRegs = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};

    for (size_t i = 0; i < args.size(); i++)
    {
        std::string arg = any_cast<std::string>(this->visit(args[i]));
        cfg->current_bb->add_IRInstr(IRInstr::copy, VarType::INT, {argRegs[i], arg});
    }

    // The result of the function call is left in %eax
    string temp = cfg->currentScope->addTempVariable("int");

    // Call the function, and indicate the destination of the return value
    cfg->current_bb->add_IRInstr(IRInstr::call, VarType::INT, {funcName, temp});

    return temp;
}

antlrcpp::Any CodeGenVisitor::visitIf_stmt(ifccParser::If_stmtContext *ctx)
{
    bool hasElseStmt = ctx->stmt().size() > 1;

    // Évalue la condition
    std::string cond = any_cast<std::string>(visit(ctx->expr()));

    BasicBlock *tmp = cfg->current_bb->exit_true;
    // Crée un nouveau bloc pour la suite (join)
    std::string joinLabel = cfg->new_BB_name();
    BasicBlock *join_bb = new BasicBlock(cfg, joinLabel);
    cfg->add_bb(join_bb);

    BasicBlock *current = cfg->current_bb;
    current->test_var_name = cond;

    // Création des blocs then et else
    std::string thenLabel = cfg->new_BB_name();
    BasicBlock *then_bb = new BasicBlock(cfg, thenLabel);
    cfg->add_bb(then_bb);

    // Crée le bloc pour la branche "else"
    BasicBlock *else_bb = join_bb;
    if (hasElseStmt)
    {
        std::string elseLabel = cfg->new_BB_name();
        else_bb = new BasicBlock(cfg, elseLabel);
        cfg->add_bb(else_bb);
        else_bb->exit_true = join_bb;
    }

    // Connecter les blocs
    join_bb->exit_true = tmp;
    then_bb->exit_true = join_bb;
    cfg->current_bb->exit_true = then_bb;
    cfg->current_bb->exit_false = else_bb;

    // Configure le bloc courant pour effectuer un saut conditionnel
    cfg->current_bb->test_var_name = cond;
    cfg->current_bb->test_var_register = cfg->IR_reg_to_asm(cond);

    // Génère la branche "then"
    cfg->current_bb = then_bb;
    this->visit(ctx->stmt(0));
    // then_bb->add_IRInstr(IRInstr::jmp, VarType::INT, {joinLabel});

    // Génère la branche "else"
    if (hasElseStmt)
    {
        cfg->current_bb = else_bb;
        this->visit(ctx->stmt(1));
        // else_bb->add_IRInstr(IRInstr::jmp, VarType::INT, {joinLabel});
    }

    // Le code après le if se trouve dans join_bb
    cfg->current_bb = join_bb;
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitWhile_stmt(ifccParser::While_stmtContext *ctx)
{
    // Crée un bloc pour évaluer la condition
    std::string condLabel = cfg->new_BB_name();
    BasicBlock *cond_bb = new BasicBlock(cfg, "cond" + condLabel);
    cfg->add_bb(cond_bb);
    cfg->current_bb->exit_true = cond_bb;
    // cfg->current_bb->add_IRInstr(IRInstr::jmp, VarType::INT, {condLabel}); // Ajoute un saut du bloc courant vers le
    // bloc de condition

    // Génère le bloc de condition
    cfg->current_bb = cond_bb;
    std::string cond = any_cast<std::string>(this->visit(ctx->expr()));
    cond_bb->test_var_name = cond;
    cond_bb->test_var_register = cfg->IR_reg_to_asm(cond);

    // Crée le bloc du corps de la boucle
    std::string bodyLabel = cfg->new_BB_name();
    BasicBlock *body_bb = new BasicBlock(cfg, "body" + bodyLabel);
    cfg->add_bb(body_bb);

    // Crée le bloc de sortie (après la boucle)
    std::string joinLabel = cfg->new_BB_name();
    BasicBlock *join_bb = new BasicBlock(cfg, "join" + joinLabel);
    cfg->add_bb(join_bb);

    // La condition détermine le chemin
    cond_bb->exit_true = body_bb;
    cond_bb->exit_false = join_bb;
    body_bb->exit_true = cond_bb;

    // Génère le corps de la boucle
    cfg->current_bb = body_bb;
    this->visit(ctx->stmt());

    // La suite du code se trouve dans join_bb
    cfg->current_bb = join_bb;
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitLogiqueParesseuxExpression(ifccParser::LogiqueParesseuxExpressionContext *ctx)
{
    // expr op=('&&'|'||') expr
    string left = any_cast<string>(this->visit(ctx->expr(0)));
    string right = any_cast<string>(this->visit(ctx->expr(1)));
    string temp = cfg->currentScope->addTempVariable("int");
    string op = ctx->op->getText();
    if (op == "&&")
    {
        cfg->current_bb->add_IRInstr(IRInstr::log_and, VarType::INT, {temp, left, right});
    }
    else if (op == "||")
    {
        cfg->current_bb->add_IRInstr(IRInstr::log_or, VarType::INT, {temp, left, right});
    }
    return temp;
}

antlrcpp::Any CodeGenVisitor::visitBlockStatement(ifccParser::BlockStatementContext *ctx)
{
    enterNewScope();
    visit(ctx->block());
    exitCurrentScope();
    return 0;
}

// =======================================================
//                      OTHERS
// =======================================================
void CodeGenVisitor::enterNewScope()
{
    // Creer un nouveau scope quand on entre un nouveau block
    SymbolTable *parentScope = cfg->currentScope;
    cfg->currentScope = new SymbolTable(parentScope->getCurrentDeclOffset());
    cfg->currentScope->setParent(parentScope);
}

void CodeGenVisitor::exitCurrentScope()
{
    // le block est finis, on synchronize le parent et ce scope; on revient vers scope de parent
    cfg->currentScope->getParent()->synchronize(cfg->currentScope);
    cfg->currentScope = cfg->currentScope->getParent();
}
