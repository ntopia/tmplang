#include <iostream>
#include <string>
#include <random>
#include <memory>
#include <unordered_map>

#include "antlr4-runtime.h"
#include "TmplangParser.h"

#include "Type.h"
#include "SymbolTable.h"

using namespace antlr4;



std::string getRandomString(int len) {
  static std::string randomchar = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, randomchar.length() - 1);
  std::string out;
  for (int i = 0; i < len; i++) {
    out += randomchar[dist(gen)];
  }
  return out;
}



bool Scope::addSymbol(const std::string& name, Type* type) {
  return symbols.emplace(name, type).second;
}
Type* Scope::findSymbol(const std::string& name) {
  auto it = symbols.find(name);
  return (it == symbols.end()) ? nullptr : it->second;
}
Type* Scope::resolve(const std::string& name) {
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

std::shared_ptr<Scope> makeRootScope() {
  Scope scope;
  scope.id = "root_" + getRandomString(8);
  scope.kind = ROOT;
  scope.parent = nullptr;
  return std::make_shared<Scope>(scope);
}

std::shared_ptr<Scope> makeFunctionScope(const std::string& fname, Scope* parent) {
  Scope scope;
  scope.id = "function_" + fname + "_" + getRandomString(8);
  scope.kind = FUNCTION;
  scope.parent = parent;
  return std::make_shared<Scope>(scope);
}

std::shared_ptr<Scope> makeBlockScope(Scope* parent) {
  Scope scope;
  scope.id = "block_" + getRandomString(8);
  scope.kind = BLOCK;
  scope.parent = parent;
  return std::make_shared<Scope>(scope);
}



void SymbolTableGenerator::enterFile(TmplangParser::FileContext *ctx) {
  scopes.put(ctx, makeRootScope());

  // move downward
  currentScope = scopes.get(ctx).get();
}

void SymbolTableGenerator::enterFunctionParamDecl(TmplangParser::FunctionParamDeclContext *ctx) {
  auto *concreteType = addConcreteType(ctx->type()->getText());

  if (!currentScope->addSymbol(ctx->identifier()->getText(), concreteType)) {
    std::cout << "param decl collision!!!\n";
  }
}

void SymbolTableGenerator::exitFunctionParams(TmplangParser::FunctionParamsContext *ctx) {
  auto *functionType = dynamic_cast<FunctionType*>(currentFunctionType);

  for (auto *decl : ctx->functionParamDecl()) {
    auto *symbolType = currentScope->findSymbol(decl->identifier()->getText());
    if (symbolType == nullptr) {
      std::cout << "symbol definition not found!!!\n";
    }
    functionType->from.push_back(symbolType);
  }
}

void SymbolTableGenerator::enterFunction(TmplangParser::FunctionContext *ctx) {
  auto *functionType = addFunctionType();

  if (!currentScope->addSymbol(ctx->identifier()->getText(), functionType)) {
    std::cout << "function decl collision!!!\n";
  }

  scopes.put(ctx, makeFunctionScope(ctx->identifier()->getText(), currentScope));
  currentScope->children.emplace(scopes.get(ctx)->id, scopes.get(ctx).get());

  // move downward
  currentScope = scopes.get(ctx).get();
  currentFunctionType = functionType;
}

void SymbolTableGenerator::exitFunction(TmplangParser::FunctionContext *ctx) {
  // move upward
  currentScope = currentScope->parent;
}

void SymbolTableGenerator::enterBlockStatement(TmplangParser::BlockStatementContext *ctx) {
  scopes.put(ctx, makeBlockScope(currentScope));
  currentScope->children.emplace(scopes.get(ctx)->id, scopes.get(ctx).get());

  // move downward
  currentScope = scopes.get(ctx).get();
}

void SymbolTableGenerator::exitBlockStatement(TmplangParser::BlockStatementContext *ctx) {
  // move upward
  currentScope = currentScope->parent;
}

void SymbolTableGenerator::enterIfStatement(TmplangParser::IfStatementContext *ctx) {
  scopes.put(ctx, makeBlockScope(currentScope));
  currentScope->children.emplace(scopes.get(ctx)->id, scopes.get(ctx).get());

  // move downward
  currentScope = scopes.get(ctx).get();
}

void SymbolTableGenerator::exitIfStatement(TmplangParser::IfStatementContext *ctx) {
  // move upward
  currentScope = currentScope->parent;
}

void SymbolTableGenerator::exitVarDeclStatement(TmplangParser::VarDeclStatementContext *ctx) {
  Type *varType;
  if (ctx->type() == nullptr) {
    varType = addTypeVar();
  }
  else {
    varType = addConcreteType(ctx->type()->getText());
  }

  if (!currentScope->addSymbol(ctx->identifier()->getText(), varType)) {
    std::cout << "var decl collision!!!\n";
  }
}
