#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <random>
#include <unordered_map>
#include <sstream>

#include "antlr4-runtime.h"
#include "TmplangLexer.h"
#include "TmplangParser.h"
#include "TmplangBaseListener.h"
#include "TmplangBaseVisitor.h"

#include "Type.h"
#include "SymbolTable.h"
#include "HMTypeInference.h"

using namespace antlr4;


class TypeEquationGenerater : public TmplangBaseListener {
 public:
  TypeEquationGenerater(tree::ParseTreeProperty<std::shared_ptr<Scope>>& _scopes) : scopes(_scopes) {}

  tree::ParseTreeProperty<std::shared_ptr<Scope>>& scopes;
  Scope *currentScope;
  Type *currentFunctionType;

  tree::ParseTreeProperty<Type*> nodeTypes;
  std::vector<TypeEquation> equations;

  void enterFile(TmplangParser::FileContext *ctx) override {
    currentScope = scopes.get(ctx).get();
  }

  void enterFunction(TmplangParser::FunctionContext *ctx) override {
    currentFunctionType = currentScope->symbols.find(ctx->identifier()->getText())->second;

    Type *returnType;
    if (ctx->functionReturnTypeDecl() != nullptr) {
      returnType = addConcreteType(ctx->functionReturnTypeDecl()->type()->getText());
    }
    else {
      returnType = addTypeVar();
    }
    dynamic_cast<FunctionType*>(currentFunctionType)->to = returnType;

    currentScope = scopes.get(ctx).get();
    currentFunctionType = currentScope->parent->symbols.find(ctx->identifier()->getText())->second;
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
    Type *ifResultType = addConcreteType("bool");

    equations.push_back(TypeEquation{ nodeTypes.get(ctx->expr()), ifResultType });
    currentScope = currentScope->parent;
  }

  void exitVarDeclStatement(TmplangParser::VarDeclStatementContext *ctx) override {
    if (ctx->expr() == nullptr) {
      return;
    }

    Type *identifierType = currentScope->resolve(ctx->identifier()->getText());
    if (identifierType == nullptr) {
      std::cout << "can't find identifier definition!!\n";
    }

    equations.push_back(TypeEquation{ identifierType, nodeTypes.get(ctx->expr()) });
  }

  void exitReturnStatement(TmplangParser::ReturnStatementContext *ctx) override {
    Type* a = nodeTypes.get(ctx->expr());
    Type* b = dynamic_cast<FunctionType*>(currentFunctionType)->to;
    equations.push_back(TypeEquation{ a, b });
  }

  void exitAssignStatement(TmplangParser::AssignStatementContext *ctx) override {
    Type *identifierType = currentScope->resolve(ctx->identifier()->getText());
    if (identifierType == nullptr) {
      std::cout << "can't find identifier definition!!\n";
    }

    equations.push_back(TypeEquation{ identifierType, nodeTypes.get(ctx->expr()) });
  }

  void enterFunctionCallExpr(TmplangParser::FunctionCallExprContext *ctx) override {
    nodeTypes.put(ctx, addTypeVar());
  }

  void exitFunctionCallExpr(TmplangParser::FunctionCallExprContext *ctx) override {
    auto *functionType = addFunctionType();

    if (ctx->exprList() != nullptr) {
      for (auto *arg : ctx->exprList()->expr()) {
        functionType->from.push_back(nodeTypes.get(arg));
      }
    }
    functionType->to = nodeTypes.get(ctx);

    equations.push_back(TypeEquation{ nodeTypes.get(ctx->expr()), functionType });
  }

  void enterNegateExpr(TmplangParser::NegateExprContext *ctx) override {
    nodeTypes.put(ctx, addTypeVar());
  }

  void exitNegateExpr(TmplangParser::NegateExprContext *ctx) override {
    equations.push_back(TypeEquation{ nodeTypes.get(ctx), nodeTypes.get(ctx->expr()) });
  }

  void enterNotExpr(TmplangParser::NotExprContext *ctx) override {
    nodeTypes.put(ctx, addTypeVar());
  }

  void exitNotExpr(TmplangParser::NotExprContext *ctx) override {
    equations.push_back(TypeEquation{ nodeTypes.get(ctx), nodeTypes.get(ctx->expr()) });
  }

  void enterMulDivExpr(TmplangParser::MulDivExprContext *ctx) override {
    nodeTypes.put(ctx, addTypeVar());
  }

