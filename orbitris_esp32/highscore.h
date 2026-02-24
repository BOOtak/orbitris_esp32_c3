#pragma once

#include <array>
#include <cstdint>
#include <cstring>

constexpr auto HIGHSCORE_NAME_SIZE = 16;
constexpr auto HIGHSCORE_ENTRIES = 8;

#pragma pack(push, 1)
struct HighscoreEntry {
  uint8_t name_index;
  uint32_t score;
};
#pragma pack(pop)

struct HighscoreTable {
  static constexpr auto NO_LAST_NAME_INDEX = (uint8_t)-1;

  std::array<std::array<char, HIGHSCORE_NAME_SIZE>, HIGHSCORE_ENTRIES> names;
  std::array<HighscoreEntry, HIGHSCORE_ENTRIES> data;
  uint8_t last_name_index{ NO_LAST_NAME_INDEX };

  inline bool is_highscore(uint32_t score) const {
    if (score == 0) {
      return false;
    }

    for (const auto& entry : data) {
      if (entry.score <= score) {
        return true;
      }
    }

    return false;
  }

  inline bool add_highscore(const char* name, uint32_t score) {
    if (score == 0) {
      return false;
    }

    if (name == nullptr) {
      return false;
    }

    size_t insert_idx = 0;
    for (insert_idx = 0; insert_idx < HIGHSCORE_ENTRIES; insert_idx++) {
      if (data[insert_idx].score <= score) {
        break;
      }
    }

    if (insert_idx == HIGHSCORE_ENTRIES) {
      return false;
    }

    size_t name_index = 0;
    bool is_valid_name_index = false;
    if (data[HIGHSCORE_ENTRIES - 1].score > 0) {
      // If table is full, reuse name index of the removed entry
      name_index = data[HIGHSCORE_ENTRIES - 1].name_index;
      is_valid_name_index = true;
    } else {
      // Find free name slot
      for (name_index = 0; name_index < HIGHSCORE_ENTRIES; name_index++) {
        if (names[name_index][0] == '\0') {
          is_valid_name_index = true;
          break;
        }
      }
    }

    if (!is_valid_name_index) {
      return false;
    }

    for (size_t i = HIGHSCORE_ENTRIES - 1; i > insert_idx; --i) {
      data[i].score = data[i - 1].score;
      data[i].name_index = data[i - 1].name_index;
    }

    std::strncpy(names[name_index].data(), name, HIGHSCORE_NAME_SIZE - 1);
    names[name_index][HIGHSCORE_NAME_SIZE - 1] = '\0';
    data[insert_idx].name_index = (uint8_t)name_index;
    data[insert_idx].score = score;

    last_name_index = name_index;

    return true;
  }
};
