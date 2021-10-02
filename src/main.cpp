#include <iostream>

#include "antlr4-runtime.h"
#include "TmplangLexer.h"
#include "TmplangParser.h"
#include "TmplangBaseListener.h"

using namespace antlr4;

class TreeListener : public TmplangBaseListener {
public:
  void exitFile(TmplangParser::FileContext *ctx) override {
    std::cout << "parsed function list\n";
    for (auto *fn : ctx->function()) {
      std::cout << fn->ID()->toString() << std::endl;
    }
  }
};

int main(int argc, const char *argv[]) {
  ANTLRInputStream input(std::cin);
  TmplangLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  TmplangParser parser(&tokens);

  tree::ParseTree *tree = parser.file();
  TreeListener listener;
  tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);

  return 0;
}
