#include <gtest/gtest.h>
#include "parser.h"
#include "ast.h"

TEST(ParserTest, ExpressionStatement)
{
  const char *input = "print 1;";

  auto statements = parse(input);
  EXPECT_EQ(statements.size(), 1);
  EXPECT_EQ(statements[0]->type_, StmtType::Expression);
}
