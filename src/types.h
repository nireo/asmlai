#ifndef LAI_TYPES_H
#define LAI_TYPES_H

#include "ast.h"
#include "token.h"

#include <memory>

namespace typesystem {
bool is_number(const ValueT);
bool is_ptr(const ValueT);
ValueT convert_to_ptr(const ValueT);
ValueT convert_from_ptr(const ValueT);
std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>
    change_type(std::unique_ptr<Expression>, ValueT, TokenType);
}; // namespace typesystem

#endif
