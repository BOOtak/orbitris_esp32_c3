#include "../orbitris_esp32/persist.h"

#include <fstream>

constexpr auto stats_location = "stats.dat";
constexpr auto highscore_location = "highscores.dat";

template<typename T>
static bool load_blob(const char *filepath, T &out) {
  std::ifstream in(filepath, std::ios::in | std::ios::binary);
  if (!in.good()) {
    return false;
  }

  in.read(reinterpret_cast<char *>(&out), sizeof(out));
  if (in.gcount() != sizeof(out)) {
    return false;
  }

  return true;
}

template<typename T>
static bool persist_blob(const char *filepath, const T &data) {
  std::ofstream out(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
  if (!out.good()) {
    return false;
  }

  out.write(reinterpret_cast<const char *>(&data), sizeof(data));

  return true;
}

bool load_highscores(HighscoreTable &out) {
  return load_blob(highscore_location, out);
}

bool persist_highscores(const HighscoreTable &highscores) {
  return persist_blob(highscore_location, highscores);
}

bool load_stats(Stats &out) {
  return load_blob(stats_location, out);
}

bool persist_stats(const Stats &stats) {
  return persist_blob(stats_location, stats);
}
