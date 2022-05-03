#include "model/kitchen.h"

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "boost/log/trivial.hpp"

namespace kitchen_sim {
namespace {

constexpr absl::string_view kExpiryScheduled = "EXPIRY_SCHEDULED";
constexpr absl::string_view kExpired = "EXPIRED";
constexpr absl::string_view kDiscarded = "DISCARDED";

}  // namespace

Kitchen::Kitchen(const Options options, boost::asio::io_context& context)
    : options_(options),
      overflow_shelf_(options_.overflow_capacity, 1),
      rand_(std::random_device{}()),
      context_(context),
      strand_(context_) {
  for (const auto& pair : options_.temp_to_capacity) {
    shelves_[pair.first] = std::make_unique<Shelf>(pair.second, 2);
  }
}

fibers::future<Order*> Kitchen::TakeOrder(std::unique_ptr<Order> order,
                                          absl::Time at_time) {
  fibers::promise<Order*> fulfilled_order;
  auto it = shelves_.find(order->temp_);
  if (it == shelves_.end()) {
    throw std::invalid_argument(
        absl::StrCat("Could not find shelf for kitchen: ", options_.name,
                     " temperature group: ", order->temp_));
  }
  order->SetFulfillmentTime(at_time);
  if (PlaceOrder(order.get(), it->second.get())) {
    // Try placing on matching temperature shelf first.
    fulfilled_order.set_value(order.get());
  } else {
    // What about the overflow shelf?
    if (!PlaceOrder(order.get(), &overflow_shelf_)) {
      MakeOverflowRoom();
      PlaceOrder(order.get(), &overflow_shelf_);
    }
    fulfilled_order.set_value(order.get());
  }

  // Schedule expiration timer.
  // Assuming a fixed expiry avoids handler cancel churn.
  absl::Time expiry = order->Expiry(1);
  BOOST_LOG_TRIVIAL(info) << order->LogMessage(kExpiryScheduled) << " @ "
                          << absl::FormatTime(
                                 "%H:%M:%S", expiry,
                                 absl::FixedTimeZone(
                                     -7 * 60 * 60));  // It's always LA time.

  order->expiration_timer_ =
      std::make_unique<boost::asio::system_timer>(context_);
  order->expiration_timer_->expires_at(absl::ToChronoTime(expiry));
  order->expiration_timer_->async_wait(boost::asio::bind_executor(
      strand_, [=, order_id = order->id_](const boost::system::error_code& e) {
        auto expired_order = PickupOrder(order_id);
        if (expired_order != nullptr) {
          BOOST_LOG_TRIVIAL(debug) << expired_order->LogMessage(kExpired);
          LogShelves();
        } else {
          // Already picked up or discarded.
        }
      }));

  orders_[order->id_] = std::move(order);
  return fulfilled_order.get_future();
}

std::unique_ptr<Order> Kitchen::PickupOrder(absl::string_view order_id,
                                            absl::Time at_time) {
  auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return nullptr;
  }
  if (OrderValue(order_id, at_time) <= 0.) {
    // Let expiry handler clean up.
    return nullptr;
  }
  std::unique_ptr<Order> order = std::move(it->second);
  order_to_shelf_[order_id]->RemoveOrder(order.get());
  order_to_shelf_.erase(order_id);
  orders_.erase(order_id);
  return order;
}

double Kitchen::OrderValue(absl::string_view id, absl::Time at_time) const {
  auto it = orders_.find(id);
  if (it == orders_.end()) {
    return 0.;
  }
  auto shelf_it = order_to_shelf_.find(id);
  if (shelf_it == order_to_shelf_.end()) {
    return 0.;
  }
  int decay_modifier = shelf_it->second == &overflow_shelf_ ? 2 : 1;
  return it->second->Value(decay_modifier, at_time);
}

namespace {
std::string PrintTemperatureType(TemperatureType temp) {
  switch (temp) {
    case TemperatureType::HOT:
      return "HOT";
    case TemperatureType::COLD:
      return "COLD";
    case TemperatureType::FROZEN:
      return "FROZEN";
    default:
      return "UNKNOWN";
  }
}

std::string LogMessageForShelf(const Kitchen::Shelf& shelf) {
  std::string message;
  message.append("[")
      .append(absl::StrJoin(
          shelf.Orders(), ", ",
          [&](std::string* out, const auto* order) {
            out->append(order->name_)
                .append(": ")
                .append(absl::StrCat(order->Value(shelf.DecayModifier())));
          }))
      .append("]");
  return message;
}
}  // namespace

void Kitchen::LogShelves() const {
  for (const auto& pair : shelves_) {
    std::string shelf_message;
    shelf_message.append("shelf: ")
        .append(PrintTemperatureType(pair.first))
        .append(" ")
        .append(LogMessageForShelf(*pair.second));
    BOOST_LOG_TRIVIAL(info) << shelf_message;
  }
  std::string overflow_message;
  overflow_message.append("shelf: OVERFLOW ")
      .append(LogMessageForShelf(overflow_shelf_));
  BOOST_LOG_TRIVIAL(info) << overflow_message;
}

bool Kitchen::PlaceOrder(Order* order, Shelf* shelf) {
  bool added = shelf->AddOrder(order);
  if (added) {
    order_to_shelf_[order->id_] = shelf;
  }
  return added;
}

void Kitchen::MakeOverflowRoom() {
  std::unordered_set<TemperatureType> open_shelves;
  for (auto& pair : shelves_) {
    if (!pair.second->AtCapacity()) {
      open_shelves.insert(pair.first);
    }
  }

  std::vector<const Order*> overflow_orders;
  for (const Order* overflow_order : overflow_shelf_.Orders()) {
    const TemperatureType temp = overflow_order->temp_;
    if (open_shelves.find(temp) != open_shelves.end()) {
      // Move overflow order to temperature shelf.
      auto* order = orders_[overflow_order->id_].get();
      overflow_shelf_.RemoveOrder(order);
      order->MoveFrom(overflow_shelf_.DecayModifier());
      PlaceOrder(order, shelves_[temp].get());
      return;
    }
    overflow_orders.push_back(overflow_order);
  }
  // Discard random order
  std::uniform_int_distribution<int> dist(0, overflow_orders.size() - 1);
  const Order* discarded = overflow_orders[dist(rand_)];
  BOOST_LOG_TRIVIAL(info) << discarded->LogMessage(kDiscarded);
  overflow_shelf_.RemoveOrder(discarded);
}

}  // namespace kitchen_sim