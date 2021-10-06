#include <vector>
#include <unordered_map>
#include <optional>

#include "Type.h"
#include "HMTypeInference.h"


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
