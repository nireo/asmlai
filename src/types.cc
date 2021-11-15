#include "types.h"
#include "ast.h"

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
