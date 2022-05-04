#include "typesystem.h"
#include "parser.h"
#include <iostream>
#include <memory>
#include <variant>

namespace typesystem {

#define ADD_NOT_NULL(node)                                                     \
  if (node != nullptr) {                                                       \
    add_type(*node);                                                           \
  }

bool is_number(parser::Type *ty) {
  return ty->type_ == parser::Types::Int || ty->type_ == parser::Types::Char ||
         ty->type_ == parser::Types::Short ||
         ty->type_ == parser::Types::Long || ty->type_ == parser::Types::Bool ||
         ty->type_ == parser::Types::Enum;
}

parser::Type *enum_type() {
  return new parser::Type(parser::Types::Enum, parser::kNumberSize,
                          parser::kNumberSize);
}

parser::Type *ptr_to(parser::Type *base) {
  parser::Type *tt =
      new parser::Type(parser::Types::Ptr, parser::kPtrSize, parser::kPtrSize);
  tt->base_type_ = base;
  return tt;
}

parser::Type *func_ty(parser::Type *return_ty) {
  parser::Type *ty = new parser::Type(parser::Types::Function, 0);
  ty->optional_data_ = return_ty;

  return return_ty;
}

parser::Type *array_of_type(parser::Type *array_type, i32 length) {
  parser::Type *ty = new parser::Type(
      parser::Types::Array, array_type->size_ * length, array_type->align_);
  parser::ArrayType array_data_;

  array_data_.array_length = length;
  ty->base_type_ = array_type;
  ty->name_ = array_type->name_;
  ty->optional_data_ = std::move(array_data_);

  return ty;
}

void add_type(parser::Node &node) {
  if (node.tt_->type_ != parser::Types::Empty)
    return;

  ADD_NOT_NULL(node.lhs_);
  ADD_NOT_NULL(node.rhs_);

  if (node.type_ == parser::NodeType::For) {
    parser::ForNode &for_node = std::get<parser::ForNode>(node.data_);
    ADD_NOT_NULL(for_node.body_);
    ADD_NOT_NULL(for_node.increment_);
    ADD_NOT_NULL(for_node.initialization_);
    ADD_NOT_NULL(for_node.condition_);
    return;
  } else if (node.type_ == parser::NodeType::If) {
    parser::IfNode &if_node = std::get<parser::IfNode>(node.data_);
    ADD_NOT_NULL(if_node.condition_);
    ADD_NOT_NULL(if_node.then_);
    ADD_NOT_NULL(if_node.else_);
    return;
  } else if (node.type_ == parser::NodeType::Block) {
    try {
      auto &vec = std::get<std::vector<parser::NodePtr>>(node.data_);
      for (auto &d : vec) {
        add_type(*d);
      }
      return;
    } catch (std::bad_variant_access &e) {
      return; // we just return as we have just encountered a null-expression
              // i.e ';'.
    }
  }

  using NT = parser::NodeType;
  switch (node.type_) {
  case NT::Add:
  case NT::Sub:
  case NT::Mul:
  case NT::Div:
  case NT::Mod:
  case NT::Neg:
    node.tt_ = node.lhs_->tt_;
    return;
  case NT::Assign:
    if (node.lhs_->tt_->type_ == parser::Types::Array) {
      std::fprintf(stderr, "assigning to a non-lvalue");
      std::exit(1);
    }

    node.tt_ = node.lhs_->tt_;
    return;
  case NT::EQ:
  case NT::NE:
  case NT::LE:
  case NT::LT:
  case NT::FunctionCall:
  case NT::Num: {
    node.tt_ = parser::default_long;
    return;
  }
  case NT::LogAnd:
  case NT::LogOr:
  case NT::Not: {
    node.tt_ = new parser::Type(parser::Types::Int, parser::kNumberSize,
                                parser::kNumberSize);
    return;
  }
  case NT::Variable: {
    node.tt_ = std::get<std::shared_ptr<parser::Object>>(node.data_)->ty_;
    return;
  }
  case NT::Addr: {
    if (node.lhs_->tt_->type_ == parser::Types::Array) {
      node.tt_ = ptr_to(node.lhs_->tt_->base_type_);
    } else {
      node.tt_ = ptr_to(node.lhs_->tt_);
    }
    return;
  }
  case NT::Derefence: {
    if (!node.lhs_->tt_->base_type_) {
      std::fprintf(stderr, "invalid pointer dereference.");
      std::exit(1);
    }

    if (node.lhs_->tt_->base_type_->type_ == parser::Types::Void) {
      std::fprintf(stderr, "dereferencing a void pointer");
      std::exit(1);
    }

    node.tt_ = node.lhs_->tt_->base_type_;
    return;
  }
  case NT::StmtExpr: {
    try {
      const auto &body_node = *std::get<parser::NodePtr>(node.data_);
      const auto &node_list = std::get<parser::NodeList>(body_node.data_);

      if (node_list[node_list.size() - 1]->type_ == NT::ExprStmt) {
        node.tt_ = node_list[node_list.size() - 1]->tt_;
        return;
      }
    } catch (const std::bad_variant_access &e) {
      std::fprintf(stderr, "statement expression needs to return a type.");
      return;
    }
  }
  case NT::Comma: {
    node.tt_ = node.rhs_->tt_;
    return;
  }
  case NT::Member: {
    try {
      node.tt_ = std::get<parser::Member *>(node.data_)->type;
      return;
    } catch (const std::bad_variant_access &e) {
      std::fprintf(stderr, "cannot access member in node pointer.");
      return;
    }
    return;
  }
  default: {
  }
  }

  return;
}
} // namespace typesystem
