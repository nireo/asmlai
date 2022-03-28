#ifndef _ASMLAI_CODEGEN_H
#define _ASMLAI_CODEGEN_H

#include "parser.h"
#include <bits/types/FILE.h>

namespace codegen {
void gen_code(std::vector<std::shared_ptr<parser::Object>> &&root, FILE *fp);
i64 align_to(i64 n, i64 align);
}; // namespace codegen

#endif
