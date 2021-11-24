#include "parser.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "ast.h"
#include "codegen_x64.h"
#include "compiler.h"
#include "token.h"
#include "types.h"

#define PARSER_ERROR(message, ...)                                             \
  do {                                                                         \
    std::fprintf(stderr, "[line %d] ", (int)lx_->line_);                       \
    std::fprintf(stderr, message, ##__VA_ARGS__);                              \
    std::fprintf(stderr, "\n");                                                \
    std::exit(1);                                                              \
  } while (false);
static std::string latest_function_name;

std::unique_ptr<Program> Parser::parse_program() {
  add_new_symbol("print_num", TYPE_FUNCTION, TYPE_CHAR);
  add_new_symbol("print_char", TYPE_FUNCTION, TYPE_CHAR);

  auto program = std::make_unique<Program>();
  program->statements_ = std::vector<StatementPtr>();

  while (current_.type != TokenType::Eof) {
    auto stmt = parse_statement();
    if (stmt != nullptr) {
      program->statements_.push_back(std::move(stmt));
    } else {
      PARSER_ERROR("error parsing statement\n");
    }
    next_token();
  }

  return program;
}

Parser::Parser(std::unique_ptr<LLexer> lx) {
  lx_ = std::move(lx);
  next_token();
  next_token();
}

void Parser::next_token() {
  current_ = peek_;
  peek_ = lx_->next_token();
}

StatementPtr Parser::parse_statement() {
  switch (current_.type) {
  case TokenType::Function:
    return parse_function_literal();
  case TokenType::Return:
    return parse_return_statement();
  case TokenType::Global:
    return parse_global_decl();
  case TokenType::Var:
    return parse_var_decl();
  case TokenType::While: {
    auto stmt = std::make_unique<ExpressionStatement>();
    stmt->expression_ = parse_while_expression();

    return stmt;
  }
  case TokenType::If: {
    auto stmt = std::make_unique<ExpressionStatement>();
    stmt->expression_ = parse_if_expression();

    return stmt;
  }
  default:
    return parse_expression_statement();
  }
}

// this cannot really be held inside the typesystem namespace as it needs the
// parsers local varialbes.
ValueT Parser::parse_type() {
  ValueT type;

  switch (peek_.type) {
  case TokenType::IntType: {
    type = TYPE_INT;
    break;
  }
  case TokenType::Void: {
    type = TYPE_VOID;
    break;
  }
  case TokenType::CharType: {
    type = TYPE_CHAR;
    break;
  }
  case TokenType::LongType: {
    type = TYPE_LONG;
    break;
  }
  default:
    PARSER_ERROR("cannot parser type for token: %s \n", peek_.literal_.c_str());
  }

  while (true) {
    next_token();
    if (!peek_token_is(TokenType::Asterisk)) {
      break;
    }
    type = typesystem::convert_to_ptr(type);
  }

  return type;
}

bool Parser::current_token_is(TokenType tt) { return current_.type == tt; }

bool Parser::peek_token_is(TokenType tt) { return peek_.type == tt; }

bool Parser::expect_peek(TokenType tt) {
  if (peek_token_is(tt)) {
    next_token();
    return true;
  }

  return false;
}

std::vector<ExpressionPtr> Parser::parse_expression_list() {
  std::vector<ExpressionPtr> exprs;

  while (current_.type != TokenType::RParen) {
    exprs.push_back(parse_expression_rec(LOWEST));
    if (current_token_is(TokenType::Comma)) {
      next_token();
    } else if (current_token_is(TokenType::RParen)) {
    } else {
      PARSER_ERROR("need ',' or ')' after list of expressions");
    }
  }

  return exprs;
}

ExpressionPtr Parser::parse_call(ExpressionPtr ident) {
  next_token();
  next_token();

  auto arguments_ = parse_expression_list();
  if (!current_token_is(TokenType::RParen))
    PARSER_ERROR(
        "function call arguments needs to be wrapped in parenthesies.");

  auto call_exp = std::make_unique<CallExpression>();

  call_exp->func_ = std::move(ident);
  call_exp->arguments_ = std::move(arguments_);

  return call_exp;
}

ExpressionPtr Parser::parse_array(ExpressionPtr ident) {
  next_token();
  next_token();
  auto arr = std::make_unique<Addr>();
  arr->to_addr_ = std::move(ident);

  auto indx = parse_expression_rec(LOWEST);
  if (!current_token_is(TokenType::RBracket))
    PARSER_ERROR("array index operation needs to end in a ]\n");

  if (indx->ValueType() != TYPE_INT && indx->ValueType() != TYPE_CHAR &&
      indx->ValueType() != TYPE_LONG)
    PARSER_ERROR("the index expression needs to be an integer.");

  auto index_modified = typesystem::change_type(
      std::move(indx), arr->ValueType(), TokenType::Plus);

  auto infix = std::make_unique<InfixExpression>();
  infix->opr = TokenType::Plus;
  infix->v_type_ = arr->ValueType();
  infix->left_ = std::move(arr);

  if (index_modified.second == nullptr)
    PARSER_ERROR("right type cannot be changed.");
  infix->right_ = std::move(index_modified.second);

  auto deref = std::make_unique<Dereference>();
  deref->to_dereference_ = std::move(infix);

  return deref;
}

ExpressionPtr Parser::parse_postfix() {
  auto identifier = parse_identifier();

  if (peek_token_is(TokenType::LParen)) {
    return parse_call(std::move(identifier));
  } else if (peek_token_is(TokenType::LBracket)) {
    return parse_array(std::move(identifier));
  }

  next_token();

  if (current_.type == TokenType::Inc || current_.type == TokenType::Dec) {
    auto type = current_.type;
    next_token();
    auto action = std::make_unique<IdentifierAction>();
    action->action_ = type;
    action->identifier_ = std::move(identifier);

    return action;
  }

  return identifier;
}

StatementPtr Parser::parse_return_statement() {
  auto returnstmt = std::make_unique<ReturnStatement>();
  const std::string top = latest_function_name;
  returnstmt->function_identifier_ = top;
  ValueT function_return_type_ = get_symbol(top).value_type_;

  next_token();

  returnstmt->types_ = function_return_type_;

  auto value = parse_expression_rec(LOWEST);

  returnstmt->return_value_ = std::move(value);

  return returnstmt;
}

StatementPtr Parser::parse_expression_statement() {
  auto stmt = std::make_unique<ExpressionStatement>();
  stmt->expression_ = parse_expression_rec(LOWEST);

  return stmt;
}

ExpressionPtr Parser::parse_primary() {
  ExpressionPtr result = nullptr;
  switch (current_.type) {
  case TokenType::Int: {
    result = parse_integer_literal();
    break;
  }
  case TokenType::String: {
    auto strlit = std::make_unique<StringLiteral>();

    std::string value = "";
    value += current_.literal_;

    int label = get_next_label();

    codegen::global_str(label, value);

    strlit->value_ = value;
    strlit->id_ = label;
    result = std::move(strlit);

    break;
  }
  case TokenType::Ident: {
    auto identifier = parse_identifier();

    if (peek_token_is(TokenType::LParen)) {
      result = parse_call(std::move(identifier));
      break;
    } else if (peek_token_is(TokenType::LBracket)) {
      result = parse_array(std::move(identifier));
      break;
    } else if (peek_token_is(TokenType::Inc)) {
      next_token();
      next_token();
      auto action = std::make_unique<IdentifierAction>();
      action->action_ = TokenType::Inc;
      action->identifier_ = std::move(identifier);

      return action;
    } else if (peek_token_is(TokenType::Dec)) {
      next_token();
      next_token();
      auto action = std::make_unique<IdentifierAction>();
      action->action_ = TokenType::Inc;
      action->identifier_ = std::move(identifier);

      return action;
    }

    result = std::move(identifier);
    break;
  }
  case TokenType::LParen: {
    next_token();

    auto expr = parse_expression_rec(LOWEST);
    if (!current_token_is(TokenType::RParen))
      PARSER_ERROR("expected right paren after");

    return expr;
  }
  default:
    PARSER_ERROR(
        "unrecognized token when parsing primary expression factor. %s",
        current_.literal_.c_str())
  }

  next_token();
  return result;
}

ExpressionPtr Parser::parse_prefix() {
  switch (current_.type) {
  case TokenType::Minus:
  case TokenType::Bang:
  case TokenType::Asterisk:
  case TokenType::Amper: {
    auto type = current_.type;
    next_token();

    auto right = parse_prefix();
    if (type != TokenType::Minus && type != TokenType::Bang) {
      if (right->Type() != AstType::Identifier)
        PARSER_ERROR(
            "ampersand or amper cannot be used for nothing but identifiers.\n");
    }

    if (type == TokenType::Amper) {
      auto addr_exp = std::make_unique<Addr>();
      addr_exp->to_addr_ = std::move(right);

      return addr_exp;
    } else if (type == TokenType::Amper) {
      auto deref_exp = std::make_unique<Dereference>();
      deref_exp->to_dereference_ = std::move(right);

      return deref_exp;
    } else if (type == TokenType::Bang || type == TokenType::Minus) {
      next_token();

      auto right = parse_prefix();
      auto prefix = std::make_unique<PrefixExpression>();
      prefix->opr = type;
      prefix->right_ = std::move(right);

      return prefix;
    } else {
      std::exit(1); // cannot get here
    }
  }
  case TokenType::Dec:
  case TokenType::Inc: {
    auto type = current_.type;
    next_token();

    auto right = parse_prefix();
    if (right->Type() != AstType::Identifier)
      PARSER_ERROR("cannot increment/decrement a non-identifier.");

    auto identifier_action = std::make_unique<IdentifierAction>();
    identifier_action->identifier_ = std::move(right);
    identifier_action->post_ = false;
    identifier_action->action_ = type;

    return identifier_action;
  }
  default:
    return parse_primary();
  }
}

ExpressionPtr Parser::parse_while_expression() {
  auto while_stmt = std::make_unique<WhileStatement>();
  if (!expect_peek(TokenType::LParen))
    PARSER_ERROR("while statement should be followed by a left paren.");

  next_token();
  while_stmt->cond_ = parse_expression_rec(LOWEST);

  if (current_token_is(TokenType::LBrace)) {
  } else {
    if (!expect_peek(TokenType::LBrace))
      PARSER_ERROR(
          "while statement condition should be followed by a left brace.");
  }

  while_stmt->body_ = parse_block_statement();

  return while_stmt;
}

ExpressionPtr Parser::parse_expression_rec(Precedence prec) {
  auto left = parse_prefix();

  if (current_token_is(TokenType::Semicolon) ||
      current_token_is(TokenType::RParen)) {
    left->set_rvalue(true);
    return left;
  }

  auto tokentype = current_.type;

  while ((prec < precedences[tokentype]) ||
         (tokentype == TokenType::Assign && precedences[tokentype] == prec)) {
    next_token();

    auto right = parse_expression_rec(precedences[tokentype]);
    auto left_type = left->ValueType();

    if (tokentype == TokenType::Assign) {
      right->set_rvalue(true);

      auto right_temp =
          typesystem::change_type(std::move(right), left_type, tokentype);
      if (right_temp.second == nullptr) {
        PARSER_ERROR("incompatible type in assingment");
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

      auto left_temp = typesystem::change_type(std::move(left),
                                               right->ValueType(), tokentype);
      auto right_temp =
          typesystem::change_type(std::move(right), left_type, tokentype);

      if (left_temp.second == nullptr && right_temp.second == nullptr) {
        PARSER_ERROR("bad types in expression\n");
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
    if (current_token_is(TokenType::Semicolon) ||
        current_token_is(TokenType::RParen) ||
        tokentype == TokenType::RBracket) {
      left->set_rvalue(true);
      return left;
    }
  }

  left->set_rvalue(true);
  return left;
}

ExpressionPtr Parser::parse_identifier() {
  auto identifier = std::make_unique<Identifier>();
  identifier->value_ = current_.literal_;

  if (latest_function_name != "") {
    const auto &sym =
        get_symbol_w_func(latest_function_name, current_.literal_);
    identifier->value_type = sym.value_type_;
  } else {
    const auto &sym = get_symbol(current_.literal_);
    identifier->value_type = sym.value_type_;
  }

  return identifier;
}

ExpressionPtr Parser::parse_integer_literal() {
  auto lit = std::make_unique<IntegerLiteral>();

  try {
    int res = std::stoi(current_.literal_);
    lit->value_ = res;
  } catch (const std::invalid_argument &e) {
    PARSER_ERROR("cannot convert string into number.\n");
  }

  return lit;
}

ExpressionPtr Parser::parse_if_expression() {
  auto exp = std::make_unique<IfExpression>();

  if (!expect_peek(TokenType::LParen))
    PARSER_ERROR("if expressions should be followed by left paren.");

  next_token();
  exp->cond_ = parse_expression_rec(LOWEST);

  if (!current_token_is(TokenType::RParen))
    PARSER_ERROR("if expression condition should be followed by right paren.");

  if (!expect_peek(TokenType::LBrace))
    PARSER_ERROR("if expression condition should be followed by left brace.");

  exp->after_ = parse_block_statement();

  if (peek_token_is(TokenType::Else)) {
    next_token();

    if (!expect_peek(TokenType::LBrace))
      PARSER_ERROR("else keyword should be followed by left brace");

    exp->other_ = parse_block_statement();
  }

  return exp;
}

std::unique_ptr<BlockStatement> Parser::parse_block_statement() {
  auto block = std::make_unique<BlockStatement>();
  block->statements_ = std::vector<StatementPtr>();

  next_token();

  while (!current_token_is(TokenType::RBrace) &&
         !current_token_is(TokenType::Eof)) {
    auto stmt = parse_statement();
    if (stmt != nullptr) {
      block->statements_.push_back(std::move(stmt));
    }
    next_token();
  }

  return block;
}

StatementPtr Parser::parse_function_literal() {
  auto lit = std::make_unique<FunctionLiteral>();

  if (!expect_peek(TokenType::Ident))
    PARSER_ERROR("function keyword should be followed by an identifier");

  auto ident = std::make_unique<Identifier>();
  std::string name = current_.literal_;

  latest_function_name = name; // inside function, this is needed for type
                               // checking, and some ast generation
  create_new_function_table(name);

  ident->value_ = name;
  lit->name_ = std::move(ident);

  if (!expect_peek(TokenType::LParen))
    PARSER_ERROR("function name should be followed by left paren.");

  // check if function has already been prototyped
  int param_count = -1;
  if (symbol_exists(name)) {
    const auto &sym = get_symbol(name);
    if (sym.type_ == TYPE_FUNCTION)
      PARSER_ERROR("global variable with same name as function.");

    // since it is a prototype, all of the function local variables, are just
    // parameters so no need to filter
    param_count = (int)get_function_locals(name).size();
  }

  lit->params_ = parse_function_params();
  if (param_count != -1 && (int)lit->params_.size() == param_count)
    PARSER_ERROR("wrong amount of parameters in function literal compared to "
                 "prototype.");

  if (!expect_peek(TokenType::Arrow))
    PARSER_ERROR("function parameters should be followed by an arrow.");

  lit->return_type_ = parse_type();
  add_new_symbol(name, TYPE_FUNCTION, lit->return_type_, get_next_label());

  if (peek_token_is(TokenType::Semicolon)) {
    next_token();
    lit->is_prototype = true;
    latest_function_name = "";
    return lit;
  } else {
    if (!expect_peek(TokenType::LBrace))
      PARSER_ERROR("function type should be followed by an left brace.");

    lit->body_ = parse_block_statement();
    latest_function_name = ""; // outside function no function name

    return lit;
  }
}

std::vector<std::unique_ptr<Identifier>> Parser::parse_function_params() {
  std::vector<std::unique_ptr<Identifier>> params;
  if (peek_token_is(TokenType::RParen)) {
    next_token();
    return params;
  }

  auto type = parse_type();
  next_token();

  auto ident = std::make_unique<Identifier>();
  ident->value_ = current_.literal_;

  // the parameters are stored in the local table meaning that they will be
  // overwritten after a function.
  new_function_param(latest_function_name, ident->value_, type, 0, 1);
  params.push_back(std::move(ident));

  next_token();
  while (current_token_is(TokenType::Comma)) {
    auto type = parse_type();
    next_token();

    auto ident = std::make_unique<Identifier>();
    ident->value_ = current_.literal_;

    new_function_param(latest_function_name, ident->value_, type, 0, 1);

    params.push_back(std::move(ident));
    next_token();
  }

  if (current_token_is(TokenType::RParen)) {
    return params;
  }

  if (!expect_peek(TokenType::RParen)) {
    PARSER_ERROR(
        "function parameters need to be followed by right parenthesis\n.");
  }

  return params;
}

StatementPtr Parser::parse_var_decl() {
  // cannot use var keyword out of function
  if (latest_function_name == "")
    PARSER_ERROR("var declarations only inside functions.");

  auto var_decl = std::make_unique<VarDecl>();
  var_decl->type_ = parse_type();

  if (!expect_peek(TokenType::Ident))
    PARSER_ERROR("var keyword should be followed by an identifier.");

  auto ident = std::make_unique<Identifier>();
  std::string name = current_.literal_;

  ident->value_ = name;
  var_decl->identifier_ = std::move(ident);
  new_function_local(latest_function_name, name, var_decl->type_, 0, 1);

  if (peek_token_is(TokenType::LBracket)) {
    next_token();
    if (!expect_peek(TokenType::Int))
      PARSER_ERROR("array declaration needs size.");

    int size = std::stoi(current_.literal_.data());
    add_new_symbol(name, TYPE_ARRAY,
                   typesystem::convert_to_ptr(var_decl->type_), 0, size);

    if (!expect_peek(TokenType::RBracket))
      PARSER_ERROR("array declration must end in ]");
  } else {
    add_new_symbol(name, TYPE_VARIABLE, var_decl->type_, 0, 1);
  }

  if (!expect_peek(TokenType::Semicolon))
    PARSER_ERROR("var declaration should be followed by an semicolon.");

  return var_decl;
}

StatementPtr Parser::parse_global_decl() {
  // cannot use global keyword inside function
  if (latest_function_name != "")
    PARSER_ERROR("global declarations cannot be inside functions.");

  auto globl = std::make_unique<GlobalVariable>();
  globl->type_ = parse_type();

  if (!expect_peek(TokenType::Ident))
    PARSER_ERROR("global keyword should be followed by an identifier.");

  auto ident = std::make_unique<Identifier>();
  const std::string name = current_.literal_.data();
  ident->value_ = name;
  globl->identifier_ = std::move(ident);

  // we don't want multiple global defintions
  if (symbol_exists(name))
    PARSER_ERROR("found multiple global definitions of '%s'", name.c_str());

  if (peek_token_is(TokenType::LBracket)) {
    next_token();
    if (!expect_peek(TokenType::Int))
      PARSER_ERROR("array declaration needs size.");

    int size = std::stoi(current_.literal_.data());
    add_new_symbol(name, TYPE_ARRAY, typesystem::convert_to_ptr(globl->type_),
                   0, size);

    if (!expect_peek(TokenType::RBracket))
      PARSER_ERROR("array declration must end in ]");
  } else {
    add_new_symbol(name, TYPE_VARIABLE, globl->type_, 0, 1);
  }

  if (!expect_peek(TokenType::Semicolon))
    PARSER_ERROR("global declaration should be followed by an semicolon.")

  return globl;
}
