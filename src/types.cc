#include "types.h"
#include "ast.h"
#include "codegen_x64.h"

bool typesystem::is_number(const ValueT type) {
  if (type == TYPE_CHAR || type == TYPE_INT || type == TYPE_LONG)
    return true;

  return false;
}

bool typesystem::is_ptr(const ValueT type) {
  if (type == TYPE_PTR_CHAR || type == TYPE_PTR_INT || type == TYPE_PTR_LONG ||
      type == TYPE_PTR_VOID) {
    return true;
  }

  return false;
}

ValueT typesystem::convert_to_ptr(const ValueT type) {
  switch (type) {
  case TYPE_VOID: {
    return TYPE_PTR_VOID;
  }
  case TYPE_CHAR: {
    return TYPE_PTR_CHAR;
  }
  case TYPE_INT: {
    return TYPE_PTR_INT;
  }
  case TYPE_LONG: {
    return TYPE_PTR_LONG;
  }
  default:
    std::fprintf(stderr, "cannot convert type: %d into pointer\n",
                 static_cast<int>(type));
    std::exit(1);
  }
}

ValueT typesystem::convert_from_ptr(const ValueT type) {
  switch (type) {
  case TYPE_PTR_VOID:
    return TYPE_VOID;
  case TYPE_PTR_CHAR:
    return TYPE_CHAR;
  case TYPE_PTR_INT:
    return TYPE_INT;
  case TYPE_PTR_LONG:
    return TYPE_LONG;
  default: {
    std::fprintf(stderr, "type is not convertable to normal type.");
    std::exit(1);
  }
  }
}

std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>
typesystem::change_type(std::unique_ptr<Expression> exp, ValueT change_type,
                        TokenType infix_opr) {
  auto exp_type = exp->ValueType();
  if (is_number(change_type) && is_number(exp_type)) {
    if (change_type == exp->ValueType())
      return {nullptr, std::move(exp)};

    int size_1 = codegen::get_bytesize_of_type(exp_type);
    int size_2 = codegen::get_bytesize_of_type(change_type);

    if (size_1 > size_2)
      return {std::move(exp), nullptr};

    if (size_2 > size_1) {
      return {nullptr, std::move(exp)};
    }
  }

  if (is_ptr(exp_type)) {
    if (exp->Type() != AstType::InfixExpression && exp_type == change_type)
      return {nullptr, std::move(exp)};
  }

  if (infix_opr == TokenType::Plus || infix_opr == TokenType::Minus) {
    if (is_number(exp_type) && is_ptr(change_type)) {
      int size = codegen::get_bytesize_of_type(convert_from_ptr(change_type));
      if (size > 1) {
        auto wrapper = std::make_unique<TypeChangeAction>();
        wrapper->size = size;
        wrapper->inner_ = std::move(exp);
        wrapper->action_ = TypeChange::Scale;

        return {nullptr, std::move(wrapper)};
      } else {
        return {std::move(exp), nullptr};
      }
    }
  }

  // type cannot be change, and thus is not compatible
  return {std::move(exp), nullptr};
}
