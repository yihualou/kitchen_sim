#ifndef KITCHEN_SIM_KITCHEN_H_
#define KITCHEN_SIM_KITCHEN_H_

#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "base.h"
#include "model/order.h"

namespace kitchen_sim {

// Represents a single food kitchen. Provides functionality for handling
// orders and returning order values.
class Kitchen {
 public:
  struct Options {
    std::string name;
    int overflow_capacity = 15;
    // Temperature group to shelf capacity.
    std::unordered_map<TemperatureType, int> temp_to_capacity = {
        {TemperatureType::HOT, 10},
        {TemperatureType::COLD, 10},
        {TemperatureType::FROZEN, 10},
    };
  };

  // Represents a single order shelf.
  class Shelf {
   public:
    explicit Shelf(int max_capacity, int decay_modifier)
        : max_capacity_(max_capacity), decay_modifier_(decay_modifier) {
      orders_.reserve(max_capacity);
    }

    bool AddOrder(const Order* order) {
      if (AtCapacity()) {
        return false;
      }
      orders_.insert(order);
      return true;
    }

    void RemoveOrder(const Order* order) { orders_.erase(order); }

    const std::unordered_set<const Order*>& Orders() const { return orders_; }

    bool AtCapacity() const { return orders_.size() >= max_capacity_; }

    int DecayModifier() const { return decay_modifier_; }

   private:
    const size_t max_capacity_;
    const int decay_modifier_;

    // Kitchen retains ownership of individual orders.
    std::unordered_set<const Order*> orders_;
  };

  Kitchen(const Options options, boost::asio::io_context& context);
  Kitchen(Kitchen const&) = delete;
  Kitchen& operator=(Kitchen const&) = delete;

  // Takes |order| and attempts to place it on the shelf matching its
  // temperature. If that fails, the order is placed on the overflow shelf; if
  // already full, room can be made by shifting an existing order to a
  // single-temperature shelf. Failing that, an overflow order is randomly
  // discarded to make room.
  // Returns a future that fires when cooking is done.
  fibers::future<Order*> TakeOrder(std::unique_ptr<Order> order,
                                   absl::Time at_time = absl::Now());

  // Returns an order matching |order_id| (or nullptr if none is found) and
  // removes it from its shelf.
  // Transfers ownership to caller if found.
  std::unique_ptr<Order> PickupOrder(absl::string_view order_id,
                                     absl::Time at_time = absl::Now());

  // Returns the value of the order with |id|.
  double OrderValue(absl::string_view id,
                    absl::Time at_time = absl::Now()) const;

  const Shelf& TemperatureShelf(TemperatureType temp) const {
    return *shelves_.at(temp);
  }
  const Shelf& OverflowShelf() const { return overflow_shelf_; }

  // Prints out current shelf contents to the info log.
  void LogShelves() const;

  boost::asio::io_context::strand& Strand() { return strand_; }

 private:
  // Places |order| on |shelf| updating any necessary bookkkeeping.
  bool PlaceOrder(Order* order, Shelf* shelf);

  // Attempts to move a single order (first possible option taken)
  // from the overflow shelf to the shelf matching its temperature group.
  // Failing that, an order is randomly discarded.
  void MakeOverflowRoom();

  const Options options_;

  // Index from temperature group to shelf (excludes overflow shelf).
  std::unordered_map<TemperatureType, std::unique_ptr<Shelf>> shelves_;
  Shelf overflow_shelf_;

  // Order bookkeeping.
  std::unordered_map<absl::string_view, std::unique_ptr<Order>>
      orders_;  // Indexed by ID
  std::unordered_map<absl::string_view, Shelf*>
      order_to_shelf_;  // Indexed by ID

  std::mt19937 rand_;

  // ASIO bookkeeping.
  boost::asio::io_context& context_;
  boost::asio::io_context::strand strand_;
};

}  // namespace kitchen_sim

#endif  // KITCHEN_SIM_KITCHEN_H_