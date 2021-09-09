#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

static std::unordered_map<TokenType, Precedence> precedences = {
  { TOKEN_EQUAL, EQUALS },     { TOKEN_BANG_EQUAL, EQUALS },
  { TOKEN_LESS, LESSGREATER }, { TOKEN_GREATER, LESSGREATER },
  { TOKEN_PLUS, SUM },         { TOKEN_MINUS, SUM },
  { TOKEN_SLASH, PRODUCT },    { TOKEN_STAR, PRODUCT },
  { TOKEN_LEFT_PAREN, CALL },  { TOKEN_LEFT_BRACKET, INDEX },
};

static std::unordered_map<TokenType, InfixParseFn> infix_fns = { {} };
static std::unordered_map<TokenType, PrefixParseFn> prefix_fns = { {} };

// std::unordered_map<TokenType, PrefixarseFn> m_prefix_parse_fns;
// std::unordered_map<TokenType, InfixParseFn> m_infix_parse_fns;
// void add_prefix_parse(TokenType tt, PrefixParseFn fn);
// void add_infix_parse(TokenType tt, InfixParseFn fn);

// void next_token();

// std::unique_ptr<Statement> parse_statement();
// std::unique_ptr<Statement> parse_decl_statement();
// // std::unique_ptr<Statement> parse_return_statement();
// std::unique_ptr<Statement> parse_expression_statement();
// std::unique_ptr<Expression> parse_expression(Precedence prec);
// std::unique_ptr<Expression> parse_identifier();
// std::unique_ptr<Expression> parse_integer_literal();
// std::unique_ptr<Expression> parse_prefix_expression();
// std::unique_ptr<Expression>
// parse_infix_expression(std::unique_ptr<Expression> left);
// std::unique_ptr<Expression> parse_boolean();
// std::unique_ptr<Expression> parse_grouped_expression();
// std::unique_ptr<Expression> parse_if_expression();
// std::unique_ptr<BlockStatement> parse_block_statement();
// std::unique_ptr<Expression> parse_function_literal();
// std::unique_ptr<Expression> parse_string_literal();
// std::unique_ptr<Expression>
// parse_call_expression(std::unique_ptr<Expression> func);
// std::unique_ptr<Expression> parse_array_literal();
// std::unique_ptr<Expression> parse_hash_literal();
// std::unique_ptr<Expression>
// parse_index_expression(std::unique_ptr<Expression> left);
// std::vector<std::unique_ptr<Expression> >
// parse_expression_list(TokenType end);

// std::vector<std::unique_ptr<IdentifierExpr> > parse_function_params();
// std::vector<std::unique_ptr<Expression> > parse_call_arguments();

// Precedence peek_precedence();
// Precedence current_precedence();

// bool expect_peek(TokenType tt);
// bool peek_token_is(TokenType tt);
// bool current_token_is(TokenType tt);
// void peek_error(TokenType tt);

// std::vector<std::string> errors_;

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
  advance();
  if(prefix_fns.find(parser.current.type) == prefix_fns.end()) {
    return nullptr;
  }

  auto fn = prefix_fns[parser.current.type];
  auto left = fn();

  while(!match(TOKEN_SEMICOLON) && prec <= get_curr_prec()) {
    advance();
    auto infix = infix_fns[parser.current.type];
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
  std::unique_ptr<Expression> expr;
  if(match(TOKEN_EQUAL))
    expr = parse_expression(LOWEST);

  consume(TOKEN_SEMICOLON, "expected ';' after declaration");

  auto var_decl = std::make_unique<VariableDecl>();
  var_decl->expr_ = std::move(expr);

  auto stmt = std::make_unique<Statement>(StmtType::VarDecl);
  stmt->var_decl_ = std::move(var_decl);

  return stmt;
}

static std::unique_ptr<Statement>
parse_print_statement()
{
  std::unique_ptr<Expression> expr = parse_expression(LOWEST);
  consume(TOKEN_SEMICOLON, "expected ';' after print statement");

  auto print_stmt = std::make_unique<PrintStmt>();
  print_stmt->expr_ = std::move(expr);

  auto stmt = std::make_unique<Statement>(StmtType::Print);
  stmt->print_stmt_ = std::move(print_stmt);

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

  return nullptr;
}

