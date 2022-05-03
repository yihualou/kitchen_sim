#include "model/kitchen.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace kitchen_sim {

Kitchen BarebonesKitchen(boost::asio::io_context& context) {
  return Kitchen({"test",
                  1,
                  {{TemperatureType::HOT, 1},
                   {TemperatureType::COLD, 1},
                   {TemperatureType::FROZEN, 1}}},
                 context);
}

TEST(KitchenTest, TakeOrderUnknownTemperatureType) {
  boost::asio::io_context context;
  Kitchen kitchen({"test"}, context);
  EXPECT_THROW(Order::CreateOrder("1", "antimatter", TemperatureType::UNKNOWN,
                                  300, 0.45, absl::UnixEpoch()),
               std::invalid_argument);
}

TEST(KitchenTest, TakeOrderFulfillmentTimeRecorded) {
  boost::asio::io_context context;
  Kitchen kitchen({"test"}, context);
  auto future = kitchen.TakeOrder(
      Order::CreateOrder("1", "ice cream", TemperatureType::COLD, 300, 0.5,
                         absl::UnixEpoch()),
      absl::UnixEpoch());
  auto order = future.get();
  EXPECT_EQ(order->FulfillmentTime().value(), absl::UnixEpoch());
}

TEST(KitchenTest, TakeOrderOverflowUsed) {
  boost::asio::io_context context;
  Kitchen kitchen = BarebonesKitchen(context);
  auto tea = Order::CreateOrder("1", "tea", TemperatureType::COLD, 300, 0.5,
                                absl::UnixEpoch());
  auto soda = Order::CreateOrder("2", "soda", TemperatureType::COLD, 300, 0.5,
                                 absl::UnixEpoch());

  const Order* overflow = soda.get();
  kitchen.TakeOrder(std::move(tea)).wait();
  kitchen.TakeOrder(std::move(soda)).wait();

  EXPECT_THAT(kitchen.OverflowShelf().Orders(), testing::ElementsAre(overflow));
}

TEST(KitchenTest, TakeOrderOverflowRoomMade) {
  boost::asio::io_context context;
  Kitchen kitchen = BarebonesKitchen(context);
  auto tea = Order::CreateOrder("1", "tea", TemperatureType::COLD, 300, 0.5,
                                absl::UnixEpoch());
  auto soda = Order::CreateOrder("2", "soda", TemperatureType::COLD, 300, 0.5,
                                 absl::UnixEpoch());

  auto burger = Order::CreateOrder("3", "burger", TemperatureType::HOT, 300,
                                   0.5, absl::UnixEpoch());
  auto pizza = Order::CreateOrder("4", "pizza", TemperatureType::HOT, 300, 0.5,
                                  absl::UnixEpoch());

  const std::string tea_id = tea->id_;
  const Order* burger_ptr = burger.get();
  const Order* pizza_ptr = pizza.get();
  kitchen.TakeOrder(std::move(tea)).wait();
  kitchen.TakeOrder(std::move(soda)).wait();  // Moved to overflow.
  kitchen.PickupOrder(tea_id);
  kitchen.TakeOrder(std::move(burger)).wait();
  kitchen.TakeOrder(std::move(pizza)).wait();  // Soda gets moved back.

  EXPECT_THAT(kitchen.TemperatureShelf(TemperatureType::HOT).Orders(),
              testing::ElementsAre(burger_ptr));
  EXPECT_THAT(kitchen.OverflowShelf().Orders(),
              testing::ElementsAre(pizza_ptr));
}

TEST(KitchenTest, TakeOrderOverflowDiscarded) {
  boost::asio::io_context context;
  Kitchen kitchen = BarebonesKitchen(context);
  auto tea = Order::CreateOrder("1", "tea", TemperatureType::COLD, 300, 0.5,
                                absl::UnixEpoch());
  auto soda = Order::CreateOrder("2", "soda", TemperatureType::COLD, 300, 0.5,
                                 absl::UnixEpoch());
  auto juice = Order::CreateOrder("3", "juice", TemperatureType::COLD, 300, 0.5,
                                  absl::UnixEpoch());

  const Order* tea_ptr = tea.get();
  const Order* juice_ptr = juice.get();
  kitchen.TakeOrder(std::move(tea)).wait();
  kitchen.TakeOrder(std::move(soda)).wait();
  kitchen.TakeOrder(std::move(juice)).wait();  // Removes soda.

  EXPECT_THAT(kitchen.TemperatureShelf(TemperatureType::COLD).Orders(),
              testing::ElementsAre(tea_ptr));
  EXPECT_THAT(kitchen.OverflowShelf().Orders(),
              testing::ElementsAre(juice_ptr));
}

TEST(KitchenTest, OrderValueExpired) {
  boost::asio::io_context context;
  Kitchen kitchen({"test"}, context);
  kitchen.TakeOrder(
      Order::CreateOrder("1", "ice cream", TemperatureType::FROZEN, 300, 0.5,
                         absl::UnixEpoch()),
      absl::UnixEpoch());

  // 50+ years is a long time to be sitting on a shelf.
  EXPECT_DOUBLE_EQ(kitchen.OrderValue("1"), 0.);
}

TEST(KitchenTest, OrderValueOverflow) {
  boost::asio::io_context context;
  Kitchen kitchen = BarebonesKitchen(context);
  kitchen.TakeOrder(
      Order::CreateOrder("1", "ice cream", TemperatureType::FROZEN, 300, 1,
                         absl::UnixEpoch()),
      absl::UnixEpoch());
  kitchen.TakeOrder(Order::CreateOrder("2", "cannoli", TemperatureType::FROZEN,
                                       300, 1, absl::UnixEpoch()),
                    absl::UnixEpoch());

  // (shelfLife - (decayRate * orderAge * shelfDecayModifier)) / shelfLife
  EXPECT_DOUBLE_EQ(
      kitchen.OrderValue("1", absl::UnixEpoch() + absl::Seconds(100)),
      (300 - 1 * 100 * 1) / 300.);
  EXPECT_DOUBLE_EQ(
      kitchen.OrderValue("2", absl::UnixEpoch() + absl::Seconds(100)),
      (300 - 2 * 100 * 1) / 300.);
}

}  // namespace kitchen_sim