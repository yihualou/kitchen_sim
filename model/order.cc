#include "model/order.h"

namespace kitchen_sim {

std::unique_ptr<Order> Order::CreateOrder(const std::string& id,
                                          const std::string& name,
                                          TemperatureType temp,
                                          int shelf_life_s, double decay_rate,
                                          absl::Time receipt_time) {
  if (id.empty()) {
    throw std::invalid_argument("Order IDs cannot be empty!");
  }
  if (name.empty()) {
    throw std::invalid_argument("Order names cannot be empty!");
  }
  if (temp == TemperatureType::UNKNOWN) {
    throw std::invalid_argument("Order temperature group must be specified!");
  }
  if (shelf_life_s < 0) {
    throw std::invalid_argument("Order shelf life must be non-negative!");
  }
  if (decay_rate < 0) {
    // Doesn't make sense for an order to increase in value as it sits.
    throw std::invalid_argument("Order decay rate must be non-negative!");
  }
  return std::make_unique<Order>(id, name, temp, shelf_life_s, decay_rate,
                                 receipt_time);
}

std::string Order::LogMessage(absl::string_view event_type) const {
  std::string message;
  message.append("[ event: ")
      .append(event_type)
      .append(" | name: ")
      .append(name_)
      .append(" | id: ")
      .append(id_)
      .append(" ]");
  return message;
}

void Order::MoveFrom(int current_shelf_decay_modifier, absl::Time at_time) {
  last_value_at_move_ = Value(current_shelf_decay_modifier, at_time);
  last_move_time_.emplace(at_time);
}

double Order::Value(int shelf_decay_modifier, absl::Time at_time) const {
  if (!fulfillment_time_.has_value() || at_time < fulfillment_time_) {
    // Orders don't have value until cooked.
    return 0.;
  }
  if (absl::ToInt64Seconds(at_time - fulfillment_time_.value()) >
      shelf_life_s_) {
    // Expired.
    return 0.;
  }
  // Since last move.
  const double order_age_s =
      absl::ToDoubleSeconds(at_time - last_move_time_.value());
  const double value =
      last_value_at_move_ -
      (decay_rate_ * order_age_s * shelf_decay_modifier) / shelf_life_s_;
  if (value < 0.) {
    return 0.;
  }
  return value;
}

absl::Time Order::Expiry(int current_shelf_decay_modifier) const {
  double ttl_s = last_value_at_move_ * shelf_life_s_ /
                 (decay_rate_ * current_shelf_decay_modifier);
  return std::min(last_move_time_.value() + absl::Seconds(ttl_s),
                  fulfillment_time_.value() + absl::Seconds(shelf_life_s_));
}
}  // namespace kitchen_sim