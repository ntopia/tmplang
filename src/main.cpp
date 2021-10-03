#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

#include "antlr4-runtime.h"
#include "TmplangLexer.h"
#include "TmplangParser.h"
#include "TmplangBaseListener.h"

using namespace antlr4;


int typecounter = 0;
enum TypeKind {
  TYPE_VAR,
  CONCRETE_TYPE,
  FUNCTION_TYPE,
};
struct Type {
};
struct TypeVar : public Type {
  int id;
};
struct ConcreteType : public Type {
  std::string name;
};
struct FunctionType : public Type {
  std::vector<std::shared_ptr<Type>> from;
  std::shared_ptr<Type> to;
};

struct Scope {
  std::unordered_map<std::string, std::shared_ptr<Type>> symbols;
  Scope *parent;

  Type* resolve(const std::string& name) {
    Scope *scope = this;
    while (scope != nullptr) {
      auto it = scope->symbols.find(name);
      if (it != scope->symbols.end()) {
        return it->second.get();
      }
      scope = scope->parent;
    }
    return nullptr;
  }
};

class SymbolTableGenerator : public TmplangBaseListener {
 public:
  tree::ParseTreeProperty<std::shared_ptr<Scope>> scopes;
  Scope *currentScope;

  void enterFile(TmplangParser::FileContext *ctx) override {
    scopes.put(ctx, std::make_shared<Scope>());
    scopes.get(ctx)->parent = nullptr;

    // move downward
    currentScope = scopes.get(ctx).get();
  }

  void enterFunctionParamDecl(TmplangParser::FunctionParamDeclContext *ctx) override {
    ConcreteType concreteType;
    concreteType.name = ctx->type()->getText();

    auto insertRet = currentScope->symbols.emplace(ctx->identifier()->getText(), std::make_shared<ConcreteType>(concreteType));
    if (!insertRet.second) {
      std::cout << "param decl collision!!!\n";
    }
  }

  void exitFunctionParamDecl(TmplangParser::FunctionParamDeclContext *ctx) override {
    // type equation to each param
  }

  void enterFunction(TmplangParser::FunctionContext *ctx) override {
    scopes.put(ctx, std::make_shared<Scope>());
    scopes.get(ctx)->parent = currentScope;

    // move downward
    currentScope = scopes.get(ctx).get();
  }

  void exitFunction(TmplangParser::FunctionContext *ctx) override {
    FunctionType functionType;

    auto name = ctx->identifier()->ID()->getText();

    if (ctx->functionParams() != nullptr) {
      for (auto *decl : ctx->functionParams()->functionParamDecl()) {
        auto paramIt = currentScope->symbols.find(decl->identifier()->getText());
        functionType.from.push_back(paramIt->second);
      }
    }

    if (ctx->functionReturnTypeDecl() != nullptr) {
      ConcreteType concreteType;
      concreteType.name = ctx->functionReturnTypeDecl()->type()->getText();
      functionType.to = std::make_shared<ConcreteType>(concreteType);
    }
    else {
      TypeVar typeVar;
      typeVar.id = typecounter++;
      functionType.to = std::make_shared<TypeVar>(typeVar);
    }

    // move upward
    currentScope = currentScope->parent;
    auto insertRet = currentScope->symbols.emplace(ctx->identifier()->getText(), std::make_shared<FunctionType>(functionType));
    if (!insertRet.second) {
      std::cout << "function decl collision!!!\n";
    }
  }

  void enterBlockStatement(TmplangParser::BlockStatementContext *ctx) override {
    scopes.put(ctx, std::make_shared<Scope>());
    scopes.get(ctx)->parent = currentScope;

    // move downward
    currentScope = scopes.get(ctx).get();
  }

  void exitBlockStatement(TmplangParser::BlockStatementContext *ctx) override {
    // move upward
    currentScope = currentScope->parent;
  }

  void enterIfStatement(TmplangParser::IfStatementContext *ctx) override {
    scopes.put(ctx, std::make_shared<Scope>());
    scopes.get(ctx)->parent = currentScope;

    // move downward
    currentScope = scopes.get(ctx).get();
  }

  void exitIfStatement(TmplangParser::IfStatementContext *ctx) override {
    // move upward
    currentScope = currentScope->parent;
  }

  void exitVarDeclStatement(TmplangParser::VarDeclStatementContext *ctx) override {
    std::shared_ptr<Type> varType;
    if (ctx->type() == nullptr) {
      TypeVar typeVar;
      typeVar.id = typecounter++;
      varType = std::make_shared<TypeVar>(typeVar);
    }
    else {
      ConcreteType concreteType;
      concreteType.name = ctx->type()->getText();
      varType = std::make_shared<ConcreteType>(concreteType);
    }

    currentScope->symbols.emplace(ctx->identifier()->getText(), varType);
  }
};

int main(int argc, const char *argv[]) {
  ANTLRInputStream input(std::cin);
  TmplangLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  TmplangParser parser(&tokens);

  tree::ParseTree *tree = parser.file();
  SymbolTableGenerator listener;
  tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);

  return 0;
}
