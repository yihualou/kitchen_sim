
#include "kitchen_sim_lib.h"

#include <fstream>

#include "absl/strings/str_cat.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "boost/bind.hpp"
#include "boost/log/trivial.hpp"
#include "single_include/nlohmann/json.hpp"

namespace kitchen_sim {
namespace {

constexpr absl::string_view kReceived = "RECEIVED";
constexpr absl::string_view kCooked = "COOKED";
constexpr absl::string_view kDelivered = "DELIVERED";

// Random time 2-6 seconds later.
absl::Time CourierArrivalTime(std::mt19937& rand) {
  std::uniform_int_distribution<int> dist(2, 6);
  return absl::Now() + absl::Seconds(dist(rand));
}

}  // namespace

template <typename OrderIterator>
void KitchenSimulation::Tick(OrderIterator begin, OrderIterator end,
                             absl::Duration interval,
                             boost::asio::system_timer* timer) {
  std::unique_ptr<Order> order = std::move(*begin);
  const std::string order_id = order->id_;
  BOOST_LOG_TRIVIAL(debug) << order->LogMessage(kReceived);

  // 1. Order cooked.
  Order* cooked_order = WaitAndGet(kitchen_.TakeOrder(std::move(order)));
  BOOST_LOG_TRIVIAL(info) << cooked_order->LogMessage(kCooked);
  kitchen_.LogShelves();

  // 2. Courier accepts.
  auto courier = std::make_unique<Courier>();
  courier->AcceptOrder({order_id, &kitchen_});
  BOOST_LOG_TRIVIAL(debug) << cooked_order->LogMessage("ACCEPTED");

  cooked_order->courier_timer_ =
      std::make_unique<boost::asio::system_timer>(context_);
  cooked_order->courier_timer_->expires_at(
      absl::ToChronoTime(CourierArrivalTime(rand_)));
  cooked_order->courier_timer_->async_wait(boost::asio::bind_executor(
      kitchen_.Strand(),
      [=, courier = move(courier)](const boost::system::error_code& e) {
        if (e == boost::asio::error::operation_aborted) return;
        // 3. Courier arrives.
        // 4. Order is delivered.
        auto delivered_order = WaitAndGet(courier->PickupCurrentOrder());
        if (delivered_order != nullptr) {
          BOOST_LOG_TRIVIAL(info) << delivered_order->LogMessage(kDelivered);
          kitchen_.LogShelves();
        } else {
          // Already expired or discarded.
        }
      }));

  // Schedule continuation.
  begin++;
  if (begin != end) {
    timer->expires_after(absl::ToChronoMilliseconds(interval));
    timer->async_wait([=](const boost::system::error_code& e) {
      if (e == boost::asio::error::operation_aborted) return;
      Tick(begin, end, interval, timer);
    });
  }
}

template <typename OrderIterator>
void KitchenSimulation::Run(OrderIterator begin, OrderIterator end) {
  std::cout << "SIMULATION START!" << std::endl;
  absl::Duration interval = absl::Seconds(1. / options_.orders_per_second);

  // Set up primary tick timer.
  boost::asio::system_timer timer(context_);
  timer.async_wait([=, &timer](const boost::system::error_code& e) {
    if (e == boost::asio::error::operation_aborted) return;
    Tick(begin, end, interval, &timer);
  });

  // Set up thread pool and run.
  std::vector<std::thread> threads;
  for (auto i = 0; i < options_.thread_count; ++i) {
    threads.emplace_back([=] { context_.run(); });
  }
  for (auto& thread : threads) {
    thread.join();
  }
  std::cout << "SIMULATION END!" << std::endl;
}

namespace {
TemperatureType FromString(const std::string& string) {
  if (string == "hot") {
    return TemperatureType::HOT;
  }
  if (string == "cold") {
    return TemperatureType::COLD;
  }
  if (string == "frozen") {
    return TemperatureType::FROZEN;
  }
  return TemperatureType::UNKNOWN;
}
}  // namespace

void KitchenSimulation::RunFromJson(const std::string& json_path) {
  std::ifstream ifs(json_path);
  if (!ifs.good()) {
    throw std::invalid_argument(
        absl::StrCat("Could not read JSON orders from path: ", json_path));
  }
  nlohmann::json json;
  ifs >> json;

  std::vector<std::unique_ptr<Order>> orders;
  for (const auto& kv : json.items()) {
    const auto& val = kv.value();
    try {
      orders.push_back(Order::CreateOrder(
          val["id"].get<std::string>(), val["name"].get<std::string>(),
          FromString(val["temp"].get<std::string>()),
          val["shelfLife"].get<int>(), val["decayRate"].get<double>(),
          absl::Now()));
    } catch (const std::invalid_argument& error) {
      if (!options_.continue_after_invalid_order) {
        throw error;
      }
    }
  }
  Run(orders.begin(), orders.end());
}

}  // namespace kitchen_sim