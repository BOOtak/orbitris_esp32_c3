#include "highscore_screen.h"

#include <cstdio>

#include "const.h"
#include "draw.h"
#include "input.h"
#include "screen.h"

HighscoreTableScreen::HighscoreTableScreen(HighscoreTable &highscores) : Screen(), highscores_{ highscores } {
}

void HighscoreTableScreen::init() {
}

Screen *HighscoreTableScreen::update() {
  if (is_key_pressed(ESP_KEY_A)) {
    return screens::menu_screen;
  }

  return this;
}

void HighscoreTableScreen::draw() const {
  fill_screen_buffer(1);

  constexpr auto title_text = "HIGHSCORES";
  constexpr auto title_size = measure_text(title_text, 3);
  constexpr int title_x = LCD_WIDTH / 2 - title_size.x / 2;
  constexpr int title_y = 5;

  print_text(title_x, title_y, 3, title_text, 0);

  constexpr auto rank_header = "RANK";
  constexpr auto name_header = "NAME";
  constexpr auto score_header = "SCORE";

  constexpr auto rank_header_size = measure_text(rank_header, 2);
  constexpr auto name_header_size = measure_text(name_header, 2);
  constexpr auto score_header_size = measure_text(score_header, 2);

  constexpr int headers_y = title_y + title_size.y + 10;
  constexpr int rank_x = 20;
  print_text(rank_x, headers_y, 2, rank_header, 0);

  constexpr int name_x = rank_x + rank_header_size.x + 30;
  print_text(name_x, headers_y, 2, name_header, 0);

  constexpr int score_x = LCD_WIDTH - 20 - score_header_size.x;
  print_text(score_x, headers_y, 2, score_header, 0);

  constexpr int table_start_x = rank_x - 5;
  constexpr int table_width = (score_x + score_header_size.x) - (rank_x - 5) + 5;

  constexpr int separator_y = headers_y + rank_header_size.y + 5;
  draw_hline(table_start_x, separator_y, table_width, 0);

  constexpr int entries_start_y = separator_y + 10;
  constexpr int line_height = 20;

  char buffer[32];

  if (highscores_.data[0].score == 0) {
    constexpr auto no_scores = "NO SCORES YET";
    constexpr auto no_scores_size = measure_text(no_scores, 2);
    print_text(LCD_WIDTH / 2 - no_scores_size.x / 2,
               LCD_HEIGHT / 2 - no_scores_size.y / 2,
               2, no_scores, 0);
    return;
  }

  for (int i = 0; i < HIGHSCORE_ENTRIES; i++) {
    if (highscores_.data[i].score == 0) {
      continue;
    }

    const int y_pos = entries_start_y + (i * line_height);

    snprintf(buffer, sizeof(buffer), "%d", i + 1);
    constexpr auto max_rank_size = measure_text("8", 2);
    print_text(rank_x + rank_header_size.x - max_rank_size.x, y_pos, 2, buffer, 0);

    uint8_t name_idx = highscores_.data[i].name_index;
    const char *name = highscores_.names[name_idx].data();
    print_text(name_x, y_pos, 2, name, 0);

    int score = highscores_.data[i].score;

    if (score >= 1000) {
      snprintf(buffer, sizeof(buffer), "%d,%03d", score / 1000, score % 1000);
    } else {
      snprintf(buffer, sizeof(buffer), "%d", score);
    }

    const auto score_size = measure_text(buffer, 2);
    print_text(score_x + score_header_size.x - score_size.x, y_pos, 2, buffer, 0);
  }

  constexpr int last_entry_y = entries_start_y + (HIGHSCORE_ENTRIES * line_height) - line_height;
  draw_hline(table_start_x, last_entry_y + line_height, table_width, 0);
}
