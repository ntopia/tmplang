#ifndef SYMBOL_TABLE_H_
#define SYMBOL_TABLE_H_

#include <string>
#include <unordered_map>
#include <memory>

#include "antlr4-runtime.h"
#include "TmplangParser.h"
#include "TmplangBaseListener.h"

#include "Type.h"

using namespace antlr4;


enum ScopeKind {
  ROOT,
  MODULE,
  FUNCTION,
  BLOCK,
};
struct Scope {
  std::string id;
  ScopeKind kind;
  std::unordered_map<std::string, Type*> symbols;
  Scope *parent;
  std::unordered_map<std::string, Scope*> children;

  bool addSymbol(const std::string& name, Type* type);
  Type* findSymbol(const std::string& name);
  Type* resolve(const std::string& name);
};


class SymbolTableGenerator : public TmplangBaseListener {
 public:
  tree::ParseTreeProperty<std::shared_ptr<Scope>> scopes;
  Type *currentFunctionType;
  Scope *currentScope;

  void enterFile(TmplangParser::FileContext *ctx) override;

  void enterFunctionParamDecl(TmplangParser::FunctionParamDeclContext *ctx) override;

  void exitFunctionParams(TmplangParser::FunctionParamsContext *ctx) override;

  void enterFunction(TmplangParser::FunctionContext *ctx) override;

  void exitFunction(TmplangParser::FunctionContext *ctx) override;

  void enterBlockStatement(TmplangParser::BlockStatementContext *ctx) override;

  void exitBlockStatement(TmplangParser::BlockStatementContext *ctx) override;

  void enterIfStatement(TmplangParser::IfStatementContext *ctx) override;

  void exitIfStatement(TmplangParser::IfStatementContext *ctx) override;

  void exitVarDeclStatement(TmplangParser::VarDeclStatementContext *ctx) override;
};

#endif
