#include "Common.h"
#include "test_runner.h"

#include <sstream>

using namespace std;

class ValueExpression : public Expression {
private:
  int mValue {0};
public:
  ValueExpression(int value) : mValue(value) {

  }

  virtual int Evaluate() const override  {
    return mValue;
  }

  virtual std::string ToString() const override {
    return to_string(mValue);
  }
};

class OperationExpression : public Expression {
private:
  virtual char Operation() const = 0;
protected:
  ExpressionPtr leftExp;
  ExpressionPtr rightExp;
public:
  OperationExpression(ExpressionPtr left, ExpressionPtr right) : leftExp(std::move(left)),
                                                                 rightExp(std::move(right))
  {}

  virtual std::string ToString() const override {
    return "(" + leftExp->ToString() + ")" + Operation() + "(" + rightExp->ToString() + ")";
  }
};

class SumExpression : public OperationExpression {
private:
  virtual char Operation() const override {
    return '+';
  }

public:
  SumExpression(ExpressionPtr left, ExpressionPtr right) : OperationExpression(std::move(left), std::move(right))
  {

  }

  virtual int Evaluate() const override  {
    return leftExp->Evaluate() + rightExp->Evaluate();
  }
};

class ProductExpression : public OperationExpression {
private:
  virtual char Operation() const override {
    return '*';
  }
  
public:
  using OperationExpression::OperationExpression;

  virtual int Evaluate() const override  {
    return leftExp->Evaluate() * rightExp->Evaluate();
  }
};

ExpressionPtr Value(int value) {
  return std::make_unique<ValueExpression>(value);
}

ExpressionPtr Sum(ExpressionPtr left, ExpressionPtr right) {
  return std::make_unique<SumExpression>(std::move(left), std::move(right));
}

ExpressionPtr Product(ExpressionPtr left, ExpressionPtr right) { 
  return std::make_unique<ProductExpression>(std::move(left), std::move(right));
}

string Print(const Expression* e) {
  if (!e) {
    return "Null expression provided";
  }
  stringstream output;
  output << e->ToString() << " = " << e->Evaluate();
  return output.str();
}

void Test() {
  ExpressionPtr e1 = Product(Value(2), Sum(Value(3), Value(4)));
  ASSERT_EQUAL(Print(e1.get()), "(2)*((3)+(4)) = 14");

  ExpressionPtr e2 = Sum(move(e1), Value(5));
  ASSERT_EQUAL(Print(e2.get()), "((2)*((3)+(4)))+(5) = 19");

  ASSERT_EQUAL(Print(e1.get()), "Null expression provided");
}

int main() {
  TestRunner tr;
  RUN_TEST(tr, Test);
  return 0;
}
