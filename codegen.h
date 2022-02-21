#ifndef _ASMLAI_CODEGEN_H
#define _ASMLAI_CODEGEN_H

#include "parser.h"

namespace codegen {
void gen_code(const std::vector<parser::NodePtr> &nodes);
};

#endif
