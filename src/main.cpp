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
  virtual void print() = 0;
};
struct TypeVar : public Type {
  int id;

  virtual void print() {
    std::cout << "Var (id: " << id << ")";
  }
};
struct ConcreteType : public Type {
  std::string name;
  ConcreteType() {}
  ConcreteType(std::string&& _name): name(_name) {}

  virtual void print() {
    std::cout << "Concrete " << name;
  }
};
struct FunctionType : public Type {
  std::vector<std::shared_ptr<Type>> from;
  std::shared_ptr<Type> to;

  virtual void print() {
    std::cout << "Func (";
    for (auto arg : from) {
      arg->print();
      std::cout << ", ";
    }
    std::cout << ") -> ";
    to->print();
  }
};
std::shared_ptr<Type> boolType = std::make_shared<ConcreteType>("bool");
std::shared_ptr<Type> intType = std::make_shared<ConcreteType>("int");
std::shared_ptr<Type> charType = std::make_shared<ConcreteType>("char");
std::vector<std::shared_ptr<Type>> tmpTypes;


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
  Type *currentFunctionType;
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

  void exitFunctionParams(TmplangParser::FunctionParamsContext *ctx) override {
    FunctionType *functionType = dynamic_cast<FunctionType*>(currentFunctionType);

    for (auto *decl : ctx->functionParamDecl()) {
      auto paramIt = currentScope->symbols.find(decl->identifier()->getText());
      functionType->from.push_back(paramIt->second);
    }
  }

  void enterFunction(TmplangParser::FunctionContext *ctx) override {
    FunctionType functionType;

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

    auto insertRet = currentScope->symbols.emplace(ctx->identifier()->getText(), std::make_shared<FunctionType>(functionType));
    if (!insertRet.second) {
      std::cout << "function decl collision!!!\n";
    }

    scopes.put(ctx, std::make_shared<Scope>());
    scopes.get(ctx)->parent = currentScope;

    // move downward
    currentScope = scopes.get(ctx).get();
    currentFunctionType = currentScope->parent->symbols.find(ctx->identifier()->getText())->second.get();
  }

  void exitFunction(TmplangParser::FunctionContext *ctx) override {
    // move upward
    currentScope = currentScope->parent;
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


struct TypeEquation {
  Type *left, *right;
};

class TypeInferencer : public TmplangBaseListener {
 public:
  TypeInferencer(tree::ParseTreeProperty<std::shared_ptr<Scope>>&& _scopes) : scopes(_scopes) {}

  tree::ParseTreeProperty<std::shared_ptr<Scope>> scopes;
  Scope *currentScope;
  Type *currentFunctionType;

  tree::ParseTreeProperty<std::shared_ptr<Type>> nodeTypes;
  std::vector<TypeEquation> equations;

  void enterFile(TmplangParser::FileContext *ctx) override {
    currentScope = scopes.get(ctx).get();

    for (auto kv : currentScope->symbols) {
      std::cout << kv.first << std::endl;
      kv.second->print();
      std::cout << std::endl;
    }
  }

  void enterFunction(TmplangParser::FunctionContext *ctx) override {
    currentScope = scopes.get(ctx).get();
    currentFunctionType = currentScope->parent->symbols.find(ctx->identifier()->getText())->second.get();
  }

  void exitFunction(TmplangParser::FunctionContext *ctx) override {
    currentScope = currentScope->parent;
  }

  void enterBlockStatement(TmplangParser::BlockStatementContext *ctx) override {
    currentScope = scopes.get(ctx).get();
  }

  void exitBlockStatement(TmplangParser::BlockStatementContext *ctx) override {
    currentScope = currentScope->parent;
  }

  void enterIfStatement(TmplangParser::IfStatementContext *ctx) override {
    currentScope = scopes.get(ctx).get();
  }

  void exitIfStatement(TmplangParser::IfStatementContext *ctx) override {
    equations.push_back(TypeEquation{ nodeTypes.get(ctx->expr()).get(), boolType.get() });
    currentScope = currentScope->parent;
  }

  void exitReturnStatement(TmplangParser::ReturnStatementContext *ctx) override {
    Type* a = nodeTypes.get(ctx->expr()).get();
    Type* b = dynamic_cast<FunctionType*>(currentFunctionType)->to.get();
    equations.push_back(TypeEquation{ a, b });
  }

  void exitAssignStatement(TmplangParser::AssignStatementContext *ctx) override {
    Type *identifierType = currentScope->resolve(ctx->identifier()->getText());
    if (identifierType == nullptr) {
      std::cout << "can't find identifier definition!!\n";
    }

    equations.push_back(TypeEquation{ identifierType, nodeTypes.get(ctx->expr()).get() });
  }

  void enterFunctionCallExpr(TmplangParser::FunctionCallExprContext *ctx) override {
    TypeVar typeVar;
    typeVar.id = typecounter++;
    nodeTypes.put(ctx, std::make_shared<TypeVar>(typeVar));
  }

  void exitFunctionCallExpr(TmplangParser::FunctionCallExprContext *ctx) override {
    FunctionType functionType;
    if (ctx->exprList() != nullptr) {
      for (auto *arg : ctx->exprList()->expr()) {
        functionType.from.push_back(nodeTypes.get(arg));
      }
    }
    functionType.to = nodeTypes.get(ctx);
    tmpTypes.push_back(std::make_shared<FunctionType>(functionType));

    equations.push_back(TypeEquation{ nodeTypes.get(ctx->expr()).get(), tmpTypes.back().get() });
  }

  void enterNegateExpr(TmplangParser::NegateExprContext *ctx) override {
    TypeVar typeVar;
    typeVar.id = typecounter++;
    nodeTypes.put(ctx, std::make_shared<TypeVar>(typeVar));
  }

  void exitNegateExpr(TmplangParser::NegateExprContext *ctx) override {
    equations.push_back(TypeEquation{ nodeTypes.get(ctx).get(), nodeTypes.get(ctx->expr()).get() });
  }

  void enterNotExpr(TmplangParser::NotExprContext *ctx) override {
    TypeVar typeVar;
    typeVar.id = typecounter++;
    nodeTypes.put(ctx, std::make_shared<TypeVar>(typeVar));
  }

  void exitNotExpr(TmplangParser::NotExprContext *ctx) override {
    equations.push_back(TypeEquation{ nodeTypes.get(ctx).get(), nodeTypes.get(ctx->expr()).get() });
  }

  void enterMulDivExpr(TmplangParser::MulDivExprContext *ctx) override {
    TypeVar typeVar;
    typeVar.id = typecounter++;
    nodeTypes.put(ctx, std::make_shared<TypeVar>(typeVar));
  }

  void exitMulDivExpr(TmplangParser::MulDivExprContext *ctx) override {
    equations.push_back(TypeEquation{ nodeTypes.get(ctx).get(), nodeTypes.get(ctx->expr()[0]).get() });
    equations.push_back(TypeEquation{ nodeTypes.get(ctx).get(), nodeTypes.get(ctx->expr()[1]).get() });
  }

  void enterPlusMinusExpr(TmplangParser::PlusMinusExprContext *ctx) override {
    TypeVar typeVar;
    typeVar.id = typecounter++;
    nodeTypes.put(ctx, std::make_shared<TypeVar>(typeVar));
  }

  void exitPlusMinusExpr(TmplangParser::PlusMinusExprContext *ctx) override {
    equations.push_back(TypeEquation{ nodeTypes.get(ctx).get(), nodeTypes.get(ctx->expr()[0]).get() });
    equations.push_back(TypeEquation{ nodeTypes.get(ctx).get(), nodeTypes.get(ctx->expr()[1]).get() });
  }

  void enterEqualExpr(TmplangParser::EqualExprContext *ctx) override {
    TypeVar typeVar;
    typeVar.id = typecounter++;
    nodeTypes.put(ctx, std::make_shared<TypeVar>(typeVar));
  }

  void exitEqualExpr(TmplangParser::EqualExprContext *ctx) override {
    equations.push_back(TypeEquation{ nodeTypes.get(ctx->expr()[0]).get(), nodeTypes.get(ctx->expr()[1]).get() });
    equations.push_back(TypeEquation{ nodeTypes.get(ctx).get(), boolType.get() });
  }

  void enterVarRefExpr(TmplangParser::VarRefExprContext *ctx) override {
    TypeVar typeVar;
    typeVar.id = typecounter++;
    nodeTypes.put(ctx, std::make_shared<TypeVar>(typeVar));
  }

  void exitVarRefExpr(TmplangParser::VarRefExprContext *ctx) override {
    Type *varType = currentScope->resolve(ctx->identifier()->getText());
    if (varType == nullptr) {
      std::cout << "can't find variable definition!!! : " << ctx->identifier()->getText() << "\n";
    }

    equations.push_back(TypeEquation{ nodeTypes.get(ctx).get(), varType });
  }

  void enterLiteralExpr(TmplangParser::LiteralExprContext *ctx) override {
    TypeVar typeVar;
    typeVar.id = typecounter++;
    nodeTypes.put(ctx, std::make_shared<TypeVar>(typeVar));
  }

  void exitLiteralExpr(TmplangParser::LiteralExprContext *ctx) override {
    if (ctx->literal()->IntegerLiteral() != nullptr) {
      equations.push_back(TypeEquation{ nodeTypes.get(ctx).get(), intType.get() });
    }
    else if (ctx->literal()->BoolLiteral() != nullptr) {
      equations.push_back(TypeEquation{ nodeTypes.get(ctx).get(), boolType.get() });
    }
    else if (ctx->literal()->CharacterLiteral() != nullptr) {
      equations.push_back(TypeEquation{ nodeTypes.get(ctx).get(), charType.get() });
    }
    else {
      std::cout << "unparsable literal!!\n";
    }
  }

  void enterParenExpr(TmplangParser::ParenExprContext *ctx) override {
    TypeVar typeVar;
    typeVar.id = typecounter++;
    nodeTypes.put(ctx, std::make_shared<TypeVar>(typeVar));
  }

  void exitParenExpr(TmplangParser::ParenExprContext *ctx) override {
    equations.push_back(TypeEquation{ nodeTypes.get(ctx).get(), nodeTypes.get(ctx->expr()).get() });
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

  TypeInferencer inferencer(std::move(listener.scopes));
  tree::ParseTreeWalker::DEFAULT.walk(&inferencer, tree);

  for (auto eq : inferencer.equations) {
    std::cout << "equation ===========\n";
    eq.left->print();
    std::cout << "  ==  ";
    eq.right->print();
    std::cout << '\n';
  }
  return 0;
}
