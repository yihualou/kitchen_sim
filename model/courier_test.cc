#include "model/courier.h"

#include "gtest/gtest.h"

namespace kitchen_sim {

TEST(CourierTest, DeliveryTimeRecorded) {
  boost::asio::io_context context;
  Kitchen kitchen({"test"}, context);
  Courier courier;
  Order* cooked_order = WaitAndGet(kitchen.TakeOrder(
      Order::CreateOrder("1", "cheese pizza", TemperatureType::HOT, 300, 0.5,
                         absl::UnixEpoch()),
      absl::UnixEpoch()));
  courier.AcceptOrder({cooked_order->id_, &kitchen});
  auto delivered_order = WaitAndGet(
      courier.PickupCurrentOrder(absl::UnixEpoch() + absl::Seconds(60)));

  EXPECT_TRUE(delivered_order.get() != nullptr);
  EXPECT_EQ(delivered_order->DeliveryTime().value(),
            absl::UnixEpoch() + absl::Seconds(60));
}

}  // namespace kitchen_sim
