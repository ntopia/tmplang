#include "Transpiler.h"

#include <string>

#include "HMTypeInference.h"
#include "Type.h"


Transpiler::Transpiler(tree::ParseTreeProperty<std::shared_ptr<Scope>>& _scopes, std::unordered_map<int, Type*> _subst) : scopes(_scopes), subst(_subst) {
}

antlrcpp::Any Transpiler::visitFile(TmplangParser::FileContext *ctx) {
  currentScope = scopes.get(ctx).get();
  indentLevel = 0;

  for (auto *func : ctx->function()) {
    visit(func);
  }
  return antlrcpp::Any();
}

antlrcpp::Any Transpiler::visitFunction(TmplangParser::FunctionContext *ctx) {
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

antlrcpp::Any Transpiler::visitFunctionParams(TmplangParser::FunctionParamsContext *ctx) {
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

antlrcpp::Any Transpiler::visitBlockStatement(TmplangParser::BlockStatementContext *ctx) {
  currentScope = scopes.get(ctx).get();

  oss << std::string(indentLevel * 2, ' ') << "{\n";
  indentLevel++;
  visitChildren(ctx);
  indentLevel--;
  oss << std::string(indentLevel * 2, ' ') << "}\n";

  currentScope = currentScope->parent;
  return antlrcpp::Any();
}

antlrcpp::Any Transpiler::visitIfStatement(TmplangParser::IfStatementContext *ctx) {
  currentScope = scopes.get(ctx).get();

  oss << std::string(indentLevel * 2, ' ') << "if (";
  visit(ctx->expr());
  oss << ")\n";

  visit(ctx->blockStatement()[0]);

  if (ctx->blockStatement().size() > 1) {
    oss << std::string(indentLevel * 2, ' ') << "else\n";
    visit(ctx->blockStatement()[1]);
  }
  else if (ctx->ifStatement() != nullptr) {
    visit(ctx->ifStatement());
  }

  currentScope = currentScope->parent;
  return antlrcpp::Any();
}

antlrcpp::Any Transpiler::visitVarDeclStatement(TmplangParser::VarDeclStatementContext *ctx) {
  Type *varType = applyUnifier(currentScope->findSymbol(ctx->identifier()->getText()), subst);

  oss << std::string(indentLevel * 2, ' ') << dynamic_cast<ConcreteType*>(varType)->name << " " << ctx->identifier()->getText() + "_" + currentScope->id << ";\n";

  visitChildren(ctx);

  return antlrcpp::Any();
}
