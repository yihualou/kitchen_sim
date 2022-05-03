#ifndef KITCHEN_SIM_LIB_H_
#define KITCHEN_SIM_LIB_H_

#include <random>

#include "model/courier.h"
#include "model/kitchen.h"
#include "model/order.h"

namespace kitchen_sim {

// Simulates the intake, fulfillment, and delivery of a stream of orders for a
// single kitchen.
class KitchenSimulation {
 public:
  struct Options {
    std::string kitchen_name;

    // Predefined LARGE (default) and SMALL settings.
    std::string kitchen_size = "LARGE";

    // Rate at which to process incoming orders.
    double orders_per_second = 2.;

    // Whether to continue the simulation after seeing an invalid order.
    bool continue_after_invalid_order = false;

    // Default number of worker threads.
    unsigned int thread_count = std::thread::hardware_concurrency();
  };

  KitchenSimulation(const Options options)
      : options_(options),
        rand_(random_device_()),
        context_(),
        kitchen_(CreateKitchen(options_, context_)) {}
  KitchenSimulation(KitchenSimulation const&) = delete;
  KitchenSimulation& operator=(KitchenSimulation const&) = delete;

  // Handle orders one-by-one starting from |begin|.
  template <typename OrderIterator>
  void Run(OrderIterator begin, OrderIterator end);

  // Convenient variant of the above that works off a JSON file of orders.
  void RunFromJson(const std::string& json_path);

 private:
  static Kitchen CreateKitchen(const Options& options,
                               boost::asio::io_context& context) {
    if (options.kitchen_size == "SMALL") {
      return Kitchen({options.kitchen_name,
                      6,
                      {{TemperatureType::HOT, 4},
                       {TemperatureType::COLD, 4},
                       {TemperatureType::FROZEN, 4}}},
                     context);
    }
    return Kitchen({options.kitchen_name,
                    15,
                    {{TemperatureType::HOT, 10},
                     {TemperatureType::COLD, 10},
                     {TemperatureType::FROZEN, 10}}},
                   context);
  }

  template <typename OrderIterator>
  void Tick(OrderIterator begin, OrderIterator end, absl::Duration interval,
            boost::asio::system_timer* timer);

  const Options options_;

  std::random_device random_device_;
  std::mt19937 rand_;

  boost::asio::io_context context_;
  Kitchen kitchen_;
};

}  // namespace kitchen_sim

#endif  // KITCHEN_SIM_LIB_H_