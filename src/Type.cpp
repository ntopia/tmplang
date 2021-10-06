#include <vector>
#include <memory>
#include <iostream>

#include "Type.h"


Type::~Type() {
}


TypeVar::~TypeVar() {
}

void TypeVar::print() {
  std::cout << "Var (id: " << id << ")";
}

bool TypeVar::equal(Type *rhs) {
  auto *t = dynamic_cast<TypeVar*>(rhs);
  return t != nullptr && id == t->id;
}


ConcreteType::ConcreteType() {
}

ConcreteType::ConcreteType(std::string&& _name): name(_name) {
}

ConcreteType::~ConcreteType() {
}

void ConcreteType::print() {
  std::cout << "Concrete " << name;
}

bool ConcreteType::equal(Type *rhs) {
  auto *t = dynamic_cast<ConcreteType*>(rhs);
  return t != nullptr && name == t->name;
}


FunctionType::~FunctionType() {
}

void FunctionType::print() {
  std::cout << "Func (";
  for (auto arg : from) {
    arg->print();
    std::cout << ", ";
  }
  std::cout << ") -> ";
  to->print();
}

bool FunctionType::equal(Type *rhs) {
  auto *t = dynamic_cast<FunctionType*>(rhs);
  if (t == nullptr || !to->equal(t->to)) return false;
  if (from.size() != t->from.size()) return false;
  for (int i = 0; i < from.size(); i++) {
    if (!from[i]->equal(t->from[i])) return false;
  }
  return true;
}

static std::vector<std::unique_ptr<Type>>& getTypeDump() {
  static std::vector<std::unique_ptr<Type>> typeDump;
  return typeDump;
}

TypeVar* addTypeVar() {
  static int typecounter = 0;
  TypeVar type;
  type.id = typecounter++;
  getTypeDump().push_back(std::make_unique<TypeVar>(type));
  return dynamic_cast<TypeVar*>(getTypeDump().back().get());
}

ConcreteType* addConcreteType(const std::string& name) {
  ConcreteType type;
  type.name = name;
  getTypeDump().push_back(std::make_unique<ConcreteType>(type));
  return dynamic_cast<ConcreteType*>(getTypeDump().back().get());
}

FunctionType* addFunctionType() {
  FunctionType type;
  getTypeDump().push_back(std::make_unique<FunctionType>(type));
  return dynamic_cast<FunctionType*>(getTypeDump().back().get());
}
