#include <vector>
#include <string>

struct Type {
  virtual ~Type();
  virtual void print() = 0;
  virtual bool equal(Type *rhs) = 0;
};
struct TypeVar : public Type {
  int id;

  ~TypeVar();
  void print();
  bool equal(Type *rhs);
};
struct ConcreteType : public Type {
  std::string name;

  ConcreteType();
  ConcreteType(std::string&& _name);
  ~ConcreteType();

  void print();
  bool equal(Type *rhs);
};
struct FunctionType : public Type {
  std::vector<Type*> from;
  Type *to;

  ~FunctionType();

  void print();
  bool equal(Type *rhs);
};

TypeVar* addTypeVar();
ConcreteType* addConcreteType(const std::string& name);
FunctionType* addFunctionType();
