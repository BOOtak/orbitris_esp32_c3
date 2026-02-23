#include "persist.h"

#include <Preferences.h>

#include "trace.h"

constexpr auto PREFERENCES_GAME = "game";
constexpr auto KEY_STATS = "stats";
constexpr auto KEY_HIGHSCORES = "highscores";

static Preferences preferences;

template<typename T>
bool load_blob(const char* key_name, T& out) {
  if (!preferences.begin(PREFERENCES_GAME, true)) {
    trace("Unable to read preferences!\n");
    return false;
  }

  auto read_bytes = preferences.getBytes(key_name, &out, sizeof(out));
  if (read_bytes != sizeof(out)) {
    trace("%s read mismatch: Size %zu, got %zu!\n", key_name, sizeof(out), read_bytes);
    preferences.end();
    return false;
  }

  preferences.end();
  return true;
}

template<typename T>
bool persist_blob(const char* key_name, const T& blob) {
  if (!preferences.begin(PREFERENCES_GAME, false)) {
    trace("Unable to open preferences for writing!\n");
    return false;
  } else {
    trace("Prefs open for writing OK!\n");
  }

  auto written_bytes = preferences.putBytes(key_name, &blob, sizeof(blob));
  if (written_bytes != sizeof(blob)) {
    trace("%s write size mismatch: Size %zu, written %zu\n", key_name, sizeof(blob), written_bytes);
    preferences.end();
    return false;
  }

  preferences.end();
  return true;
}

bool load_highscores(HighscoreTable &out) {
  return load_blob(KEY_HIGHSCORES, out);
}

bool persist_highscores(const HighscoreTable &highscores) {
  return persist_blob(KEY_HIGHSCORES, highscores);
}

bool load_stats(Stats &out) {
  return load_blob(KEY_STATS, out);
}

bool persist_stats(const Stats &stats) {
  return persist_blob(KEY_STATS, stats);
}
