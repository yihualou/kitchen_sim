#ifndef KITCHEN_SIM_BASE_H_
#define KITCHEN_SIM_BASE_H_

#include "boost/asio.hpp"
#include "boost/fiber/all.hpp"

namespace fibers = boost::fibers;

template <typename T>
T WaitAndGet(fibers::future<T> future) {
  future.wait();
  return future.get();
}

#endif  // KITCHEN_SIM_BASE_H_