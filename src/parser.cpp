#include "parser.h"
#include "ast.h"
#include "token.h"
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

static const std::unordered_map<tokentypes, Precedence> precedences = {
  { tokentypes::Eq, EQUALS },      { tokentypes::Neq, EQUALS },
  { tokentypes::LT, LESSGREATER }, { tokentypes::GT, LESSGREATER },
  { tokentypes::Plus, SUM },       { tokentypes::Minus, SUM },
  { tokentypes::Slash, PRODUCT },  { tokentypes::Asterisk, PRODUCT },
  { tokentypes::LParen, CALL },    { tokentypes::LBracket, INDEX },
};

static std::unordered_map<tokentypes, InfixParseFn> m_infix_parse_fns;
static std::unordered_map<tokentypes, PrefixParseFn> m_prefix_parse_fns;

Parser parser;

void
Parser::next_token()
{
  current_ = peek_;
  peek_ = lx_->next_token();
}

bool
Parser::current_token_is(tokentypes tt)
{
  return current_.type == tt;
}

bool
Parser::peek_token_is(tokentypes tt)
{
  return peek_.type == tt;
}

bool
Parser::expect_peek(tokentypes tt)
{
  if(peek_token_is(tt)) {
    next_token();
    return true;
  }

  peek_error(tt);
  return false;
}

static std::unique_ptr<Statement> parse_statement();

static std::unique_ptr<Expression>
parse_expression(Precedence prec)
{
  if(m_prefix_parse_fns.find(parser.current_.type)
     == m_prefix_parse_fns.end()) {
    return nullptr;
  }

  auto fn = m_prefix_parse_fns[parser.current_.type];
  auto left = fn();

  while(!parser.peek_token_is(tokentypes::Semicolon)
        && prec < parser.peek_precedence()) {
    auto infix = m_infix_parse_fns[parser.peek_.type];
    if(infix == nullptr)
      return left;
    parser.next_token();
    left = infix(std::move(left));
  }

  return left;
}

static std::unique_ptr<Statement>
parse_let_statement()
{
  auto letstmt = std::make_unique<LetStatement>();

  if(!parser.expect_peek(tokentypes::Ident)) {
    return nullptr;
  }

  auto ident = std::make_unique<Identifier>();
  ident->value_ = parser.current_.literal;
  letstmt->name_ = std::move(ident);

  if(!parser.expect_peek(tokentypes::Assign)) {
    return nullptr;
  }

  parser.next_token();

  letstmt->value_ = parse_expression(LOWEST);

  if(parser.peek_token_is(tokentypes::Semicolon))
    parser.next_token();

  return letstmt;
}

std::unique_ptr<Statement>
parse_return_statement()
{
  auto returnstmt = std::make_unique<ReturnStatement>();

  parser.next_token();
  returnstmt->return_value_ = parse_expression(LOWEST);

  if(parser.peek_token_is(tokentypes::Semicolon))
    parser.next_token();

  return returnstmt;
}

std::unique_ptr<Statement>
parse_expression_statement()
{
  auto stmt = std::make_unique<ExpressionStatement>();

  stmt->expression_ = parse_expression(LOWEST);
  if(parser.peek_token_is(tokentypes::Semicolon)) {
    parser.next_token();
  }

  return stmt;
}

static std::unique_ptr<Expression>
parse_identifier()
{
  auto identifier = std::make_unique<Identifier>();
  identifier->value_ = parser.current_.literal;

  return identifier;
}

static std::unique_ptr<Expression>
parse_integer_literal()
{
  auto lit = std::make_unique<IntegerLiteral>();

  try {
    int res = std::stoi(parser.current_.literal);
    lit->value_ = res;
  } catch(std::invalid_argument e) {
    parser.errors_.push_back("could not parse integer");
    return nullptr;
  }

  return lit;
}

static std::unique_ptr<Expression>
parse_prefix_expression()
{
  auto exp = std::make_unique<PrefixExpression>();
  exp->opr = parser.current_.type;

  parser.next_token();
  exp->right_ = parse_expression(PREFIX);

  return exp;
}

Precedence
Parser::peek_precedence()
{
  if(precedences.find(peek_.type) != precedences.end())
    return precedences.at(peek_.type);
  return LOWEST;
}

Precedence
Parser::current_precedence()
{
  if(precedences.find(current_.type) != precedences.end())
    return precedences.at(current_.type);
  return LOWEST;
}

