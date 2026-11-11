#include "house_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "cat.h"
#include "const.h"
#include "draw.h"
#include "input.h"
#include "light.h"
#include "target.h"
#include "trace.h"
#include "world.h"

namespace {

constexpr auto BLOCK_SIZE_PX = BLOCK_SIZE * CELL_BLOCKS;

constexpr float PLAYER_RADIUS = 16.0f;
constexpr float PLAYER_COLLISION_RADIUS = PLAYER_RADIUS;
constexpr auto PLAYER_START = Vector2{ 3, 11 } * BLOCK_SIZE_PX + PLAYER_RADIUS;

constexpr float MOVE_SPEED = 1.8f;
constexpr float RUN_MULTIPLIER = 2.0f;

constexpr float MESS_RUG = 15.0f;
constexpr float MESS_BED = 30.0f;
constexpr float PEE_FILL_RUG = 0.5f;
constexpr float PEE_FILL_BED = 0.5f;

// Passable pee targets: white interiors, black outlines.
constexpr auto BED_RECT = Rectangle{ 4, 12, 2, 2 } * BLOCK_SIZE_PX;
constexpr auto RUG_A_RECT = Rectangle{ 12, 10, 1, 1 } * BLOCK_SIZE_PX;
constexpr auto RUG_B_RECT = Rectangle{ 12, 12, 1, 1 } * BLOCK_SIZE_PX;
constexpr auto BOX = Rectangle{ 7, 8, 1, 1 } * BLOCK_SIZE_PX;

PeeTarget targets[] = {
  { BED_RECT, 4, 12, 0.0f, PEE_FILL_BED, MESS_BED },
  { RUG_A_RECT, 12, 10, 0.0f, PEE_FILL_RUG, MESS_RUG },
  { RUG_B_RECT, 12, 12, 0.0f, PEE_FILL_RUG, MESS_RUG },
};

constexpr int TARGET_COUNT = std::size(targets);

struct CatSpawn {
  int col;
  int row;
};

constexpr CatSpawn CAT_SPAWNS[] = {
  { 1, 5 },
  { 2, 2 },
  { 11, 1 },
  { 12, 7 },
};

constexpr int CAT_COUNT = std::size(CAT_SPAWNS);

Cat cats[CAT_COUNT];

int screen_x(float world_x, int camera_x) {
  return static_cast<int>(std::lround(world_x - camera_x));
}

int screen_y(float world_y, int camera_y) {
  return static_cast<int>(std::lround(world_y - camera_y));
}

void draw_target(const Rectangle& rect, int camera_x, int camera_y) {
  draw_rectangle_lines(screen_x(rect.x, camera_x), screen_y(rect.y, camera_y),
                       static_cast<int>(std::lround(rect.width)),
                       static_cast<int>(std::lround(rect.height)), LCD_BLACK);
}

void draw_target_fill(const Rectangle& rect, float fill, int camera_x, int camera_y) {
  if (fill <= 0.0f) {
    return;
  }

  const int x0 = screen_x(rect.x + 1.0f, camera_x);
  const int y0 = screen_y(rect.y + 1.0f, camera_y);
  const int x1 = screen_x(rect.x + rect.width - 2.0f, camera_x);
  const int y1 = screen_y(rect.y + rect.height - 2.0f, camera_y);
  const float threshold = fill * 16.0f;

  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      if (threshold > static_cast<float>(bayer_value(x, y)) + 0.5f) {
        draw_pixel(x, y, LCD_BLACK);
      }
    }
  }
}

void draw_mess_hud(float mess) {
  draw_rectangle(6, 6, 72, 8, LCD_BLACK);
  const int width = static_cast<int>((mess / 100.0f) * 68.0f);
  if (width > 0) {
    draw_rectangle(8, 8, width, 4, LCD_WHITE);
  }
  draw_rectangle_lines(6, 6, 72, 8, LCD_WHITE);
}

void draw_lose_overlay() {
  draw_rectangle(80, 88, 240, 64, LCD_BLACK);
  draw_rectangle_lines(80, 88, 240, 64, LCD_WHITE);

  const char* title = "HOUSE IS A MESS!";
  const Vector2 title_size = measure_text(title, 2);
  print_text(200 - static_cast<int>(title_size.x / 2.0f), 98, 2, title, LCD_WHITE);

  const char* subtitle = "YOU LOSE";
  const Vector2 subtitle_size = measure_text(subtitle, 2);
  print_text(200 - static_cast<int>(subtitle_size.x / 2.0f), 124, 2, subtitle, LCD_WHITE);
}

}  // namespace

HouseScreen::HouseScreen()
  : Screen(), player_pos_{ PLAYER_START } {}

