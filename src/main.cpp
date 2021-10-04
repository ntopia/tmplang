#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

#include "antlr4-runtime.h"
#include "TmplangLexer.h"
#include "TmplangParser.h"
#include "TmplangBaseListener.h"

#include "Type.h"

using namespace antlr4;



struct Scope {
  std::unordered_map<std::string, Type*> symbols;
  Scope *parent;

  Type* resolve(const std::string& name) {
    Scope *scope = this;
    while (scope != nullptr) {
      auto it = scope->symbols.find(name);
      if (it != scope->symbols.end()) {
        return it->second;
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
    auto *concreteType = addConcreteType(ctx->type()->getText());

    auto insertRet = currentScope->symbols.emplace(ctx->identifier()->getText(), concreteType);
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
    auto *functionType = addFunctionType();

    auto insertRet = currentScope->symbols.emplace(ctx->identifier()->getText(), functionType);
    if (!insertRet.second) {
      std::cout << "function decl collision!!!\n";
    }

    scopes.put(ctx, std::make_shared<Scope>());
    scopes.get(ctx)->parent = currentScope;

    // move downward
    currentScope = scopes.get(ctx).get();
    currentFunctionType = currentScope->parent->symbols.find(ctx->identifier()->getText())->second;
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
    Type *varType;
    if (ctx->type() == nullptr) {
      varType = addTypeVar();
    }
    else {
      varType = addConcreteType(ctx->type()->getText());
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


std::optional<std::unordered_map<int, Type*>> unifyVariable(TypeVar *typeVar, Type *type, std::unordered_map<int, Type*> subst);

std::optional<std::unordered_map<int, Type*>> unify(Type *x, Type *y, std::optional<std::unordered_map<int, Type*>> subst) {
  if (!subst.has_value()) {
    return std::nullopt;
  }
  if (x->equal(y)) {
    return subst;
  }
  auto *varX = dynamic_cast<TypeVar*>(x);
  if (varX != nullptr) {
    return unifyVariable(varX, y, subst.value());
  }
  auto *varY = dynamic_cast<TypeVar*>(y);
  if (varY != nullptr) {
    return unifyVariable(varY, x, subst.value());
  }
  auto *funcX = dynamic_cast<FunctionType*>(x);
  auto *funcY = dynamic_cast<FunctionType*>(y);
  if (funcX != nullptr && funcY != nullptr) {
    if (funcX->from.size() != funcY->from.size()) {
      return std::nullopt;
    }
    subst = unify(funcX->to, funcY->to, subst);
    for (int i = 0; i < funcX->from.size(); i++) {
      subst = unify(funcX->from[i], funcY->from[i], subst);
    }
    return subst;
  }
  return std::nullopt;
}

bool occursCheck(TypeVar *typeVar, Type *type, std::unordered_map<int, Type*>& subst) {
  if (type->equal((Type*)typeVar)) {
    return true;
  }
  TypeVar *utypeVar = dynamic_cast<TypeVar*>(type);
  if (utypeVar != nullptr && subst.find(utypeVar->id) != subst.end()) {
    return occursCheck(typeVar, subst[utypeVar->id], subst);
  }
  FunctionType *fType = dynamic_cast<FunctionType*>(type);
  if (fType != nullptr) {
    if (occursCheck(typeVar, fType->to, subst)) {
      return true;
    }
    for (auto *arg : fType->from) {
      if (occursCheck(typeVar, arg, subst)) {
        return true;
      }
    }
  }
  return false;
}

std::optional<std::unordered_map<int, Type*>> unifyVariable(TypeVar *typeVar, Type *type, std::unordered_map<int, Type*> subst) {
  if (subst.find(typeVar->id) != subst.end()) {
    return unify(subst[typeVar->id], type, subst);
  }
  TypeVar *utypeVar = dynamic_cast<TypeVar*>(type);
  if (utypeVar != nullptr && subst.find(utypeVar->id) != subst.end()) {
    return unify(typeVar, subst[utypeVar->id], subst);
  }
  if (occursCheck(typeVar, type, subst)) {
    return std::nullopt;
  }

  std::unordered_map<int, Type*> newSubst = subst;
  newSubst.emplace(typeVar->id, type);
  return newSubst;
}

std::optional<std::unordered_map<int, Type*>> unifyAllEquations(std::vector<TypeEquation>& equations) {
  std::optional<std::unordered_map<int, Type*>> subst = std::unordered_map<int, Type*>{};
  for (auto& eq : equations) {
    subst = unify(eq.left, eq.right, subst);
    if (subst == std::nullopt) break;
  }
  return subst;
}

std::vector<std::shared_ptr<Type>> tmpTypes;

Type* applyUnifier(Type *type, std::unordered_map<int, Type*> subst) {
  if (subst.empty()) {
    return type;
  }
  ConcreteType intType{ "int" };
  ConcreteType boolType{ "bool" };
  ConcreteType charType{ "char" };
  if (type->equal(&intType) || type->equal(&boolType) || type->equal(&charType)) {
    return type;
  }
  auto *typeVar = dynamic_cast<TypeVar*>(type);
  if (typeVar != nullptr) {
    if (subst.find(typeVar->id) != subst.end()) {
      return applyUnifier(subst[typeVar->id], subst);
    }
    else {
      return type;
    }
  }
  auto *fType = dynamic_cast<FunctionType*>(type);
  if (fType != nullptr) {
    auto *newFunctionType = addFunctionType();
    for (auto *arg : fType->from) {
      newFunctionType->from.push_back(applyUnifier(arg, subst));
    }
    newFunctionType->to = applyUnifier(fType->to, subst);
    return newFunctionType;
  }
  return nullptr;
}


class Checker : public TmplangBaseListener {
 public:
  Checker(tree::ParseTreeProperty<std::shared_ptr<Scope>> _scopes, std::unordered_map<int, Type*> _subst) : scopes(_scopes), subst(_subst) {}

  tree::ParseTreeProperty<std::shared_ptr<Scope>> scopes;
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

  auto subst = unifyAllEquations(inferencer.equations);
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
  Checker checker(std::move(inferencer.scopes), subst.value());
  tree::ParseTreeWalker::DEFAULT.walk(&checker, tree);
  return 0;
}