static std::unique_ptr<Expression>
parse_infix_expression(std::unique_ptr<Expression> left)
{
  auto exp = std::make_unique<InfixExpression>();
  exp->opr = parser.current_.type;
  exp->left_ = std::move(left);

  auto prec = parser.current_precedence();
  parser.next_token();
  exp->right_ = std::move(parse_expression(prec));

  return exp;
}

static std::unique_ptr<Expression>
parse_grouped_expression()
{
  parser.next_token();
  auto exp = parse_expression(LOWEST);
  if(!parser.expect_peek(tokentypes::RParen))
    return nullptr;

  return exp;
}

static std::unique_ptr<Expression>
parse_boolean()
{
  auto exp = std::make_unique<BooleanExpression>();
  exp->value_ = parser.current_token_is(tokentypes::True);

  return exp;
}

static std::unique_ptr<BlockStatement>
parse_block_statement()
{
  auto block = std::make_unique<BlockStatement>();
  block->token = parser.current_;
  block->statements_ = std::vector<std::unique_ptr<Statement> >();

  parser.next_token();

  while(!parser.current_token_is(tokentypes::RBrace)
        && !parser.current_token_is(tokentypes::Eof)) {
    auto stmt = parse_statement();
    if(stmt != nullptr) {
      block->statements_.push_back(std::move(stmt));
    }
    parser.next_token();
  }

  return block;
}

static std::unique_ptr<Expression>
parse_if_expression()
{
  auto exp = std::make_unique<IfExpression>();

  if(!parser.expect_peek(tokentypes::LParen))
    return nullptr;

  parser.next_token();
  exp->cond_ = parse_expression(LOWEST);

  if(!parser.expect_peek(tokentypes::RParen))
    return nullptr;

  if(!parser.expect_peek(tokentypes::LBrace))
    return nullptr;

  exp->after_ = parse_block_statement();

  if(parser.peek_token_is(tokentypes::Else)) {
    parser.next_token();

    if(!parser.expect_peek(tokentypes::LBrace)) {
      return nullptr;
    }

    exp->other_ = parse_block_statement();
  }

  return exp;
}

static std::vector<std::unique_ptr<Identifier> >
parse_function_params()
{
  std::vector<std::unique_ptr<Identifier> > params;
  if(parser.peek_token_is(tokentypes::LParen)) {
    parser.next_token();
    return params;
  }

  parser.next_token();

  auto ident = std::make_unique<Identifier>();
  ident->value_ = parser.current_.literal;
  params.push_back(std::move(ident));

  while(parser.peek_token_is(tokentypes::Comma)) {
    parser.next_token();
    parser.next_token();

    auto ident = std::make_unique<Identifier>();
    ident->value_ = parser.current_.literal;
    params.push_back(std::move(ident));
  }

  if(!parser.expect_peek(tokentypes::RParen))
    return std::vector<std::unique_ptr<Identifier> >();

  return params;
}

static std::unique_ptr<Expression>
parse_function_literal()
{
  auto lit = std::make_unique<FunctionLiteral>();

  if(!parser.expect_peek(tokentypes::LParen))
    return nullptr;

  lit->params_ = parse_function_params();

  if(!parser.expect_peek(tokentypes::LBrace))
    return nullptr;

  lit->body_ = std::move(parse_block_statement());

  return lit;
}

static std::vector<std::unique_ptr<Expression> >
parse_expression_list(tokentypes end)
{
  std::vector<std::unique_ptr<Expression> > expressions;
  if(parser.peek_token_is(end)) {
    parser.next_token();
    return expressions;
  }

  parser.next_token();
  expressions.push_back(std::move(parse_expression(LOWEST)));
  while(parser.peek_token_is(tokentypes::Comma)) {
    parser.next_token();
    parser.next_token();

    expressions.push_back(std::move(parse_expression(LOWEST)));
  }

  if(!parser.expect_peek(end))
    return std::vector<std::unique_ptr<Expression> >();

  return expressions;
}

static std::unique_ptr<Expression>
parse_call_expression(std::unique_ptr<Expression> func)
{
  auto exp = std::make_unique<CallExpression>();
  exp->func_ = std::move(func);
  exp->arguments_ = parse_expression_list(tokentypes::RParen);

  return exp;
}

static std::unique_ptr<Expression>
parse_string_literal()
{
  auto strlit = std::make_unique<StringLiteral>();
  strlit->value_ = parser.current_.literal;

  return strlit;
}

