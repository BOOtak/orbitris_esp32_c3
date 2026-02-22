#include "persist.h"

#include <Preferences.h>

#include "trace.h"

constexpr auto PREFERENCES_GAME = "game";
constexpr auto KEY_STATS = "stats";

static Preferences preferences;

bool load_stats(Stats &out) {
  if (!preferences.begin(PREFERENCES_GAME, true)) {
    trace("Unable to read preferences!\n");
    return false;
  }

  auto read_bytes = preferences.getBytes(KEY_STATS, &out, sizeof(out));
  if (read_bytes != sizeof(out)) {
    trace("Stats read mismatch: Size %zu, got %zu!\n", sizeof(out), read_bytes);
    preferences.end();
    return false;
  }

  preferences.end();
  return true;
}

bool persist_stats(const Stats &stats) {
  if (!preferences.begin(PREFERENCES_GAME, false)) {
    trace("Unable to open preferences for writing!\n");
    return false;
  } else {
    trace("Prefs open for writing OK!\n");
  }

  auto written_bytes = preferences.putBytes(KEY_STATS, &stats, sizeof(stats));
  if (written_bytes != sizeof(stats)) {
    trace("Write size mismatch: Size %zu, written %zu\n", sizeof(stats), written_bytes);
    preferences.end();
    return false;
  }

  preferences.end();
  return true;
}
