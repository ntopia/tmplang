#ifndef HM_TYPE_INFERENCE_H_
#define HM_TYPE_INFERENCE_H_

#include <vector>
#include <unordered_map>
#include <optional>

#include "Type.h"


struct TypeEquation {
  Type *left, *right;
};

std::optional<std::unordered_map<int, Type*>> unifyAllEquations(std::vector<TypeEquation>& equations);

Type* applyUnifier(Type *type, std::unordered_map<int, Type*> subst);

#endif
