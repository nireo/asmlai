#ifndef LAI_TYPES_H
#define LAI_TYPES_H

#include "ast.h"

namespace typesystem {
bool is_number(const ValueT);
bool is_ptr(const ValueT);
ValueT convert_to_ptr(const ValueT);
ValueT convert_from_ptr(const ValueT);
}; // namespace typesystem

#endif
