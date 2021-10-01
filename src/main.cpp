#include <iostream>

#include "antlr4-runtime.h"
#include "tmplangLexer.h"
#include "tmplangParser.h"
#include "tmplangBaseListener.h"

using namespace antlr4;

class TreeListener : public tmplangBaseListener {
public:
  void enterFile(tmplangParser::FileContext *ctx) override {
    std::cout << "file\n";
  }
};

int main(int argc, const char *argv[]) {
  ANTLRInputStream input(std::cin);
  tmplangLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  tmplangParser parser(&tokens);

  tree::ParseTree *tree = parser.file();
  TreeListener listener;
  tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);

  return 0;
}
