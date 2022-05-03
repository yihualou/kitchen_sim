#include "model/order.h"

#include "gtest/gtest.h"

namespace kitchen_sim {

std::unique_ptr<Order> DefaultOrder(int shelf_life_s = 300,
                                    double decay_rate = 0.45) {
  return Order::CreateOrder("1", "cheese pizza", TemperatureType::HOT,
                            shelf_life_s, decay_rate, absl::UnixEpoch());
}

TEST(OrderTest, InvalidArguments) {
  // id
  EXPECT_THROW(Order::CreateOrder("", "cheese pizza", TemperatureType::HOT, 300,
                                  0.45, absl::UnixEpoch()),
               std::invalid_argument);

  // name
  EXPECT_THROW(Order::CreateOrder("1", "", TemperatureType::HOT, 300, 0.45,
                                  absl::UnixEpoch()),
               std::invalid_argument);

  // shelf_life_s
  EXPECT_THROW(Order::CreateOrder("1", "cheese pizza", TemperatureType::HOT,
                                  -300, 0.45, absl::UnixEpoch()),
               std::invalid_argument);
}

TEST(OrderTest, ValueBeforeCooking) {
  auto order = DefaultOrder();
  // No value until order cooked.
  EXPECT_EQ(order->Value(1, absl::UnixEpoch() + absl::Seconds(600)), 0.0);
}

TEST(OrderTest, ValueUndecaying) {
  auto order = DefaultOrder(300, 0.0);
  order->SetFulfillmentTime(absl::UnixEpoch());
  // Should always be 1 until shelf life expires.
  EXPECT_EQ(order->Value(1, absl::UnixEpoch() + absl::Seconds(300)), 1.0);
  EXPECT_EQ(order->Value(1, absl::UnixEpoch() + absl::Seconds(301)), 0.0);
}

TEST(OrderTest, ValueFuture) {
  auto order = DefaultOrder(500, 1.0);
  order->SetFulfillmentTime(absl::UnixEpoch());

  EXPECT_EQ(order->Value(1, absl::UnixEpoch()), 1.0);
  // Since both modifiers are 1, value = 1 - <elapsed seconds>/<shelf life>
  EXPECT_EQ(order->Value(1, absl::UnixEpoch() + absl::Seconds(200)), 0.6);
}

TEST(OrderTest, ValueMovedToOverflow) {
  auto order = DefaultOrder(500, 1.0);
  order->SetFulfillmentTime(absl::UnixEpoch());

  EXPECT_EQ(order->Value(1, absl::UnixEpoch() + absl::Seconds(200)), 0.6);
  order->MoveFrom(1, absl::UnixEpoch() + absl::Seconds(200));

  // Deteriates faster now.
  EXPECT_EQ(order->Value(2, absl::UnixEpoch() + absl::Seconds(200 + 150)), 0.0);
}

TEST(OrderTest, ValueMovedFromOverflow) {
  auto order = DefaultOrder(500, 1.0);
  order->SetFulfillmentTime(absl::UnixEpoch());

  EXPECT_EQ(order->Value(2, absl::UnixEpoch() + absl::Seconds(100)), 0.6);
  order->MoveFrom(2, absl::UnixEpoch() + absl::Seconds(100));

  // Deteriates slower now.
  EXPECT_EQ(order->Value(1, absl::UnixEpoch() + absl::Seconds(100 + 300)), 0.0);
}

TEST(OrderTest, ValueFarFuture) {
  auto order = DefaultOrder();
  order->SetFulfillmentTime(absl::UnixEpoch());
  // Clips at 0.
  EXPECT_EQ(order->Value(1, absl::UnixEpoch() + absl::Seconds(9001)), 0.0);
}

}  // namespace kitchen_sim