std::vector<std::unique_ptr<Statement> >
parse(const char *source)
{
  std::vector<std::unique_ptr<Statement> > result;
  init_lexer(source);
  parser = Parser();

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

// add_prefix_parse(tokentypes::IDENT, &Parser::parse_identifier);
// add_prefix_parse(tokentypes::INT, &Parser::parse_integer_literal);
// add_prefix_parse(tokentypes::BANG, &Parser::parse_prefix_expression);
// add_prefix_parse(tokentypes::MINUS, &Parser::parse_prefix_expression);
// add_prefix_parse(tokentypes::TRUE, &Parser::parse_boolean);
// add_prefix_parse(tokentypes::FALSE, &Parser::parse_boolean);
// add_prefix_parse(tokentypes::LPAREN, &Parser::parse_grouped_expression);
// add_prefix_parse(tokentypes::IF, &Parser::parse_if_expression);
// add_prefix_parse(tokentypes::FUNCTION, &Parser::parse_function_literal);
// add_prefix_parse(tokentypes::STRING, &Parser::parse_string_literal);
// add_prefix_parse(tokentypes::LBRACKET, &Parser::parse_array_literal);
// add_prefix_parse(tokentypes::LBRACE, &Parser::parse_hash_literal);

// m_infix_parse_fns = std::unordered_map<TokenType, InfixParseFn>();
// add_infix_parse(tokentypes::PLUS, &Parser::parse_infix_expression);
// add_infix_parse(tokentypes::MINUS, &Parser::parse_infix_expression);
// add_infix_parse(tokentypes::SLASH, &Parser::parse_infix_expression);
// add_infix_parse(tokentypes::ASTERISK, &Parser::parse_infix_expression);
// add_infix_parse(tokentypes::EQ, &Parser::parse_infix_expression);
// add_infix_parse(tokentypes::NEQ, &Parser::parse_infix_expression);
// add_infix_parse(tokentypes::LT, &Parser::parse_infix_expression);
// add_infix_parse(tokentypes::GT, &Parser::parse_infix_expression);
// add_infix_parse(tokentypes::LPAREN, &Parser::parse_call_expression);
// add_infix_parse(tokentypes::LBRACKET, &Parser::parse_index_expression);

// std::unique_ptr<Statement>
// Parser::parse_statement()
// {
//   if(current_.type == tokentypes::LET) {
//     return parse_let_statement();
//   } else if(current_.type == tokentypes::RETURN) {
//     return parse_return_statement();
//   } else {
//     return parse_expression_statement();
//   }
// }

// bool
// Parser::current_token_is(TokenType tt)
// {
//   return current_.type == tt;
// }

// bool
// Parser::peek_token_is(TokenType tt)
// {
//   return peek_.type == tt;
// }

// bool
// Parser::expect_peek(TokenType tt)
// {
//   if(peek_token_is(tt)) {
//     next_token();
//     return true;
//   }

//   peek_error(tt);
//   return false;
// }

// std::unique_ptr<Statement>
// Parser::parse_let_statement()
// {
//   auto letstmt = std::make_unique<LetStatement>();
//   letstmt->token = current_;

//   if(!expect_peek(tokentypes::IDENT)) {
//     return nullptr;
//   }

//   auto ident = std::make_unique<Identifier>();
//   ident->token = current_;
//   ident->value = current_.literal;
//   letstmt->name = std::move(ident);

//   if(!expect_peek(tokentypes::ASSIGN)) {
//     return nullptr;
//   }

//   next_token();

//   letstmt->value = parse_expression(LOWEST);

//   if(peek_token_is(tokentypes::SEMICOLON))
//     next_token();

//   return letstmt;
// }

// std::unique_ptr<Statement>
// Parser::parse_return_statement()
// {
//   auto returnstmt = std::make_unique<ReturnStatement>();
//   returnstmt->token = current_;

//   next_token();
//   returnstmt->return_value = parse_expression(LOWEST);

//   if(peek_token_is(tokentypes::SEMICOLON))
//     next_token();

//   return returnstmt;
// }

// std::unique_ptr<Statement>
// Parser::parse_expression_statement()
// {
//   auto stmt = std::make_unique<ExpressionStatement>();
//   stmt->token = current_;

//   stmt->expression = parse_expression(LOWEST);
//   if(peek_token_is(tokentypes::SEMICOLON)) {
//     next_token();
//   }

//   return stmt;
// }

// std::unique_ptr<Expression>
// Parser::parse_expression(Precedence prec)
// {
//   if(m_prefix_parse_fns.find(current_.type) == m_prefix_parse_fns.end()) {
//     return nullptr;
//   }

//   auto fn = m_prefix_parse_fns[current_.type];
//   auto left = (this->*fn)();

//   while(!peek_token_is(tokentypes::SEMICOLON) && prec < peek_precedence()) {
//     auto infix = m_infix_parse_fns[peek_.type];
//     if(infix == nullptr)
//       return left;
//     next_token();
//     left = (this->*infix)(std::move(left));
//   }

//   return left;
// }

// std::unique_ptr<Expression>
// Parser::parse_identifier()
// {
//   auto identifier = std::make_unique<Identifier>();
//   identifier->token = current_;
//   identifier->value = current_.literal;

//   return identifier;
// }

// std::unique_ptr<Expression>
// Parser::parse_integer_literal()
// {
//   auto lit = std::make_unique<IntegerLiteral>();
//   lit->token = current_;

//   try {
//     int res = std::stoi(current_.literal);
//     lit->value = res;
//   } catch(std::invalid_argument e) {
//     errors_.push_back("could not parse integer");
//     return nullptr;
//   }

//   return lit;
// }

// std::unique_ptr<Expression>
// Parser::parse_prefix_expression()
// {
//   auto exp = std::make_unique<PrefixExpression>();
//   exp->token = current_;
//   exp->opr = current_.literal;

//   next_token();
//   exp->right = parse_expression(PREFIX);

//   return exp;
// }

// Precedence
// Parser::peek_precedence()
// {
//   if(precedences.find(peek_.type) != precedences.end())
//     return precedences.at(peek_.type);
//   return LOWEST;
// }

// Precedence
// Parser::current_precedence()
// {
//   if(precedences.find(current_.type) != precedences.end())
//     return precedences.at(current_.type);
//   return LOWEST;
// }

// std::unique_ptr<Expression>
// Parser::parse_infix_expression(std::unique_ptr<Expression> left)
// {
//   auto exp = std::make_unique<InfixExpression>();
//   exp->token = current_;
//   exp->opr = current_.literal;
//   exp->left = std::move(left);

//   auto prec = current_precedence();
//   next_token();
//   exp->right = std::move(parse_expression(prec));

//   return exp;
// }

// std::unique_ptr<Expression>
// Parser::parse_grouped_expression()
// {
//   next_token();
//   auto exp = parse_expression(LOWEST);
//   if(!expect_peek(tokentypes::RPAREN))
//     return nullptr;

//   return exp;
// }

// std::unique_ptr<Expression>
// Parser::parse_boolean()
// {
//   auto exp = std::make_unique<BooleanExpression>();
//   exp->token = current_;
//   exp->value = current_token_is(tokentypes::TRUE);

//   return exp;
// }

// std::unique_ptr<Expression>
// Parser::parse_if_expression()
// {
//   auto exp = std::make_unique<IfExpression>();
//   exp->token = current_;

//   if(!expect_peek(tokentypes::LPAREN))
//     return nullptr;

//   next_token();
//   exp->cond = parse_expression(LOWEST);

//   if(!expect_peek(tokentypes::RPAREN))
//     return nullptr;

//   if(!expect_peek(tokentypes::LBRACE))
//     return nullptr;

//   exp->after = parse_block_statement();

//   if(peek_token_is(tokentypes::ELSE)) {
//     next_token();

//     if(!expect_peek(tokentypes::LBRACE)) {
//       return nullptr;
//     }

//     exp->other = parse_block_statement();
//   }

//   return exp;
// }

// std::unique_ptr<BlockStmt>
// Parser::parse_block_statement()
// {
//   auto block = std::make_unique<BlockStatement>();
//   block->token = current_;
//   block->statements = std::vector<std::unique_ptr<Statement> >();

//   next_token();

//   while(!current_token_is(tokentypes::RBRACE)
//         && !current_token_is(tokentypes::EOFF)) {
//     auto stmt = parse_statement();
//     if(stmt != nullptr) {
//       block->statements.push_back(std::move(stmt));
//     }
//     next_token();
//   }

//   return block;
// }

// std::unique_ptr<Expression>
// Parser::parse_function_literal()
// {
//   auto lit = std::make_unique<FunctionLiteral>();
//   lit->token = current_;

//   if(!expect_peek(tokentypes::LPAREN))
//     return nullptr;

//   lit->params = parse_function_params();

//   if(!expect_peek(tokentypes::LBRACE))
//     return nullptr;

//   lit->body = std::move(parse_block_statement());

//   return lit;
// }

// std::vector<std::unique_ptr<Identifier> >
// Parser::parse_function_params()
// {
//   std::vector<std::unique_ptr<Identifier> > params;
//   if(peek_token_is(tokentypes::LPAREN)) {
//     next_token();
//     return params;
//   }

//   next_token();

//   auto ident = std::make_unique<Identifier>();
//   ident->token = current_;
//   ident->value = current_.literal;
//   params.push_back(std::move(ident));

//   while(peek_token_is(tokentypes::COMMA)) {
//     next_token();
//     next_token();

//     auto ident = std::make_unique<Identifier>();
//     ident->token = current_;
//     ident->value = current_.literal;
//     params.push_back(std::move(ident));
//   }

//   if(!expect_peek(tokentypes::RPAREN))
//     return std::vector<std::unique_ptr<Identifier> >();

//   return params;
// }

// std::unique_ptr<Expression>
// Parser::parse_call_expression(std::unique_ptr<Expression> func)
// {
//   auto exp = std::make_unique<CallExpression>();
//   exp->token = current_;
//   exp->func = std::move(func);
//   exp->arguments = std::move(parse_expression_list(tokentypes::RPAREN));

//   return exp;
// }

// std::unique_ptr<Expression>
// Parser::parse_string_literal()
// {
//   auto strlit = std::make_unique<StringLiteral>();
//   strlit->token = current_;
//   strlit->value = current_.literal;

//   return strlit;
// }

// std::vector<std::unique_ptr<Expression> >
// Parser::parse_call_arguments()
// {
//   std::vector<std::unique_ptr<Expression> > args;

//   if(peek_token_is(tokentypes::RPAREN)) {
//     next_token();
//     return args;
//   }

//   next_token();
//   args.push_back(std::move(parse_expression(LOWEST)));

//   while(peek_token_is(tokentypes::COMMA)) {
//     next_token();
//     next_token();

//     args.push_back(std::move(parse_expression(LOWEST)));
//   }

//   if(!expect_peek(tokentypes::RPAREN))
//     return std::vector<std::unique_ptr<Expression> >();

//   return args;
// }

// std::vector<std::string>
// Parser::errors() const
// {
//   return errors_;
// }

// void
// Parser::peek_error(TokenType tt)
// {
//   std::string err
//       = "expected next token to be " + tt + " got " + peek_.type + "
//       instead";
//   errors_.push_back(err);
// }

// void
// Parser::add_prefix_parse(TokenType tt, PrefixParseFn fn)
// {
//   m_prefix_parse_fns[tt] = fn;
// }

// void
// Parser::add_infix_parse(TokenType tt, InfixParseFn fn)
// {
//   m_infix_parse_fns[tt] = fn;
// }
