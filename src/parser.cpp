#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <stack>
#include <string>

#include "ast.h"
#include "compiler.h"
#include "parser.h"
#include "token.h"

#define STOP_EXECUTION(message, ...)                                          \
  do {                                                                        \
    std::fprintf(stderr, message, ##__VA_ARGS__);                             \
    std::exit(1);                                                             \
  } while(false);

static std::stack<std::string> latest_function_identifers;

std::unique_ptr<Program>
Parser::parse_program()
{
  auto program = std::make_unique<Program>();
  program->statements_ = std::vector<std::unique_ptr<Statement> >();

  while(current_.type != tokentypes::Eof) {
    auto stmt = parse_statement();
    if(stmt != nullptr) {
      program->statements_.push_back(std::move(stmt));
    } else {
      STOP_EXECUTION("error parsing statement\n");
    }
    next_token();
  }

  return program;
}

Parser::Parser(std::unique_ptr<Lexer> lx)
{
  lx_ = std::move(lx);

  m_prefix_parse_fns = std::unordered_map<tokentypes, PrefixParseFn>();

  add_prefix_parse(tokentypes::Ident, &Parser::parse_identifier);
  add_prefix_parse(tokentypes::Int, &Parser::parse_integer_literal);
  add_prefix_parse(tokentypes::Bang, &Parser::parse_prefix_expression);
  add_prefix_parse(tokentypes::Minus, &Parser::parse_prefix_expression);
  add_prefix_parse(tokentypes::Asterisk, &Parser::parse_prefix_expression);
  add_prefix_parse(tokentypes::Amper, &Parser::parse_prefix_expression);
  add_prefix_parse(tokentypes::True, &Parser::parse_boolean);
  add_prefix_parse(tokentypes::False, &Parser::parse_boolean);
  add_prefix_parse(tokentypes::LParen, &Parser::parse_grouped_expression);
  add_prefix_parse(tokentypes::If, &Parser::parse_if_expression);
  add_prefix_parse(tokentypes::While, &Parser::parse_while_expression);
  add_prefix_parse(tokentypes::String, &Parser::parse_string_literal);
  add_prefix_parse(tokentypes::LBracket, &Parser::parse_array_literal);
  add_prefix_parse(tokentypes::Print, &Parser::parse_print_statement);
  add_prefix_parse(tokentypes::For, &Parser::parse_for_expression);

  m_infix_parse_fns = std::unordered_map<tokentypes, InfixParseFn>();
  add_infix_parse(tokentypes::Plus, &Parser::parse_infix_expression);
  add_infix_parse(tokentypes::Minus, &Parser::parse_infix_expression);
  add_infix_parse(tokentypes::Slash, &Parser::parse_infix_expression);
  add_infix_parse(tokentypes::Asterisk, &Parser::parse_infix_expression);
  add_infix_parse(tokentypes::Eq, &Parser::parse_infix_expression);
  add_infix_parse(tokentypes::Neq, &Parser::parse_infix_expression);
  add_infix_parse(tokentypes::LT, &Parser::parse_infix_expression);
  add_infix_parse(tokentypes::GT, &Parser::parse_infix_expression);
  add_infix_parse(tokentypes::LParen, &Parser::parse_call_expression);
  add_infix_parse(tokentypes::LBracket, &Parser::parse_index_expression);

  next_token();
  next_token();
}

void
Parser::next_token()
{
  current_ = peek_;
  peek_ = lx_->next_token();
}

std::unique_ptr<Statement>
Parser::parse_statement()
{
  if(current_.type == tokentypes::Let) {
    return parse_let_statement();
  }
  if(current_.type == tokentypes::Function) {
    return parse_function_literal();
  } else if(current_.type == tokentypes::Return) {
    return parse_return_statement();
  } else if(current_.type == tokentypes::Ident) {
    return parse_assingment();
  } else if(current_.type == tokentypes::Global) {
    return parse_global_decl();
  } else {
    return parse_expression_statement();
  }
}

static valuetype
convert_type_to_pointer(const valuetype type)
{
  switch(type) {
  case TYPE_VOID: {
    return TYPE_PTR_VOID;
  }
  case TYPE_INT: {
    return TYPE_PTR_INT;
  }
  case TYPE_LONG: {
    return TYPE_PTR_LONG;
  }
  default:
    STOP_EXECUTION("cannot convert type: %d into pointer\n", type);
  }
}

valuetype
Parser::parse_type()
{
  valuetype type;

  switch(peek_.type) {
  case tokentypes::IntType: {
    type = TYPE_INT;
    break;
  }
  case tokentypes::Void: {
    type = TYPE_VOID;
    break;
  }
  default:
    STOP_EXECUTION("cannot parser type for token: %d \n", peek_.type);
  }

  while(true) {
    next_token();
    if(!peek_token_is(tokentypes::Asterisk)) {
      break;
    }

    type = convert_type_to_pointer(type);
  }

  return type;
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

  return false;
}

static valuetype
v_from_token(const tokentypes type)
{
  switch(type) {
  case tokentypes::Void:
    return TYPE_VOID;
  case tokentypes::IntType:
    return TYPE_INT;
  case tokentypes::CharType:
    return TYPE_CHAR;
  default:
    STOP_EXECUTION("cannot convert token type into value.\n");
  }
}

std::unique_ptr<Statement>
Parser::parse_let_statement()
{
  auto letstmt = std::make_unique<LetStatement>();

  if(!expect_peek(tokentypes::Ident)) {
    return nullptr;
  }

  auto ident = std::make_unique<Identifier>();
  const std::string name = current_.literal;
  ident->value_ = name;

  if(!expect_peek(tokentypes::Colon)) {
    return nullptr;
  }

  letstmt->v_type = parse_type();

  if(!expect_peek(tokentypes::Assign)) {
    return nullptr;
  }
  next_token();

  if(!symbol_exists(ident->value_)) {
    add_new_symbol(ident->value_, TYPE_VARIABLE, letstmt->v_type);
  }

  letstmt->name_ = std::move(ident);

  auto exp = parse_expression_rec(LOWEST);
  // check if the types are applicable.
  auto res = change_type(std::move(exp), letstmt->v_type, tokentypes::Eof);
  if(res.second == nullptr) {
    letstmt->value_ = std::move(res.first);
    //   std::fprintf(stderr, "types are not applicable in let statement.");
    //   std::exit(1);
    // }
  } else {
    letstmt->value_ = std::move(res.second);
  }

  return letstmt;
}

std::unique_ptr<Statement>
Parser::parse_assingment()
{
  auto assign = std::make_unique<AssignmentStatement>();

  auto identifier = std::make_unique<Identifier>();
  identifier->value_ = current_.literal;

  const auto &sym = get_symbol(current_.literal);
  assign->assingment_type_ = sym.value_type_;

  next_token();

  if(!expect_peek(tokentypes::Assign)) {
    return nullptr;
  }

  auto exp = parse_expression(LOWEST);
  if(!check_type_compatible(assign->assingment_type_, exp.second, false))
    STOP_EXECUTION("assignment types are wrong\n");

  if(peek_token_is(tokentypes::Semicolon))
    next_token();

  return assign;
}

std::unique_ptr<Statement>
Parser::parse_return_statement()
{
  auto returnstmt = std::make_unique<ReturnStatement>();
  const std::string top = latest_function_identifers.top();
  returnstmt->function_identifier_ = top;
  valuetype function_return_type_ = get_symbol(top).value_type_;

  next_token();

  returnstmt->types_ = function_return_type_;

  auto value = parse_expression_rec(LOWEST);
  if(!check_type_compatible(function_return_type_, value->ValueType(), false))
    STOP_EXECUTION("return value doesn't match functions return type\n");
  returnstmt->return_value_ = std::move(value);

  return returnstmt;
}

std::unique_ptr<Expression>
Parser::parse_print_statement()
{
  auto print_stmt = std::make_unique<PrintStatement>();
  next_token();
  print_stmt->print_value_ = parse_expression_rec(LOWEST);

  return print_stmt;
}

std::unique_ptr<Statement>
Parser::parse_expression_statement()
{
  auto stmt = std::make_unique<ExpressionStatement>();
  stmt->expression_ = parse_expression_rec(LOWEST);

  return stmt;
}

std::unique_ptr<Expression>
Parser::parse_primary()
{
  std::unique_ptr<Expression> result = nullptr;
  switch(current_.type) {
  case tokentypes::Int: {
    result = parse_integer_literal();
    break;
  }
  case tokentypes::Ident: {
    auto identifier = parse_identifier();

    if(peek_token_is(tokentypes::LParen)) {
      next_token();
      result = parse_call_expression(std::move(identifier));
      break;
    }

    const auto &ident = static_cast<const Identifier &>(*identifier);
    if(!symbol_exists(ident.value_))
      STOP_EXECUTION("cannot find variable: '%s'\n", ident.value_.c_str());

    result = std::move(identifier);
    break;
  }
  default:
    STOP_EXECUTION(
        "unrecognized token when parsing primary expression factor. '%s",
        current_.literal.c_str());
  }

  next_token();
  return result;
}

std::unique_ptr<Expression>
Parser::parse_prefix()
{
  switch(current_.type) {
  case tokentypes::Amper: {
    auto tokentype = current_.type;
    next_token();

    auto right = parse_prefix();
    if(right->Type() != AstType::Identifier)
      STOP_EXECUTION(
          "ampersand cannot be used for nothing but identifiers.\n");

    auto prefix_exp = std::make_unique<PrefixExpression>();
    prefix_exp->opr = tokentype;
    prefix_exp->right_ = std::move(right);

    return prefix_exp;
  }
  case tokentypes::Asterisk: {
    next_token();

    auto right = parse_prefix();
    if(right->Type() != AstType::Identifier)
      STOP_EXECUTION(
          "ampersand cannot be used for nothing but identifiers.\n");

    auto deref_exp = std::make_unique<Dereference>();
    deref_exp->to_dereference_ = std::move(right);

    return deref_exp;
  }
  case tokentypes::Bang:
  case tokentypes::Minus: {
    return parse_prefix_expression();
  }
  case tokentypes::Print: {
    auto print_stmt = std::make_unique<PrintStatement>();
    next_token();

    print_stmt->print_value_ = parse_expression_rec(LOWEST);

    return print_stmt;
  }
  default:
    return parse_primary();
  }
}

std::unique_ptr<Expression>
Parser::parse_expression_rec(Precedence prec)
{
  auto left = parse_prefix();

  if(current_token_is(tokentypes::Semicolon)
     || current_token_is(tokentypes::RParen)) {
    return left;
  }

  auto tokentype = current_.type;

  while(
      (prec < precedences[tokentype])
      || (tokentype == tokentypes::Assign && precedences[tokentype] == prec)) {
    next_token();

    auto right = parse_expression_rec(precedences[tokentype]);
    auto left_type = left->ValueType();

    if(tokentype == tokentypes::Assign) {
      right->set_rvalue(true);

      auto right_temp = change_type(std::move(right), left_type, tokentype);
      if(right_temp.second == nullptr) {
        STOP_EXECUTION("incompatible type in assingment");
      }

      auto infix = std::make_unique<InfixExpression>();
      infix->left_ = std::move(right_temp.second);
      infix->right_ = std::move(left);

      infix->v_type_ = infix->left_->ValueType();
      infix->opr = tokentype;

      left = std::move(infix);
    } else {
      left->set_rvalue(true);
      right->set_rvalue(true);

      auto left_temp
          = change_type(std::move(left), right->ValueType(), tokentype);
      auto right_temp = change_type(std::move(right), left_type, tokentype);

      if(left_temp.second == nullptr && right_temp.second == nullptr) {
        STOP_EXECUTION("bad types in expression\n");
      }

      auto infix = std::make_unique<InfixExpression>();
      if(left_temp.second != nullptr) {
        infix->left_ = std::move(left_temp.second);
      } else {
        infix->left_ = std::move(left_temp.first);
      }

      if(right_temp.second != nullptr) {
        infix->right_ = std::move(right_temp.second);
      } else {
        infix->right_ = std::move(right_temp.first);
      }

      infix->v_type_ = infix->left_->ValueType();
      infix->opr = tokentype;

      left = std::move(infix);
    }

    tokentype = current_.type;
    if(current_token_is(tokentypes::Semicolon)
       || current_token_is(tokentypes::RParen)) {
      left->set_rvalue(true);
      return left;
    }
  }

  left->set_rvalue(true);
  return left;
}

std::pair<std::unique_ptr<Expression>, valuetype>
Parser::parse_expression(Precedence prec)
{
  if(m_prefix_parse_fns.find(current_.type) == m_prefix_parse_fns.end()) {
    return { nullptr, TYPE_VOID };
  }

  auto fn = m_prefix_parse_fns[current_.type];
  auto left = (this->*fn)();
  valuetype type = left->ValueType();

  while(!peek_token_is(tokentypes::Semicolon) && prec < peek_precedence()) {
    auto infix = m_infix_parse_fns[peek_.type];
    if(infix == nullptr) {
      return { std::move(left), left->ValueType() };
    }

    next_token();
    left = (this->*infix)(std::move(left));

    // we cannot cast infix expression on these, so ignore them for now.
    if(left->Type() != AstType::CallExpression
       && left->Type() != AstType::InfixExpression) {
      const auto &inf = static_cast<const InfixExpression &>(*left);
      if(!check_type_compatible(inf.left_->ValueType(),
                                inf.right_->ValueType(), true))
        STOP_EXECUTION("types are not equal in infix expression.\n");

      type = inf.left_->ValueType();
    } else if(left->Type() == AstType::CallExpression) {
      const auto &cexp = static_cast<const CallExpression &>(*left);
      switch(cexp.func_->Type()) {
      case AstType::Identifier: {
        const auto &ident = static_cast<const Identifier &>(*cexp.func_);
        const auto &sym = get_symbol(ident.value_);
        type = sym.value_type_;

        break;
      }
      case AstType::FunctionLiteral: {
        // TODO: no
        const auto &fn_lit = static_cast<const CallExpression &>(*left);
        break;
      }
      default:
        STOP_EXECUTION("cannot call non-function");
      }
    }
  }

  return { std::move(left), type };
}

std::unique_ptr<Expression>
Parser::parse_identifier()
{
  auto identifier = std::make_unique<Identifier>();
  identifier->value_ = current_.literal;

  const auto &sym = get_symbol(current_.literal);
  identifier->value_type = sym.value_type_;

  return identifier;
}

std::unique_ptr<Expression>
Parser::parse_integer_literal()
{
  auto lit = std::make_unique<IntegerLiteral>();

  try {
    int res = std::stoi(current_.literal);
    lit->value_ = res;
  } catch(const std::invalid_argument &e) {
    STOP_EXECUTION("cannot convert string into number.\n");
  }

  return lit;
}

std::unique_ptr<Expression>
Parser::parse_prefix_expression()
{
  auto exp = std::make_unique<PrefixExpression>();
  exp->opr = current_.type;

  next_token();
  exp->right_ = parse_expression(PREFIX).first;

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

std::unique_ptr<Expression>
Parser::parse_infix_expression(std::unique_ptr<Expression> left)
{
  auto exp = std::make_unique<InfixExpression>();
  exp->opr = current_.type;
  exp->left_ = std::move(left);

  auto prec = current_precedence();
  next_token();
  exp->right_ = std::move(parse_expression(prec).first);

  return exp;
}

std::unique_ptr<Expression>
Parser::parse_grouped_expression()
{
  next_token();
  auto exp = parse_expression(LOWEST);
  if(!expect_peek(tokentypes::RParen))
    return nullptr;

  return std::move(exp.first);
}

std::unique_ptr<Expression>
Parser::parse_boolean()
{
  auto exp = std::make_unique<BooleanExpression>();
  exp->value_ = current_token_is(tokentypes::True);

  return exp;
}

std::unique_ptr<Expression>
Parser::parse_while_expression()
{
  auto while_stmt = std::make_unique<WhileStatement>();
  if(!expect_peek(tokentypes::LParen))
    return nullptr;

  next_token();
  while_stmt->cond_ = parse_expression(LOWEST).first;

  if(!expect_peek(tokentypes::RParen))
    return nullptr;

  if(!expect_peek(tokentypes::LBrace))
    return nullptr;

  while_stmt->body_ = parse_block_statement();

  return while_stmt;
}

std::unique_ptr<Expression>
Parser::parse_if_expression()
{
  auto exp = std::make_unique<IfExpression>();

  if(!expect_peek(tokentypes::LParen))
    return nullptr;

  next_token();
  exp->cond_ = parse_expression(LOWEST).first;

  if(!expect_peek(tokentypes::RParen))
    return nullptr;

  if(!expect_peek(tokentypes::LBrace))
    return nullptr;

  exp->after_ = parse_block_statement();

  if(peek_token_is(tokentypes::Else)) {
    next_token();

    if(!expect_peek(tokentypes::LBrace)) {
      return nullptr;
    }

    exp->other_ = parse_block_statement();
  }

  return exp;
}

std::unique_ptr<BlockStatement>
Parser::parse_block_statement()
{
  auto block = std::make_unique<BlockStatement>();
  block->token = current_;
  block->statements_ = std::vector<std::unique_ptr<Statement> >();

  next_token();

  while(!current_token_is(tokentypes::RBrace)
        && !current_token_is(tokentypes::Eof)) {
    auto stmt = parse_statement();
    if(stmt != nullptr) {
      block->statements_.push_back(std::move(stmt));
    }
    next_token();
  }

  return block;
}

std::unique_ptr<Statement>
Parser::parse_function_literal()
{
  auto lit = std::make_unique<FunctionLiteral>();

  if(!expect_peek(tokentypes::Ident)) {
    return nullptr;
  }

  auto ident = std::make_unique<Identifier>();
  const std::string name = current_.literal;
  ident->value_ = name;
  lit->name_ = std::move(ident);

  if(!expect_peek(tokentypes::LParen))
    return nullptr;

  lit->params_ = parse_function_params();

  if(!expect_peek(tokentypes::Arrow))
    return nullptr;

  if(peek_token_is(tokentypes::Void) || peek_token_is(tokentypes::IntType)
     || peek_token_is(tokentypes::CharType)) {
    lit->return_type_ = v_from_token(peek_.type);
    next_token();
  } else {
    return nullptr;
  }
  add_new_symbol(name, TYPE_FUNCTION, lit->return_type_, get_next_label());

  latest_function_identifers.push(name);

  if(!expect_peek(tokentypes::LBrace))
    return nullptr;

  lit->body_ = std::move(parse_block_statement());

  latest_function_identifers.pop();

  return lit;
}

std::vector<std::unique_ptr<Identifier> >
Parser::parse_function_params()
{
  std::vector<std::unique_ptr<Identifier> > params;
  if(peek_token_is(tokentypes::LParen)) {
    next_token();
    return params;
  }

  next_token();

  auto ident = std::make_unique<Identifier>();
  ident->value_ = current_.literal;
  params.push_back(std::move(ident));

  while(peek_token_is(tokentypes::Comma)) {
    next_token();
    next_token();

    auto ident = std::make_unique<Identifier>();
    ident->value_ = current_.literal;
    params.push_back(std::move(ident));
  }

  if(!expect_peek(tokentypes::RParen))
    return std::vector<std::unique_ptr<Identifier> >();

  return params;
}

std::unique_ptr<Expression>
Parser::parse_call_expression(std::unique_ptr<Expression> func)
{
  auto exp = std::make_unique<CallExpression>();
  exp->func_ = std::move(func);
  exp->arguments_ = std::move(parse_expression_list(tokentypes::RParen));

  return exp;
}

std::unique_ptr<Expression>
Parser::parse_string_literal()
{
  auto strlit = std::make_unique<StringLiteral>();
  strlit->value_ = current_.literal;

  return strlit;
}

std::unique_ptr<Expression>
Parser::parse_array_literal()
{
  auto arr = std::make_unique<ArrayLiteral>();
  arr->elements_ = std::move(parse_expression_list(tokentypes::RBracket));

  return arr;
}

std::unique_ptr<Expression>
Parser::parse_for_expression()
{
  auto for_stmt = std::make_unique<ForStatement>();

  if(!expect_peek(tokentypes::LParen))
    return nullptr;

  for_stmt->assignment_ = parse_let_statement();

  if(!expect_peek(tokentypes::Semicolon))
    return nullptr;

  for_stmt->cond_ = parse_expression(LOWEST).first;

  if(!expect_peek(tokentypes::Semicolon))
    return nullptr;

  for_stmt->after_every_ = parse_let_statement();

  if(!expect_peek(tokentypes::RParen))
    return nullptr;

  if(!expect_peek(tokentypes::LBrace))
    return nullptr;

  for_stmt->body_ = parse_block_statement();

  return for_stmt;
}

std::vector<std::unique_ptr<Expression> >
Parser::parse_expression_list(tokentypes end)
{
  std::vector<std::unique_ptr<Expression> > expressions;
  if(peek_token_is(end)) {
    next_token();
    return expressions;
  }

  next_token();
  expressions.push_back(std::move(parse_expression(LOWEST).first));
  while(peek_token_is(tokentypes::Comma)) {
    next_token();
    next_token();

    expressions.push_back(std::move(parse_expression(LOWEST).first));
  }

  if(!expect_peek(end))
    return std::vector<std::unique_ptr<Expression> >();

  return expressions;
}

std::vector<std::unique_ptr<Expression> >
Parser::parse_call_arguments()
{
  std::vector<std::unique_ptr<Expression> > args;

  if(peek_token_is(tokentypes::RParen)) {
    next_token();
    return args;
  }

  next_token();
  args.push_back(std::move(parse_expression(LOWEST).first));

  while(peek_token_is(tokentypes::Comma)) {
    next_token();
    next_token();

    args.push_back(std::move(parse_expression(LOWEST).first));
  }

  if(!expect_peek(tokentypes::RParen))
    return std::vector<std::unique_ptr<Expression> >();

  return args;
}

std::unique_ptr<Expression>
Parser::parse_index_expression(std::unique_ptr<Expression> left)
{
  auto exp = std::make_unique<IndexExpression>();
  exp->left_ = std::move(left);

  next_token();
  exp->index_ = parse_expression(LOWEST).first;
  if(!expect_peek(tokentypes::RBracket))
    return nullptr;

  return exp;
}

std::unique_ptr<Statement>
Parser::parse_global_decl()
{
  // cannot use global keyword inside function
  if(!latest_function_identifers.empty()) {
    return nullptr;
  }

  auto globl = std::make_unique<GlobalVariable>();
  globl->type_ = parse_type();

  if(!expect_peek(tokentypes::Ident))
    return nullptr;

  auto ident = std::make_unique<Identifier>();
  const std::string name = current_.literal;
  ident->value_ = name;
  globl->identifier_ = std::move(ident);

  // we don't want multiple global defintions
  if(symbol_exists(name)) {
    return nullptr;
  }

  add_new_symbol(name, TYPE_VARIABLE, globl->type_);

  if(!expect_peek(tokentypes::Semicolon))
    return nullptr;

  return globl;
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
