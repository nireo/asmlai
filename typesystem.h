#ifndef _ASMLAI_TYPESYSTEM_H
#define _ASMLAI_TYPESYSTEM_H

#include "parser.h"
#include <memory>
namespace typesystem {
void add_type(parser::Node &node);
parser::Type *ptr_to(parser::Type *base);
parser::Type *func_ty(parser::Type *ty);
} // namespace typesystem

#endif
