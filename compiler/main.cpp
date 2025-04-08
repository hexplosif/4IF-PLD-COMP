#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include "antlr4-runtime.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"
#include "generated/ifccBaseVisitor.h"

#include "CodeGenVisitor.h"

using namespace antlr4;
using namespace std;

std::map<std::string, DefFonction*> predefinedFunctions; /**< map of all functions in the program */
void predefineFunctions() {
    predefinedFunctions = std::map<std::string, DefFonction*>();

    predefinedFunctions["putchar"] = new DefFonction("putchar", VarType::CHAR);
    predefinedFunctions["putchar"]->setParameters({VarType::CHAR});

    predefinedFunctions["getchar"] = new DefFonction("getchar", VarType::CHAR);
    predefinedFunctions["getchar"]->setParameters({});
}

int main(int argn, const char **argv)
{
    if(argn != 2) {
        cerr << "usage: ifcc path/to/file.c" << endl;
        exit(1);
    }

    ifstream inputFile(argv[1]);
    if(!inputFile.good()) {
        cerr << "error: cannot read file: " << argv[1] << endl;
        exit(1);
    }
    
    stringstream buffer;
    buffer << inputFile.rdbuf();

    ANTLRInputStream input(buffer.str());
    ifccLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    tokens.fill();

    ifccParser parser(&tokens);
    tree::ParseTree* tree = parser.axiom();

    if(parser.getNumberOfSyntaxErrors() != 0) {
        cerr << "error: syntax error during parsing" << endl;
        exit(1);
    }

    predefineFunctions();

    // Le visiteur construit le CFG/IR et, en fin de visite, génère le code assembleur sur stdout.
    CodeGenVisitor v;
    v.visit(tree);
    v.gen_asm(cout);

    return 0;
}
