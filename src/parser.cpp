#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <stack>
#include <string>

#include "ast.h"
#include "codegen_x64.h"
#include "compiler.h"
#include "parser.h"
#include "token.h"

#define STOP_EXECUTION(message, ...)                                           \
  do {                                                                         \
    std::fprintf(stderr, message, ##__VA_ARGS__);                              \
    std::exit(1);                                                              \
  } while (false);

static std::stack<std::string> latest_function_identifers;

std::unique_ptr<Program> Parser::parse_program() {
  add_new_symbol("print_num", TYPE_FUNCTION, TYPE_CHAR);

  auto program = std::make_unique<Program>();
  program->statements_ = std::vector<std::unique_ptr<Statement>>();

  while (current_.type != tokentypes::Eof) {
    auto stmt = parse_statement();
    if (stmt != nullptr) {
      program->statements_.push_back(std::move(stmt));
    } else {
      STOP_EXECUTION("error parsing statement\n");
    }
    next_token();
  }

  return program;
}

Parser::Parser(std::unique_ptr<Lexer> lx) {
  lx_ = std::move(lx);

  next_token();
  next_token();
}

void Parser::next_token() {
  current_ = peek_;
  peek_ = lx_->next_token();
}

std::unique_ptr<Statement> Parser::parse_statement() {
  if (current_.type == tokentypes::Let) {
    return parse_let_statement();
  }
  if (current_.type == tokentypes::Function) {
    return parse_function_literal();
  } else if (current_.type == tokentypes::Return) {
    return parse_return_statement();
  } else if (current_.type == tokentypes::Global) {
    return parse_global_decl();
  } else if (current_.type == tokentypes::Var) {
    return parse_var_decl();
  } else {
    return parse_expression_statement();
  }
}

static valuetype convert_type_to_pointer(const valuetype type) {
  switch (type) {
  case TYPE_VOID: {
    return TYPE_PTR_VOID;
  }
  case TYPE_CHAR: {
    return TYPE_PTR_CHAR;
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

valuetype Parser::parse_type() {
  valuetype type;

  switch (peek_.type) {
  case tokentypes::IntType: {
    type = TYPE_INT;
    break;
  }
  case tokentypes::Void: {
    type = TYPE_VOID;
    break;
  }
  case tokentypes::CharType: {
    type = TYPE_CHAR;
    break;
  }
  default:
    STOP_EXECUTION("cannot parser type for token: %d \n", peek_.type);
  }

  while (true) {
    next_token();
    if (!peek_token_is(tokentypes::Asterisk)) {
      break;
    }

    type = convert_type_to_pointer(type);
  }

  return type;
}

bool Parser::current_token_is(tokentypes tt) { return current_.type == tt; }

bool Parser::peek_token_is(tokentypes tt) { return peek_.type == tt; }

bool Parser::expect_peek(tokentypes tt) {
  if (peek_token_is(tt)) {
    next_token();
    return true;
  }

  return false;
}

static valuetype v_from_token(const tokentypes type) {
  switch (type) {
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

std::unique_ptr<Expression>
Parser::parse_call(std::unique_ptr<Expression> ident) {
  next_token();
  next_token();

  auto argument = parse_expression_rec(LOWEST);
  if (!current_token_is(tokentypes::RParen))
    STOP_EXECUTION(
        "function call arguments needs to be wrapped in parenthesies.");

  auto call_exp = std::make_unique<CallExpression>();

  call_exp->func_ = std::move(ident);

  std::vector<std::unique_ptr<Expression>> arguments_;
  arguments_.push_back(std::move(argument));

  return call_exp;
}

std::unique_ptr<Expression>
Parser::parse_array(std::unique_ptr<Expression> ident) {

  next_token();
  next_token();
  auto arr = std::make_unique<Addr>();
  arr->to_addr_ = std::move(ident);

  auto indx = parse_expression_rec(LOWEST);
  if (!current_token_is(tokentypes::RBracket))
    STOP_EXECUTION("array index operation needs to end in a ]\n");

  if (indx->ValueType() != TYPE_INT && indx->ValueType() != TYPE_CHAR &&
      indx->ValueType() != TYPE_LONG)
    STOP_EXECUTION("the index expression needs to be an integer.");

  auto index_modified =
      change_type(std::move(indx), arr->ValueType(), tokentypes::Plus);

  auto infix = std::make_unique<InfixExpression>();
  infix->opr = tokentypes::Plus;
  infix->v_type_ = arr->ValueType();
  infix->left_ = std::move(arr);
  if (index_modified.second == nullptr)
    STOP_EXECUTION("right type cannot be changed.");
  infix->right_ = std::move(index_modified.second);

  auto deref = std::make_unique<Dereference>();
  deref->to_dereference_ = std::move(infix);

  return deref;
}

std::unique_ptr<Expression> Parser::parse_postfix() {
  auto identifier = parse_identifier();

  if (peek_token_is(tokentypes::LParen)) {
    return parse_call(std::move(identifier));
  } else if (peek_token_is(tokentypes::LBracket)) {
    return parse_array(std::move(identifier));
  }

  next_token();

  switch (current_.type) {
  case tokentypes::Inc: {
    next_token();
    auto action = std::make_unique<IdentifierAction>();
    action->action_ = tokentypes::Inc;
    action->identifier_ = std::move(identifier);

    return action;
  }
  case tokentypes::Dec: {
    next_token();
    auto action = std::make_unique<IdentifierAction>();
    action->action_ = tokentypes::Dec;
    action->identifier_ = std::move(identifier);

    return action;
  }
  default:
    return identifier;
  }
}

std::unique_ptr<Statement> Parser::parse_let_statement() {
  auto letstmt = std::make_unique<LetStatement>();

  if (!expect_peek(tokentypes::Ident)) {
    return nullptr;
  }

  auto ident = std::make_unique<Identifier>();
  const std::string name = current_.literal;
  ident->value_ = name;

  if (!expect_peek(tokentypes::Colon)) {
    return nullptr;
  }

  letstmt->v_type = parse_type();

  if (!expect_peek(tokentypes::Assign)) {
    return nullptr;
  }
  next_token();

  if (!symbol_exists(ident->value_)) {
    add_new_symbol(ident->value_, TYPE_VARIABLE, letstmt->v_type);
  }

  letstmt->name_ = std::move(ident);

  auto exp = parse_expression_rec(LOWEST);
  // check if the types are applicable.
  auto res = change_type(std::move(exp), letstmt->v_type, tokentypes::Eof);
  if (res.second == nullptr) {
    letstmt->value_ = std::move(res.first);
    //   std::fprintf(stderr, "types are not applicable in let statement.");
    //   std::exit(1);
    // }
  } else {
    letstmt->value_ = std::move(res.second);
  }

  return letstmt;
}

std::unique_ptr<Statement> Parser::parse_return_statement() {
  auto returnstmt = std::make_unique<ReturnStatement>();
  const std::string top = latest_function_identifers.top();
  returnstmt->function_identifier_ = top;
  valuetype function_return_type_ = get_symbol(top).value_type_;

  next_token();

  returnstmt->types_ = function_return_type_;

  auto value = parse_expression_rec(LOWEST);
  if (!check_type_compatible(function_return_type_, value->ValueType(), false))
    STOP_EXECUTION("return value doesn't match functions return type\n");
  returnstmt->return_value_ = std::move(value);

  return returnstmt;
}

std::unique_ptr<Statement> Parser::parse_expression_statement() {
  auto stmt = std::make_unique<ExpressionStatement>();
  stmt->expression_ = parse_expression_rec(LOWEST);

  return stmt;
}

std::unique_ptr<Expression> Parser::parse_primary() {
  std::unique_ptr<Expression> result = nullptr;
  switch (current_.type) {
  case tokentypes::Int: {
    result = parse_integer_literal();
    break;
  }
  case tokentypes::String: {
    auto strlit = std::make_unique<StringLiteral>();
    auto value = current_.literal;

    int label = get_next_label();

    global_str(label, value);

    strlit->value_ = value;
    strlit->id_ = label;
    result = std::move(strlit);

    break;
  }
  case tokentypes::Ident: {
    auto identifier = parse_identifier();

    if (peek_token_is(tokentypes::LParen)) {
      result = parse_call(std::move(identifier));
      break;
    } else if (peek_token_is(tokentypes::LBracket)) {
      result = parse_array(std::move(identifier));
      break;
    } else if (peek_token_is(tokentypes::Inc)) {
      next_token();
      next_token();
      auto action = std::make_unique<IdentifierAction>();
      action->action_ = tokentypes::Inc;
      action->identifier_ = std::move(identifier);

      return action;
    } else if (peek_token_is(tokentypes::Dec)) {
      next_token();
      next_token();
      auto action = std::make_unique<IdentifierAction>();
      action->action_ = tokentypes::Inc;
      action->identifier_ = std::move(identifier);

      return action;
    }

    result = std::move(identifier);
    break;
  }
  case tokentypes::LParen: {
    next_token();

    auto expr = parse_expression_rec(LOWEST);
    if (!current_token_is(tokentypes::RParen))
      STOP_EXECUTION("expected right paren after");

    return expr;
  }
  default:
    STOP_EXECUTION(
        "unrecognized token when parsing primary expression factor. '%s",
        current_.literal.c_str());
  }

  next_token();
  return result;
}

std::unique_ptr<Expression> Parser::parse_prefix() {
  switch (current_.type) {
  case tokentypes::Amper: {
    next_token();

    auto right = parse_prefix();
    if (right->Type() != AstType::Identifier)
      STOP_EXECUTION("ampersand cannot be used for nothing but identifiers.\n");

    auto addr_exp = std::make_unique<Addr>();
    addr_exp->to_addr_ = std::move(right);

    return addr_exp;
  }
  case tokentypes::Asterisk: {
    next_token();

    auto right = parse_prefix();
    if (right->Type() != AstType::Identifier)
      STOP_EXECUTION("ampersand cannot be used for nothing but identifiers.\n");

    auto deref_exp = std::make_unique<Dereference>();
    deref_exp->to_dereference_ = std::move(right);

    return deref_exp;
  }
  case tokentypes::Dec:
  case tokentypes::Inc: {
    auto type = current_.type;
    next_token();

    auto right = parse_prefix();
    if (right->Type() != AstType::Identifier)
      STOP_EXECUTION("cannot increment/decrement a non-identifier.");

    auto identifier_action = std::make_unique<IdentifierAction>();
    identifier_action->identifier_ = std::move(right);
    identifier_action->post_ = false;
    identifier_action->action_ = type;

    return identifier_action;
  }
  case tokentypes::Bang:
  case tokentypes::Minus: {
    auto type = current_.type;
    next_token();

    auto right = parse_prefix();

    auto prefix = std::make_unique<PrefixExpression>();
    prefix->opr = type;
    prefix->right_ = std::move(right);

    return prefix;
  }
  case tokentypes::If: {
    return parse_if_expression();
  }
  case tokentypes::While: {
    auto while_stmt = std::make_unique<WhileStatement>();
    if (!expect_peek(tokentypes::LParen))
      return nullptr;

    next_token();
    while_stmt->cond_ = parse_expression_rec(LOWEST);

    if (!expect_peek(tokentypes::LBrace))
      return nullptr;

    while_stmt->body_ = parse_block_statement();

    return while_stmt;
  }
  default:
    return parse_primary();
  }
}

std::unique_ptr<Expression> Parser::parse_expression_rec(Precedence prec) {
  auto left = parse_prefix();

  if (current_token_is(tokentypes::Semicolon) ||
      current_token_is(tokentypes::RParen)) {
    left->set_rvalue(true);
    return left;
  }

  auto tokentype = current_.type;

  while ((prec < precedences[tokentype]) ||
         (tokentype == tokentypes::Assign && precedences[tokentype] == prec)) {
    next_token();

    auto right = parse_expression_rec(precedences[tokentype]);
    auto left_type = left->ValueType();

    if (tokentype == tokentypes::Assign) {
      right->set_rvalue(true);

      auto right_temp = change_type(std::move(right), left_type, tokentype);
      if (right_temp.second == nullptr) {
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

      auto left_temp =
          change_type(std::move(left), right->ValueType(), tokentype);
      auto right_temp = change_type(std::move(right), left_type, tokentype);

      if (left_temp.second == nullptr && right_temp.second == nullptr) {
        STOP_EXECUTION("bad types in expression\n");
      }

      auto infix = std::make_unique<InfixExpression>();
      if (left_temp.second != nullptr) {
        infix->left_ = std::move(left_temp.second);
      } else {
        infix->left_ = std::move(left_temp.first);
      }

      if (right_temp.second != nullptr) {
        infix->right_ = std::move(right_temp.second);
      } else {
        infix->right_ = std::move(right_temp.first);
      }

      infix->v_type_ = infix->left_->ValueType();
      infix->opr = tokentype;

      left = std::move(infix);
    }

    tokentype = current_.type;
    if (current_token_is(tokentypes::Semicolon) ||
        current_token_is(tokentypes::RParen) ||
        tokentype == tokentypes::RBracket) {
      left->set_rvalue(true);
      return left;
    }
  }

  left->set_rvalue(true);
  return left;
}

std::unique_ptr<Expression> Parser::parse_identifier() {
  auto identifier = std::make_unique<Identifier>();
  identifier->value_ = current_.literal;

  const auto &sym = get_symbol(current_.literal);
  identifier->value_type = sym.value_type_;

  return identifier;
}

std::unique_ptr<Expression> Parser::parse_integer_literal() {
  auto lit = std::make_unique<IntegerLiteral>();

  try {
    int res = std::stoi(current_.literal);
    lit->value_ = res;
  } catch (const std::invalid_argument &e) {
    STOP_EXECUTION("cannot convert string into number.\n");
  }

  return lit;
}

Precedence Parser::peek_precedence() {
  if (precedences.find(peek_.type) != precedences.end())
    return precedences.at(peek_.type);
  return LOWEST;
}

Precedence Parser::current_precedence() {
  if (precedences.find(current_.type) != precedences.end())
    return precedences.at(current_.type);
  return LOWEST;
}

std::unique_ptr<Expression> Parser::parse_if_expression() {
  auto exp = std::make_unique<IfExpression>();

  if (!expect_peek(tokentypes::LParen))
    return nullptr;

  next_token();
  exp->cond_ = parse_expression_rec(LOWEST);

  if (!expect_peek(tokentypes::LBrace))
    return nullptr;

  exp->after_ = parse_block_statement();

  if (peek_token_is(tokentypes::Else)) {
    next_token();

    if (!expect_peek(tokentypes::LBrace)) {
      return nullptr;
    }

    exp->other_ = parse_block_statement();
  }

  return exp;
}

std::unique_ptr<BlockStatement> Parser::parse_block_statement() {
  auto block = std::make_unique<BlockStatement>();
  block->token = current_;
  block->statements_ = std::vector<std::unique_ptr<Statement>>();

  next_token();

  while (!current_token_is(tokentypes::RBrace) &&
         !current_token_is(tokentypes::Eof)) {
    auto stmt = parse_statement();
    if (stmt != nullptr) {
      block->statements_.push_back(std::move(stmt));
    }
    next_token();
  }

  return block;
}

std::unique_ptr<Statement> Parser::parse_function_literal() {
  auto lit = std::make_unique<FunctionLiteral>();

  if (!expect_peek(tokentypes::Ident)) {
    return nullptr;
  }

  auto ident = std::make_unique<Identifier>();
  const std::string name = current_.literal;
  ident->value_ = name;
  lit->name_ = std::move(ident);

  // reset local variables, since we are inside a new function.
  reset_local_variables();

  if (!expect_peek(tokentypes::LParen))
    return nullptr;

  lit->params_ = parse_function_params();

  if (!expect_peek(tokentypes::Arrow))
    return nullptr;

  if (peek_token_is(tokentypes::Void) || peek_token_is(tokentypes::IntType) ||
      peek_token_is(tokentypes::CharType)) {
    lit->return_type_ = v_from_token(peek_.type);
    next_token();
  } else {
    return nullptr;
  }
  add_new_symbol(name, TYPE_FUNCTION, lit->return_type_, get_next_label());

  latest_function_identifers.push(name);

  if (!expect_peek(tokentypes::LBrace))
    return nullptr;

  lit->body_ = std::move(parse_block_statement());

  latest_function_identifers.pop();

  return lit;
}

std::vector<std::unique_ptr<Identifier>> Parser::parse_function_params() {
  std::vector<std::unique_ptr<Identifier>> params;
  if (peek_token_is(tokentypes::LParen)) {
    next_token();
    return params;
  }

  next_token();

  auto ident = std::make_unique<Identifier>();
  ident->value_ = current_.literal;

  if (!expect_peek(tokentypes::Colon)) {
    return {};
  }

  auto type = parse_type();
  next_token();
  params.push_back(std::move(ident));

  while (peek_token_is(tokentypes::Comma)) {
    next_token();
    next_token();

    auto ident = std::make_unique<Identifier>();
    ident->value_ = current_.literal;

    if (!expect_peek(tokentypes::Colon)) {
      return {};
    }

    auto type = parse_type();
    next_token();

    params.push_back(std::move(ident));
  }

  if (!expect_peek(tokentypes::RParen))
    return std::vector<std::unique_ptr<Identifier>>();

  return params;
}

std::unique_ptr<Statement> Parser::parse_var_decl() {
  // cannot use var keyword out of function
  if (latest_function_identifers.empty()) {
    return nullptr;
  }

  auto var_decl = std::make_unique<VarDecl>();
  var_decl->type_ = parse_type();

  if (!expect_peek(tokentypes::Ident))
    return nullptr;

  auto ident = std::make_unique<Identifier>();
  const std::string name = current_.literal;
  ident->value_ = name;
  var_decl->identifier_ = std::move(ident);

  add_new_local_var(name, var_decl->type_, 0, 1);

  if (peek_token_is(tokentypes::LBracket)) {
    next_token();
    if (!expect_peek(tokentypes::Int))
      STOP_EXECUTION("array declaration needs size.");

    int size = std::stoi(current_.literal);
    add_new_symbol(name, TYPE_ARRAY, convert_type_to_pointer(var_decl->type_),
                   0, size);

    if (!expect_peek(tokentypes::RBracket))
      STOP_EXECUTION("array declration must end in ]");
  } else {
    add_new_symbol(name, TYPE_VARIABLE, var_decl->type_, 0, 1);
  }

  if (!expect_peek(tokentypes::Semicolon))
    return nullptr;

  return var_decl;
}

std::unique_ptr<Statement> Parser::parse_global_decl() {
  // cannot use global keyword inside function
  if (!latest_function_identifers.empty()) {
    return nullptr;
  }

  auto globl = std::make_unique<GlobalVariable>();
  globl->type_ = parse_type();

  if (!expect_peek(tokentypes::Ident))
    return nullptr;

  auto ident = std::make_unique<Identifier>();
  const std::string name = current_.literal;
  ident->value_ = name;
  globl->identifier_ = std::move(ident);

  // we don't want multiple global defintions
  if (symbol_exists(name)) {
    return nullptr;
  }

  if (peek_token_is(tokentypes::LBracket)) {
    next_token();
    if (!expect_peek(tokentypes::Int))
      STOP_EXECUTION("array declaration needs size.");

    int size = std::stoi(current_.literal);
    add_new_symbol(name, TYPE_ARRAY, convert_type_to_pointer(globl->type_), 0,
                   size);

    if (!expect_peek(tokentypes::RBracket))
      STOP_EXECUTION("array declration must end in ]");
  } else {
    add_new_symbol(name, TYPE_VARIABLE, globl->type_, 0, 1);
  }

  if (!expect_peek(tokentypes::Semicolon))
    return nullptr;

  return globl;
}
