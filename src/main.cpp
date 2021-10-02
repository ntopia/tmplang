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
  BOOL,
  FUNC,
  TYPE_VAR,
};
struct Type {
  TypeKind type;
  int id;
  std::vector<Type> from, to;
};

struct TypeEquation {
  Type left, right;
};

class AssignTypenameListener : public TmplangBaseListener {
 private:
  tree::ParseTreeProperty<Type> props;

 public:
  std::vector<TypeEquation> equations;

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
    equations.push_back(TypeEquation{ props.get(ctx->expr()), Type{ FUNC, typecounter++, childTypes, std::vector<Type>{ props.get(ctx) } }});
  }

  void enterNegateExpr(TmplangParser::NegateExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void exitNegateExpr(TmplangParser::NegateExprContext *ctx) override {
    equations.push_back(TypeEquation{ props.get(ctx), props.get(ctx->expr()) });
  }

  void enterNotExpr(TmplangParser::NotExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void exitNotExpr(TmplangParser::NotExprContext *ctx) override {
    equations.push_back(TypeEquation{ props.get(ctx->expr()), Type{ BOOL, typecounter++ } });
    equations.push_back(TypeEquation{ props.get(ctx), Type{ BOOL, typecounter++ } });
  }

  void enterMulDivExpr(TmplangParser::MulDivExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void exitMulDivExpr(TmplangParser::MulDivExprContext *ctx) override {
    equations.push_back(TypeEquation{ props.get(ctx->expr()[0]), props.get(ctx->expr()[1]) });
    equations.push_back(TypeEquation{ props.get(ctx), props.get(ctx->expr()[0]) });
    equations.push_back(TypeEquation{ props.get(ctx), props.get(ctx->expr()[1]) });
  }

  void enterPlusMinusExpr(TmplangParser::PlusMinusExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void exitPlusMinusExpr(TmplangParser::PlusMinusExprContext *ctx) override {
    equations.push_back(TypeEquation{ props.get(ctx->expr()[0]), props.get(ctx->expr()[1]) });
    equations.push_back(TypeEquation{ props.get(ctx), props.get(ctx->expr()[0]) });
    equations.push_back(TypeEquation{ props.get(ctx), props.get(ctx->expr()[1]) });
  }

  void enterEqualExpr(TmplangParser::EqualExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void exitEqualExpr(TmplangParser::EqualExprContext *ctx) override {
    // no implicit conversion while equality checking
    equations.push_back(TypeEquation{ props.get(ctx->expr()[0]), props.get(ctx->expr()[1]) });
    equations.push_back(TypeEquation{ props.get(ctx), Type{ BOOL, typecounter++ } });
  }

  void enterVarRefExpr(TmplangParser::VarRefExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void enterLiteralExpr(TmplangParser::LiteralExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void exitLiteralExpr(TmplangParser::LiteralExprContext *ctx) override {
    equations.push_back(TypeEquation{ props.get(ctx), props.get(ctx->literal()) });
  }

  void enterParenExpr(TmplangParser::ParenExprContext *ctx) override {
    props.put(ctx, Type{ TYPE_VAR, typecounter++ });
  }

  void exitParenExpr(TmplangParser::ParenExprContext *ctx) override {
    equations.push_back(TypeEquation{ props.get(ctx), props.get(ctx->expr()) });
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

  for (auto eq : listener.equations) {
    std::cout << eq.left.type << " " << eq.left.id << " " << eq.right.type << " " << eq.right.id << "\n";
  }
  return 0;
}
