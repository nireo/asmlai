#include "typesystem.h"
#include "parser.h"

namespace typesystem {

void add_type(parser::Node *node) {
  if (node == nullptr || node->tt_ != nullptr)
    return;

  add_type(node->lhs_.get());
  add_type(node->rhs_.get());

  if (node->type_ == parser::NodeType::For) {
    parser::ForNode &for_node = std::get<parser::ForNode>(node->data_);
    add_type(for_node.body_.get());
    add_type(for_node.increment_.get());
    add_type(for_node.initialization_.get());
    add_type(for_node.condition_.get());
  } else if (node->type_ == parser::NodeType::If) {
    parser::IfNode &if_node = std::get<parser::IfNode>(node->data_);
    add_type(if_node.condition_.get());
    add_type(if_node.then_.get());
    add_type(if_node.else_.get());
  } else if (node->type_ == parser::NodeType::Block) {
    auto &vec = std::get<std::vector<parser::NodePtr>>(node->data_);
    for (size_t i = 0; i < vec.size(); ++i) {
      add_type(vec[i].get());
    }
  }

  using NT = parser::NodeType;
  switch (node->type_) {
  case NT::Add:
  case NT::Sub:
  case NT::Mul:
  case NT::Div:
  case NT::Neg:
  case NT::Assign:
    return;
  }

  return;
}
} // namespace typesystem
