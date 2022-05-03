#include <fstream>
#include <iostream>

#include "gflags/gflags.h"
#include "kitchen_sim_lib.h"

DEFINE_string(json_path, "",
              "Path to a JSON file containing serialized orders.");

DEFINE_string(kitchen_name, "Din Tai Fung", "Name of the simulated kitchen.");
DEFINE_string(kitchen_size, "LARGE", "Size of the simulated kitchen.");
DEFINE_double(orders_per_second, 2., "Parsing/handling rate for orders.");
DEFINE_bool(continue_after_invalid_order, true,
            "Whether to continue simulation after an order parsing failure.");

static bool FileExists(const char* flagname, const std::string& value) {
  std::ifstream ifs(value.c_str());
  return ifs.good();
}
DEFINE_validator(json_path, &FileExists);

static bool IsValidSize(const char* flagname, const std::string& value) {
  return value == "SMALL" || value == "LARGE";
}
DEFINE_validator(kitchen_size, &IsValidSize);

static bool IsPositive(const char* flagname, double value) { return value > 0; }
DEFINE_validator(orders_per_second, &IsPositive);

int main(int argc, char* argv[]) {
  gflags::SetUsageMessage(
      "kitchen_sim --json_path=<path> [ --kitchen_name='Din Tai Fung' "
      "--kitchen_size='SMALL' --orders_per_second=10 ]");
  gflags::SetVersionString("1.0.0");
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  kitchen_sim::KitchenSimulation simulation(
      {FLAGS_kitchen_name, FLAGS_kitchen_size, FLAGS_orders_per_second,
       FLAGS_continue_after_invalid_order});
  try {
    simulation.RunFromJson(FLAGS_json_path);
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }
}