static std::unique_ptr<Expression>
parse_array_literal()
{
  auto arr = std::make_unique<ArrayLiteral>();
  arr->elements_ = std::move(parse_expression_list(tokentypes::RBracket));

  return arr;
}

static std::vector<std::unique_ptr<Expression> >
parse_call_arguments()
{
  std::vector<std::unique_ptr<Expression> > args;

  if(parser.peek_token_is(tokentypes::RParen)) {
    parser.next_token();
    return args;
  }

  parser.next_token();
  args.push_back(std::move(parse_expression(LOWEST)));

  while(parser.peek_token_is(tokentypes::Comma)) {
    parser.next_token();
    parser.next_token();

    args.push_back(std::move(parse_expression(LOWEST)));
  }

  if(!parser.expect_peek(tokentypes::RParen))
    return std::vector<std::unique_ptr<Expression> >();

  return args;
}

static std::unique_ptr<Expression>
parse_index_expression(std::unique_ptr<Expression> left)
{
  auto exp = std::make_unique<IndexExpression>();
  exp->left_ = std::move(left);

  parser.next_token();
  exp->index_ = parse_expression(LOWEST);
  if(!parser.expect_peek(tokentypes::RBracket))
    return nullptr;

  return exp;
}

static std::unique_ptr<Statement>
parse_statement()
{
  if(parser.current_.type == tokentypes::Let) {
    return parse_let_statement();
  } else if(parser.current_.type == tokentypes::Return) {
    return parse_return_statement();
  } else {
    return parse_expression_statement();
  }
}

static std::unique_ptr<Program>
parse_program()
{
  auto program = std::make_unique<Program>();
  program->statements_ = std::vector<std::unique_ptr<Statement> >();

  while(parser.current_.type != tokentypes::Eof) {
    auto stmt = parse_statement();
    if(stmt != nullptr) {
      program->statements_.push_back(std::move(stmt));
    }
    parser.next_token();
  }

  return program;
}

Parser::Parser(std::unique_ptr<Lexer> lx)
{
  lx_ = std::move(lx);

  m_prefix_parse_fns = std::unordered_map<tokentypes, PrefixParseFn>();

  add_prefix_parse(tokentypes::Ident, parse_identifier);
  add_prefix_parse(tokentypes::Int, parse_integer_literal);
  add_prefix_parse(tokentypes::Bang, parse_prefix_expression);
  add_prefix_parse(tokentypes::Minus, parse_prefix_expression);
  add_prefix_parse(tokentypes::True, parse_boolean);
  add_prefix_parse(tokentypes::False, parse_boolean);
  add_prefix_parse(tokentypes::LParen, parse_grouped_expression);
  add_prefix_parse(tokentypes::If, parse_if_expression);
  add_prefix_parse(tokentypes::Function, parse_function_literal);
  add_prefix_parse(tokentypes::String, parse_string_literal);
  add_prefix_parse(tokentypes::LBracket, parse_array_literal);

  m_infix_parse_fns = std::unordered_map<tokentypes, InfixParseFn>();
  add_infix_parse(tokentypes::Plus, parse_infix_expression);
  add_infix_parse(tokentypes::Minus, parse_infix_expression);
  add_infix_parse(tokentypes::Slash, parse_infix_expression);
  add_infix_parse(tokentypes::Asterisk, parse_infix_expression);
  add_infix_parse(tokentypes::Eq, parse_infix_expression);
  add_infix_parse(tokentypes::Neq, parse_infix_expression);
  add_infix_parse(tokentypes::LT, parse_infix_expression);
  add_infix_parse(tokentypes::GT, parse_infix_expression);
  add_infix_parse(tokentypes::LParen, parse_call_expression);
  add_infix_parse(tokentypes::LBracket, parse_index_expression);

  next_token();
  next_token();
}

std::vector<std::string>
Parser::errors() const
{
  return errors_;
}

void
Parser::peek_error(tokentypes tt)
{
  errors_.push_back("wrong token");
}

void
Parser::add_prefix_parse(tokentypes tt, PrefixParseFn fn)
{
  m_prefix_parse_fns[tt] = fn;
}

void
Parser::add_infix_parse(tokentypes tt, InfixParseFn fn)
{
  m_infix_parse_fns[tt] = fn;
}

std::unique_ptr<Program>
parse_source(std::string source)
{
  auto lexer = Lexer(source);
  auto parser = Parser(std::make_unique<Lexer>(lexer));

  return parse_program();
}