  void exitMulDivExpr(TmplangParser::MulDivExprContext *ctx) override {
    equations.push_back(TypeEquation{ nodeTypes.get(ctx), nodeTypes.get(ctx->expr()[0]) });
    equations.push_back(TypeEquation{ nodeTypes.get(ctx), nodeTypes.get(ctx->expr()[1]) });
  }

  void enterPlusMinusExpr(TmplangParser::PlusMinusExprContext *ctx) override {
    nodeTypes.put(ctx, addTypeVar());
  }

  void exitPlusMinusExpr(TmplangParser::PlusMinusExprContext *ctx) override {
    equations.push_back(TypeEquation{ nodeTypes.get(ctx), nodeTypes.get(ctx->expr()[0]) });
    equations.push_back(TypeEquation{ nodeTypes.get(ctx), nodeTypes.get(ctx->expr()[1]) });
  }

  void enterEqualExpr(TmplangParser::EqualExprContext *ctx) override {
    nodeTypes.put(ctx, addTypeVar());
  }

  void exitEqualExpr(TmplangParser::EqualExprContext *ctx) override {
    auto *equalResultType = addConcreteType("bool");

    equations.push_back(TypeEquation{ nodeTypes.get(ctx->expr()[0]), nodeTypes.get(ctx->expr()[1]) });
    equations.push_back(TypeEquation{ nodeTypes.get(ctx), equalResultType });
  }

  void enterVarRefExpr(TmplangParser::VarRefExprContext *ctx) override {
    nodeTypes.put(ctx, addTypeVar());
  }

  void exitVarRefExpr(TmplangParser::VarRefExprContext *ctx) override {
    Type *varType = currentScope->resolve(ctx->identifier()->getText());
    if (varType == nullptr) {
      std::cout << "can't find variable definition!!! : " << ctx->identifier()->getText() << "\n";
    }

    equations.push_back(TypeEquation{ nodeTypes.get(ctx), varType });
  }

  void enterLiteralExpr(TmplangParser::LiteralExprContext *ctx) override {
    if (ctx->literal()->IntegerLiteral() != nullptr) {
      nodeTypes.put(ctx, addConcreteType("int"));
    }
    else if (ctx->literal()->BoolLiteral() != nullptr) {
      nodeTypes.put(ctx, addConcreteType("bool"));
    }
    else if (ctx->literal()->CharacterLiteral() != nullptr) {
      nodeTypes.put(ctx, addConcreteType("char"));
    }
    else {
      std::cout << "unparsable literal!!\n";
    }
  }

  void enterParenExpr(TmplangParser::ParenExprContext *ctx) override {
    nodeTypes.put(ctx, addTypeVar());
  }

  void exitParenExpr(TmplangParser::ParenExprContext *ctx) override {
    equations.push_back(TypeEquation{ nodeTypes.get(ctx), nodeTypes.get(ctx->expr()) });
  }
};



class Checker : public TmplangBaseListener {
 public:
  Checker(tree::ParseTreeProperty<std::shared_ptr<Scope>>& _scopes, std::unordered_map<int, Type*> _subst) : scopes(_scopes), subst(_subst) {}

  tree::ParseTreeProperty<std::shared_ptr<Scope>>& scopes;
  Scope *currentScope;
  std::unordered_map<int, Type*> subst;

  void enterFile(TmplangParser::FileContext *ctx) override {
    currentScope = scopes.get(ctx).get();
  }

  void enterFunction(TmplangParser::FunctionContext *ctx) override {
    Type *varType = currentScope->resolve(ctx->identifier()->getText());
    Type *inferredType = applyUnifier(varType, subst);
    std::cout << ctx->identifier()->getText() << ": ";
    inferredType->print();
    std::cout << "\n";

    currentScope = scopes.get(ctx).get();
  }

  void enterFunctionParamDecl(TmplangParser::FunctionParamDeclContext *ctx) override {
    Type *varType = currentScope->resolve(ctx->identifier()->getText());
    Type *inferredType = applyUnifier(varType, subst);
    std::cout << ctx->identifier()->getText() << ": ";
    inferredType->print();
    std::cout << "\n";
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

  void enterVarDeclStatement(TmplangParser::VarDeclStatementContext *ctx) override {
    Type *varType = currentScope->resolve(ctx->identifier()->getText());
    Type *inferredType = applyUnifier(varType, subst);
    std::cout << ctx->identifier()->getText() << ": ";
    inferredType->print();
    std::cout << "\n";
  }
};


class Transpiler : public TmplangBaseVisitor {
 public:
  std::ostringstream oss;

