#ifndef _ASMLAI_TYPESYSTEM_H
#define _ASMLAI_TYPESYSTEM_H

#include "parser.h"
#include <memory>
namespace typesystem {
void add_type(parser::Node &node);
parser::Type *ptr_to(parser::Type *base);
} // namespace typesystem

#endif
