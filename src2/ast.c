#include "ast.h"

std::string
Program::TokenLiteral() const
{
  if(statements.size() > 0) {
    return statements[0].get()->TokenLiteral();
  }

  return "";
}

std::string
Program::String()
{
  std::string res;
  for(int i = 0; i < (int)statements.size(); ++i) {
    res += statements[i]->String();
  }

  return res;
}

std::string
LetStatement::String()
{
  std::string res = TokenLiteral() + " " + name->String() + " = ";

  if(value != nullptr) {
    res += value->String();
  }

  res += ";";
  return res;
}

std::string
ReturnStatement::String()
{
  if(return_value != nullptr) {
    return "return " + return_value->String();
  }

  return "return;";
}

std::string
ExpressionStatement::String()
{
  if(expression != nullptr) {
    return expression->String();
  }

  return "";
}

std::string
PrefixExpression::String()
{
  return "(" + opr + right->String() + ")";
}

std::string
InfixExpression::String()
{
  return "(" + left->String() + " " + opr + " " + right->String() + ")";
}

std::string
CallExpression::String()
{
  std::string argstring;
  for(auto &arg : arguments)
    argstring += arg->String();

  return func->String() + "(" + argstring + ")";
}

std::string
ArrayLiteral::String()
{
  std::string res = "[";
  for(const auto &exp : elements)
    res += exp->String();
  res += "]";

  return res;
}

std::string
IndexExpression::String()
{
  return "(" + left->String() + "[" + index->String() + "])";
}
