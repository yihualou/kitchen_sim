#ifndef KITCHEN_SIM_COURIER_H_
#define KITCHEN_SIM_COURIER_H_

#include <optional>
#include <string>

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "base.h"
#include "model/kitchen.h"
#include "model/order.h"

namespace kitchen_sim {

// Represents a single food delivery courier.
class Courier {
 public:
  struct OrderInfo {
    const std::string order_id;
    Kitchen* kitchen;
  };

  Courier() = default;
  Courier(Courier const&) = delete;
  Courier& operator=(Courier const&) = delete;

  // Always returns true at the moment.
  bool AcceptOrder(const OrderInfo& order_info);

  // Returns a future (along with ownership) to a delivered order.
  // May return nullptr if an order was discarded or expired.
  fibers::future<std::unique_ptr<Order>> PickupCurrentOrder(
      absl::Time at_time = absl::Now());

 private:
  std::optional<OrderInfo> current_order_;
};

}  // namespace kitchen_sim

#endif  // KITCHEN_SIM_COURIER_H_