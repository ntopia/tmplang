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

int typecounter = 0;
auto *typeDump = new std::vector<std::unique_ptr<Type>>();

TypeVar* addTypeVar() {
  TypeVar type;
  type.id = typecounter++;
  typeDump->push_back(std::make_unique<TypeVar>(type));
  return dynamic_cast<TypeVar*>(typeDump->back().get());
}

ConcreteType* addConcreteType(const std::string& name) {
  ConcreteType type;
  type.name = name;
  typeDump->push_back(std::make_unique<ConcreteType>(type));
  return dynamic_cast<ConcreteType*>(typeDump->back().get());
}

FunctionType* addFunctionType() {
  FunctionType type;
  typeDump->push_back(std::make_unique<FunctionType>(type));
  return dynamic_cast<FunctionType*>(typeDump->back().get());
}
