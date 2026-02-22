#include "../orbitris_esp32/persist.h"

#include <fstream>

constexpr auto stats_location = "stats.dat";

bool load_stats(Stats &out) {
  std::ifstream in(stats_location, std::ios::in | std::ios::binary);
  if (!in.good()) {
    return false;
  }

  in.read(reinterpret_cast<char *>(&out), sizeof(out));
  if (in.gcount() != sizeof(out)) {
    return false;
  }

  return true;
}

bool persist_stats(const Stats &stats) {
  std::ofstream out(stats_location, std::ios::out | std::ios::binary | std::ios::trunc);
  if (!out.good()) {
    return false;
  }

  out.write(reinterpret_cast<const char *>(&stats), sizeof(stats));

  return true;
}
