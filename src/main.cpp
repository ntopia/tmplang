#include <iostream>
#include <vector>

#include "antlr4-runtime.h"
#include "TmplangLexer.h"
#include "TmplangParser.h"
#include "TmplangBaseListener.h"

using namespace antlr4;

int typecounter = 0;
enum TypeKind {
  INT,
  CHAR,
  FUNC,
  TYPE_VAR,
};
struct Type {
  TypeKind type;
  int id;
  std::vector<Type> from;
  int to;
};

struct TypeEquation {
  Type left, right;
};

class AssignTypenameListener : public TmplangBaseListener {
 private:
  tree::ParseTreeProperty<Type> props;
  std::vector<TypeEquation> equations;

 public:
  void exitFile(TmplangParser::FileContext *ctx) override {
    std::cout << "parsed function list\n";
    for (auto *fn : ctx->function()) {
      std::cout << fn->ID()->toString() << std::endl;
    }
  }

  void enterFunctionCallExpr(TmplangParser::FunctionCallExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void exitFunctionCallExpr(TmplangParser::FunctionCallExprContext *ctx) override {
    std::vector<Type> childTypes;
    for (auto* expr : ctx->exprList()->expr()) {
      childTypes.push_back(props.get(expr));
    }
    //equations.push_back(TypeEquation{ ctx-> props.get(ctx), Type{ FUNC, typecounter++, childTypes, }})
  }

  void enterNegateExpr(TmplangParser::NegateExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void enterNotExpr(TmplangParser::NotExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void enterMulDivExpr(TmplangParser::MulDivExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void enterPlusMinusExpr(TmplangParser::PlusMinusExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void enterEqualExpr(TmplangParser::EqualExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void enterVarRefExpr(TmplangParser::VarRefExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void enterLiteralExpr(TmplangParser::LiteralExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void enterParenExpr(TmplangParser::ParenExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void enterLiteral(TmplangParser::LiteralContext *ctx) override {
    if (ctx->CharacterLiteral() != nullptr) {
      std::cout << "character literal\n";
      props.put(ctx, Type{ CHAR, typecounter++ });
    }
    else if (ctx->IntegerLiteral() != nullptr) {
      std::cout << "integer literal " << ctx->IntegerLiteral()->toString() << std::endl;
      props.put(ctx, Type{ INT, typecounter++ });
    }
  }
};

int main(int argc, const char *argv[]) {
  ANTLRInputStream input(std::cin);
  TmplangLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  TmplangParser parser(&tokens);

  tree::ParseTree *tree = parser.file();
  AssignTypenameListener listener;
  tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);

  return 0;
}
