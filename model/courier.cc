#include "model/courier.h"

namespace kitchen_sim {

bool Courier::AcceptOrder(const OrderInfo& order_info) {
  current_order_.emplace(order_info);
  return true;
}

fibers::future<std::unique_ptr<Order>> Courier::PickupCurrentOrder(
    absl::Time at_time) {
  fibers::promise<std::unique_ptr<Order>> delivered_order;
  if (!current_order_.has_value()) {
    throw std::invalid_argument(
        "Cannot pickup when no order has been accepted!");
  }
  // Presently delivers immediately.
  auto order = current_order_.value().kitchen->PickupOrder(
      current_order_.value().order_id, at_time);
  current_order_.reset();

  if (order != nullptr) {
    order->SetDeliveryTime(at_time);
    delivered_order.set_value(std::move(order));
  } else {
    delivered_order.set_value(nullptr);
  }
  return delivered_order.get_future();
}

}  // namespace kitchen_sim
