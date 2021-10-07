#ifndef TRANSPILER_H_
#define TRANSPILER_H_

#include <vector>
#include <unordered_map>
#include <utility>
#include <memory>
#include <sstream>

#include "antlr4-runtime.h"
#include "TmplangParser.h"
#include "TmplangBaseVisitor.h"

#include "Type.h"
#include "SymbolTable.h"

using namespace antlr4;


class Transpiler : public TmplangBaseVisitor {
 public:
  std::ostringstream oss;
  int indentLevel;

  Transpiler(tree::ParseTreeProperty<std::shared_ptr<Scope>>& _scopes, std::unordered_map<int, Type*> _subst);

  tree::ParseTreeProperty<std::shared_ptr<Scope>>& scopes;
  Scope *currentScope;
  Type *currentFunctionType;
  std::unordered_map<int, Type*> subst;


  antlrcpp::Any visitFile(TmplangParser::FileContext *ctx) override;

  antlrcpp::Any visitFunction(TmplangParser::FunctionContext *ctx) override;

  antlrcpp::Any visitFunctionParams(TmplangParser::FunctionParamsContext *ctx) override;

  antlrcpp::Any visitBlockStatement(TmplangParser::BlockStatementContext *ctx) override;

  antlrcpp::Any visitIfStatement(TmplangParser::IfStatementContext *ctx) override;

  antlrcpp::Any visitVarDeclStatement(TmplangParser::VarDeclStatementContext *ctx) override;

  antlrcpp::Any visitAssignStatement(TmplangParser::AssignStatementContext *ctx) override;

  antlrcpp::Any visitReturnStatement(TmplangParser::ReturnStatementContext *ctx) override;

  antlrcpp::Any visitNormalStatement(TmplangParser::NormalStatementContext *ctx) override;

 private:
  std::vector<std::pair<std::string, Type*>> emitAllVarDecls(Scope *root);

};

#endif
