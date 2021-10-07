#include "Transpiler.h"

#include <string>
#include <queue>

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

std::vector<std::pair<std::string, Type*>> Transpiler::emitAllVarDecls(Scope *root) {
  std::vector<std::pair<std::string, Type*>> results;

  std::queue<Scope*> q;
  q.push(root);
  while (!q.empty()) {
    Scope *scope = q.front();
    q.pop();

    for (auto& kv : scope->symbols) {
      results.emplace_back(kv.first + "_" + scope->id, kv.second);
    }

    for (auto& kv : scope->children) {
      q.push(kv.second);
    }
  }

  return results;
}

antlrcpp::Any Transpiler::visitBlockStatement(TmplangParser::BlockStatementContext *ctx) {
  currentScope = scopes.get(ctx).get();

  oss << std::string(indentLevel * 2, ' ') << "{\n";
  indentLevel++;

  // when a block is function-starting block, prints all variable declarations first.
  if (dynamic_cast<TmplangParser::FunctionContext*>(ctx->parent) != nullptr) {
    auto decls = emitAllVarDecls(currentScope);
    for (auto& decl : decls) {
      Type* varType = applyUnifier(decl.second, subst);
      oss << std::string(indentLevel * 2, ' ') << dynamic_cast<ConcreteType*>(varType)->name << " " << decl.first << ";\n";
    }
  }
  oss << "\n";

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
  if (ctx->expr() == nullptr) {
    // TODO: needs to print a default value
    return antlrcpp::Any();
  }

  oss << std::string(indentLevel * 2, ' ') << ctx->identifier()->getText() << " = ";

  visit(ctx->expr());

  oss << ";\n";
  return antlrcpp::Any();
}

antlrcpp::Any Transpiler::visitAssignStatement(TmplangParser::AssignStatementContext *ctx) {
  oss << std::string(indentLevel * 2, ' ') << ctx->identifier()->getText() << " = ";

  visit(ctx->expr());

  oss << ";\n";
  return antlrcpp::Any();
}

antlrcpp::Any Transpiler::visitReturnStatement(TmplangParser::ReturnStatementContext *ctx) {
  oss << std::string(indentLevel * 2, ' ') << "return ";

  visit(ctx->expr());

  oss << ";\n";
  return antlrcpp::Any();
}

antlrcpp::Any Transpiler::visitNormalStatement(TmplangParser::NormalStatementContext *ctx) {
  oss << std::string(indentLevel * 2, ' ');

  visit(ctx->expr());

  oss << ";\n";
  return antlrcpp::Any();
}