void HouseScreen::init() {
  player_pos_ = PLAYER_START;
  mess_ = 0.0f;
  lost_ = false;

  targets[0] = { BED_RECT, 4, 12, 0.0f, PEE_FILL_BED, MESS_BED };
  targets[1] = { RUG_A_RECT, 12, 10, 0.0f, PEE_FILL_RUG, MESS_RUG };
  targets[2] = { RUG_B_RECT, 12, 12, 0.0f, PEE_FILL_RUG, MESS_RUG };

  std::srand(platform_micros());
  for (int i = 0; i < CAT_COUNT; ++i) {
    cat_init(cats[i], CAT_SPAWNS[i].col, CAT_SPAWNS[i].row);
  }
  update_camera();
}

void HouseScreen::update_camera() {
  camera_pos_.x = std::clamp(player_pos_.x - LCD_WIDTH / 2.0f, 0.0f,
                             static_cast<float>(MAX_CAMERA_X));
  camera_pos_.y = std::clamp(player_pos_.y - LCD_HEIGHT / 2.0f, 0.0f,
                             static_cast<float>(MAX_CAMERA_Y));
}

Screen* HouseScreen::update() {
  if (lost_) {
    return this;
  }

  Vector2 dir{};
  if (is_key_down(ESP_KEY_LEFT)) {
    dir.x -= 1.0f;
  }
  if (is_key_down(ESP_KEY_RIGHT)) {
    dir.x += 1.0f;
  }
  if (is_key_down(ESP_KEY_UP)) {
    dir.y -= 1.0f;
  }
  if (is_key_down(ESP_KEY_DOWN)) {
    dir.y += 1.0f;
  }

  if (dir.x != 0.0f || dir.y != 0.0f) {
    const float speed = is_key_down(ESP_KEY_B) ? MOVE_SPEED * RUN_MULTIPLIER : MOVE_SPEED;
    const float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    const float step = speed / length;
    Vector2 cat_centers[CAT_COUNT];
    float cat_radii[CAT_COUNT];
    for (int i = 0; i < CAT_COUNT; ++i) {
      cat_centers[i] = cats[i].pos;
      cat_radii[i] = CAT_RADIUS;
    }
    move_with_collision_circles(player_pos_, { dir.x * step, dir.y * step },
                                PLAYER_COLLISION_RADIUS, cat_centers, cat_radii, CAT_COUNT);
  }

  for (int i = 0; i < CAT_COUNT; ++i) {
    cat_update(cats[i], i, cats, CAT_COUNT, player_pos_, PLAYER_COLLISION_RADIUS, targets,
               TARGET_COUNT, &mess_);
  }

  if (mess_ >= 100.0f) {
    mess_ = 100.0f;
    lost_ = true;
  }

  update_camera();

  return this;
}

void HouseScreen::draw() const {
  fill_screen_buffer(LCD_WHITE);

  const int camera_x = static_cast<int>(std::lround(camera_pos_.x));
  const int camera_y = static_cast<int>(std::lround(camera_pos_.y));
  const Vector2 player_screen{ player_pos_.x - camera_x, player_pos_.y - camera_y };

  // Scene: white interiors stay as background; only black fills/outlines and
  // solids are drawn. The darkness pass hides whatever is not lit.
  for (const auto& target : targets) {
    draw_target_fill(target.rect, target.fill, camera_x, camera_y);
    draw_target(target.rect, camera_x, camera_y);
  }

  draw_target(BOX, camera_x, camera_y);
  draw_rectangle_lines(screen_x(BOX.x + 4.0f, camera_x), screen_y(BOX.y + 4.0f, camera_y),
                       static_cast<int>(std::lround(BOX.width - 8.0f)),
                       static_cast<int>(std::lround(BOX.height - 8.0f)), LCD_BLACK);

  for (const auto& cat : cats) {
    draw_circle_lines(screen_x(cat.pos.x, camera_x), screen_y(cat.pos.y, camera_y),
                      static_cast<int>(std::lround(CAT_RADIUS)), LCD_BLACK);
  }

  draw_filled_circle(static_cast<int>(std::lround(player_screen.x)),
                     static_cast<int>(std::lround(player_screen.y)),
                     static_cast<int>(PLAYER_RADIUS), LCD_BLACK);

  static ScreenPoint vertices[LIGHT_RAYS];
  build_light_polygon(player_pos_, camera_x, camera_y, vertices);
  draw_darkness_polygon(vertices, LIGHT_RAYS, to_subpixel(player_screen.x),
                        to_subpixel(player_screen.y));

  // HUD and end-state overlay stay visible in the dark.
  draw_mess_hud(mess_);
  if (lost_) {
    draw_lose_overlay();
  }
}

void HouseScreen::close() {
}
