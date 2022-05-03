#ifndef KITCHEN_SIM_ORDER_H_
#define KITCHEN_SIM_ORDER_H_

#include <memory>
#include <optional>

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "base.h"

namespace kitchen_sim {

// Avaiable temperature groups for orders.
enum class TemperatureType { UNKNOWN, FROZEN, COLD, HOT };

// Represents a single food order.
class Order {
 public:
  // Factory method that creates an order from the following parameters:
  // |id| = Unique ID for this specific order.
  // |name| = Order name (e.g. cheeze pizza).
  // |temp| = Temperature group (e.g. hot, cold).
  // |shelf_life_s| = Seconds this order can sit on a shelf before discarding.
  // |decay_rate| = Rate at which value deteriorates per second.
  // |receipt_time| = Time when order was received in the system.
  // Returns an exception if parameters fail validation.
  static std::unique_ptr<Order> CreateOrder(
      const std::string& id, const std::string& name, TemperatureType temp,
      int shelf_life_s, double decay_rate,
      absl::Time receipt_time = absl::Now());

  Order(const std::string& id, const std::string& name, TemperatureType temp,
        int shelf_life_s, double decay_rate, absl::Time receipt_time)
      : id_(id),
        name_(name),
        temp_(temp),
        shelf_life_s_(shelf_life_s),
        decay_rate_(decay_rate),
        receipt_time_(receipt_time) {}
  Order(Order const&) = delete;
  Order& operator=(Order const&) = delete;

  std::string LogMessage(absl::string_view event_type) const;

  void MoveFrom(int current_shelf_decay_modifier,
                absl::Time at_time = absl::Now());

  // Computes the value given |at_time| (defaults to the current time) and
  // |shelf_decay_modifier| (multiplies decay delta, defaults to 1).
  // Takes into account shelf moving.
  double Value(int shelf_decay_modifier,
               absl::Time at_time = absl::Now()) const;

  // Returns estimated time this order expires. Capped to |fulfillment_time_| +
  // |shelf_life_s_|.
  absl::Time Expiry(int current_shelf_decay_modifier) const;

  // Time accessors.
  const std::optional<absl::Time>& FulfillmentTime() const {
    return fulfillment_time_;
  };
  void SetFulfillmentTime(absl::Time at_time) {
    fulfillment_time_.emplace(at_time);
    last_move_time_.emplace(at_time);
  }

  const std::optional<absl::Time>& DeliveryTime() const {
    return delivery_time_;
  };
  void SetDeliveryTime(absl::Time at_time) { delivery_time_.emplace(at_time); }

  const std::string id_;
  const std::string name_;
  const TemperatureType temp_ = TemperatureType::UNKNOWN;
  const int shelf_life_s_ = 0;      // Max seconds before considered waste.
  const double decay_rate_ = 0.0f;  // Per-second.

  // Always set immediately once order comes in.
  const absl::Time receipt_time_;

  std::unique_ptr<boost::asio::system_timer> expiration_timer_;
  std::unique_ptr<boost::asio::system_timer> courier_timer_;

 private:
  // Set at later times in the processing pipeline.
  std::optional<absl::Time> fulfillment_time_;
  std::optional<absl::Time> delivery_time_;

  // May be changed during shelf moves.
  double last_value_at_move_ = 1.0;
  std::optional<absl::Time> last_move_time_;  // Unset if never moved.
};

}  // namespace kitchen_sim

#endif  // KITCHEN_SIM_ORDER_H_