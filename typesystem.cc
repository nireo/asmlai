#include "typesystem.h"
#include "parser.h"
#include <memory>

namespace typesystem {

#define ADD_NOT_NULL(node)                                                     \
  if (node == nullptr) {                                                       \
    add_type(*node);                                                           \
  }

parser::Type *ptr_to(parser::Type *base) {
  parser::Type *tt = new parser::Type(parser::Types::Ptr);
  tt->base_type_ = base;

  return tt;
}

void add_type(parser::Node &node) {
  if (node.tt_ != nullptr)
    return;

  ADD_NOT_NULL(node.lhs_);
  ADD_NOT_NULL(node.rhs_);

  if (node.type_ == parser::NodeType::For) {
    parser::ForNode &for_node = std::get<parser::ForNode>(node.data_);
    ADD_NOT_NULL(for_node.body_);
    ADD_NOT_NULL(for_node.increment_);
    ADD_NOT_NULL(for_node.initialization_);
    ADD_NOT_NULL(for_node.condition_);
  } else if (node.type_ == parser::NodeType::If) {
    parser::IfNode &if_node = std::get<parser::IfNode>(node.data_);
    ADD_NOT_NULL(if_node.condition_);
    ADD_NOT_NULL(if_node.then_);
    ADD_NOT_NULL(if_node.else_);
  } else if (node.type_ == parser::NodeType::Block) {
    auto &vec = std::get<std::vector<parser::NodePtr>>(node.data_);
    for (size_t i = 0; i < vec.size(); ++i) {
      ADD_NOT_NULL(vec[i]);
    }
  }

  using NT = parser::NodeType;
  switch (node.type_) {
  case NT::Add:
  case NT::Sub:
  case NT::Mul:
  case NT::Div:
  case NT::Neg:
  case NT::Assign:
    node.tt_ = node.lhs_->tt_;
    return;
  case NT::EQ:
  case NT::NE:
  case NT::LE:
  case NT::LT:
  case NT::Variable:
  case NT::Num: {
    node.tt_ = new parser::Type(parser::Types::Int);
    return;
  }
  case NT::Addr: {
    node.tt_ = ptr_to(node.lhs_->tt_);
    return;
  }
  case NT::Derefence: {
    if (node.lhs_->tt_->type_ == parser::Types::Ptr) {
      node.tt_ = node.lhs_->tt_->base_type_;
    } else {
      node.tt_ = new parser::Type(parser::Types::Int);
    }

    return;
  }
  default: {
  }
  }

  return;
}
} // namespace typesystem
