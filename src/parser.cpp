#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include "ast.h"
#include "lexer.h"
#include "parser.h"

static std::unordered_map<TokenType, Precedence> precedences = {
  { TOKEN_EQUAL, EQUALS },     { TOKEN_BANG_EQUAL, EQUALS },
  { TOKEN_LESS, LESSGREATER }, { TOKEN_GREATER, LESSGREATER },
  { TOKEN_PLUS, SUM },         { TOKEN_MINUS, SUM },
  { TOKEN_SLASH, PRODUCT },    { TOKEN_STAR, PRODUCT },
  { TOKEN_LEFT_PAREN, CALL },  { TOKEN_LEFT_BRACKET, INDEX },
};

static std::unordered_map<TokenType, InfixParseFn> infix_fns = { {} };
static std::unordered_map<TokenType, PrefixParseFn> prefix_fns = { {} };

struct Parser {
  bool panic_mode;
  bool had_error;
  Token current;
  Token previous;
  Parser() : panic_mode(false), had_error(false) {}
};

Parser parser;

static std::vector<std::unique_ptr<Statement> > statements;

static void
error_at(Token *token, const char *message)
{
  if(parser.panic_mode)
    return;
  parser.panic_mode = true;

  fprintf(stderr, "[line %d] Error", token->line);

  if(token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if(token->type == TOKEN_ERROR) {
  } else {
    fprintf(stderr, " at '%.*s'", token->len, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.had_error = true;
}

static void
error(const char *message)
{
  error_at(&parser.previous, message);
}

static void
error_at_current(const char *message)
{
  error_at(&parser.current, message);
}

static void
advance(void)
{
  parser.previous = parser.current;

  for(;;) {
    parser.current = get_token();
    if(parser.current.type != TOKEN_ERROR)
      break;

    error_at_current(parser.current.start);
  }
}

static void
consume(TokenType type, const char *message)
{
  if(parser.current.type == type) {
    advance();
    return;
  }

  error_at_current(message);
}

static bool
check(TokenType type)
{
  return parser.current.type == type;
}

static bool
match(TokenType type)
{
  if(!check(type))
    return false;

  advance();

  return true;
}

std::unique_ptr<Expression>
parse_integer_literal()
{
  auto lit = std::make_unique<Expression>(ExprType::IntegerLiteral);
  std::int64_t value
      = (std::int64_t)std::strtod(parser.previous.start, nullptr);
  lit->int_lit_ = value;

  return lit;
}

static Precedence
get_curr_prec()
{
  if(precedences.find(parser.current.type) != precedences.end()) {
    return precedences[parser.current.type];
  }

  return LOWEST;
}

static std::unique_ptr<Expression>
parse_expression(Precedence prec)
{
  printf("start: %s", parser.previous.start);
  advance();
  if(prefix_fns.find(parser.previous.type) == prefix_fns.end()) {
    error("no prefix function found");
    return nullptr;
  }

  printf("start: %s", parser.previous.start);

  auto fn = prefix_fns[parser.current.type];
  auto left = fn();

  while(!match(TOKEN_SEMICOLON) && prec <= get_curr_prec()) {
    advance();
    auto infix = infix_fns[parser.previous.type];
    if(infix == nullptr) {
      return left;
    }

    left = infix(std::move(left));
  }

  return left;
}

static std::unique_ptr<Expression>
parse_infix_expression(std::unique_ptr<Expression> left)
{
  auto infix = std::make_unique<InfixExpr>();
  infix->opr = parser.current.type;
  infix->expr_left_ = std::move(left);

  Precedence prec = get_curr_prec();
  advance();

  infix->expr_right_ = std::move(parse_expression(prec));

  auto exp = std::make_unique<Expression>(ExprType::Infix);
  exp->infix = std::move(infix);

  return exp;
}

static std::unique_ptr<Statement>
parse_variable_declaration()
{
  auto type = parser.previous.type;
  std::unique_ptr<Expression> expr;
  if(match(TOKEN_EQUAL))
    expr = parse_expression(LOWEST);

  consume(TOKEN_SEMICOLON, "expected ';' after declaration");

  auto var_decl = std::make_unique<VarDecl>();
  var_decl->value = std::move(expr);
  var_decl->type = type;

  return var_decl;
}

static std::unique_ptr<Statement>
parse_print_statement()
{
  printf("prev: %s", parser.previous.start);
  printf("curr: %s", parser.current.start);
  std::unique_ptr<Expression> expr = parse_expression(LOWEST);
  consume(TOKEN_SEMICOLON, "expected ';' after print statement");

  auto stmt = std::make_unique<PrintStmt>();
  stmt->value = std::move(expr);

  return stmt;
}

static std::unique_ptr<Statement>
parse_expression_statement()
{
  auto expr_stmt = std::make_unique<ExprStmt>();

  auto expr = parse_expression(LOWEST);
  consume(TOKEN_SEMICOLON, "expect ';' after expression");

  auto stmt = std::make_unique<Statement>(StmtType::Expression);
  stmt->expr_stmt_ = std::move(expr_stmt);

  return stmt;
}

static std::unique_ptr<Statement>
parse_statement()
{
  if(match(TOKEN_INT) || match(TOKEN_CHAR)) {
    return parse_variable_declaration();
  } else if(match(TOKEN_PRINT)) {
    return parse_print_statement();
  }

  return parse_expression_statement();
}

std::vector<std::unique_ptr<Statement> >
parse(const char *source)
{
  std::vector<std::unique_ptr<Statement> > result;
  init_lexer(source);
  parser = Parser();
  prefix_fns[TOKEN_NUMBER] = parse_integer_literal;

  advance();

  while(!match(TOKEN_EOF)) {
    auto stmt = parse_statement();
    if(stmt == nullptr) {
      break;
    }

    result.push_back(std::move(stmt));
  }

  return std::move(statements);
}
