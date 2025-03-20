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

    // Le visiteur construit le CFG/IR et, en fin de visite, génère le code assembleur sur stdout.
    CodeGenVisitor v;
    v.visit(tree);

    return 0;
}