  Transpiler(tree::ParseTreeProperty<std::shared_ptr<Scope>>& _scopes, std::unordered_map<int, Type*> _subst) : scopes(_scopes), subst(_subst) {}

  tree::ParseTreeProperty<std::shared_ptr<Scope>>& scopes;
  Scope *currentScope;
  Type *currentFunctionType;
  std::unordered_map<int, Type*> subst;


  antlrcpp::Any visitFile(TmplangParser::FileContext *ctx) override {
    currentScope = scopes.get(ctx).get();

    for (auto *func : ctx->function()) {
      visit(func);
    }
    return antlrcpp::Any();
  }

  antlrcpp::Any visitFunction(TmplangParser::FunctionContext *ctx) override {
    currentFunctionType = applyUnifier(currentScope->findSymbol(ctx->identifier()->getText()), subst);
    currentScope = scopes.get(ctx).get();

    Type *returnType = applyUnifier(dynamic_cast<FunctionType*>(currentFunctionType)->to, subst);

    oss << dynamic_cast<ConcreteType*>(returnType)->name << " " << ctx->identifier()->getText() << "(";

    if (ctx->functionParams() != nullptr) {
      visit(ctx->functionParams());
    }

    oss << ")";

    visit(ctx->blockStatement());
    oss << "\n";

    currentScope = currentScope->parent;
    return antlrcpp::Any();
  }

  antlrcpp::Any visitFunctionParams(TmplangParser::FunctionParamsContext *ctx) override {
    for (auto i = 0; i < ctx->functionParamDecl().size(); i++) {
      // we don't need to infer function param type, because it always needs to be concrete.
      Type *paramType = currentScope->findSymbol(ctx->functionParamDecl()[i]->identifier()->getText());
      oss << dynamic_cast<ConcreteType*>(paramType)->name << " " << ctx->functionParamDecl()[i]->identifier()->getText();
      if (i + 1 < ctx->functionParamDecl().size()) {
        oss << ", ";
      }
    }
    return antlrcpp::Any();
  }

  antlrcpp::Any visitBlockStatement(TmplangParser::BlockStatementContext *ctx) override {
    currentScope = scopes.get(ctx).get();

    oss << " {\n";
    visitChildren(ctx);
    oss << "\n}\n";

    currentScope = currentScope->parent;
    return antlrcpp::Any();
  }

  antlrcpp::Any visitIfStatement(TmplangParser::IfStatementContext *ctx) override {
    currentScope = scopes.get(ctx).get();

    oss << "if (";
    visit(ctx->expr());
    oss << ")";

    visit(ctx->blockStatement()[0]);

    if (ctx->blockStatement().size() > 1) {
      oss << "else ";
      visit(ctx->blockStatement()[1]);
    }
    else if (ctx->ifStatement() != nullptr) {
      visit(ctx->ifStatement());
    }

    currentScope = currentScope->parent;
    return antlrcpp::Any();
  }
};


int main(int argc, const char *argv[]) {
  ANTLRInputStream input(std::cin);
  TmplangLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  TmplangParser parser(&tokens);

  tree::ParseTree *tree = parser.file();

  SymbolTableGenerator symgen;
  tree::ParseTreeWalker::DEFAULT.walk(&symgen, tree);

  auto symbolTable = std::move(symgen.scopes);

  TypeEquationGenerater eqgen(symbolTable);
  tree::ParseTreeWalker::DEFAULT.walk(&eqgen, tree);

  for (auto& eq : eqgen.equations) {
    std::cout << "equation ===========\n";
    eq.left->print();
    std::cout << "  ==  ";
    eq.right->print();
    std::cout << '\n';
  }

  auto subst = unifyAllEquations(eqgen.equations);
  if (!subst.has_value()) {
    std::cout << "Type inference failed...\n";
    return 0;
  }
  else {
    std::cout << "Type inference succeeded!!\n";
    for (auto kv : subst.value()) {
      std::cout << "Type var id: " << kv.first << " -> ";
      kv.second->print();
      std::cout << "\n";
    }
  }

  std::cout << "---------------------------\n";
  std::cout << "Type inference result\n";
  Checker checker(symbolTable, subst.value());
  tree::ParseTreeWalker::DEFAULT.walk(&checker, tree);

  std::cout << "\n";
  std::cout << "transpiled result: \n\n";
  Transpiler transpiler(symbolTable, subst.value());
  transpiler.visit(tree);

  std::cout << transpiler.oss.str();
  return 0;
}
