#ifndef _ASMLAI_TYPESYSTEM_H
#define _ASMLAI_TYPESYSTEM_H

#include "parser.h"
#include <memory>
namespace typesystem {
void add_type(parser::Node &node);
parser::Type *ptr_to(parser::Type *base);
parser::Type *func_ty(parser::Type *ty);
parser::Type *array_of_type(parser::Type *array_type, i32 length);
bool is_number(parser::Type *ty);
} // namespace typesystem

#endif
