#include "house_screen.h"

#include <algorithm>
#include <cmath>

#include "const.h"
#include "draw.h"
#include "input.h"
#include "light.h"
#include "world.h"

namespace {

constexpr auto BLOCK_SIZE_PX = BLOCK_SIZE * CELL_BLOCKS;

constexpr float PLAYER_RADIUS = 16.0f;
constexpr float PLAYER_COLLISION_RADIUS = PLAYER_RADIUS;
constexpr auto PLAYER_START = Vector2{ 3, 11 } * BLOCK_SIZE_PX + PLAYER_RADIUS;

constexpr float MOVE_SPEED = 1.8f;
constexpr float RUN_MULTIPLIER = 2.0f;

constexpr float CAT_RADIUS = 10.0f;

// Passable pee targets: white interiors, black outlines.
constexpr auto BED = Rectangle{ 4, 12, 2, 2 } * BLOCK_SIZE_PX;
constexpr auto BOX = Rectangle{ 7, 8, 1, 1 } * BLOCK_SIZE_PX;

constexpr Rectangle RUGS[] = {
  Rectangle{ 12, 10, 1, 1 } * BLOCK_SIZE_PX,
  Rectangle{ 12, 12, 1, 1 } * BLOCK_SIZE_PX,
};

constexpr Vector2 CATS[] = {
  Vector2{ 1, 5 } * BLOCK_SIZE_PX + CAT_RADIUS,
  Vector2{ 2, 2 } * BLOCK_SIZE_PX + CAT_RADIUS,
  Vector2{ 11, 1 } * BLOCK_SIZE_PX + CAT_RADIUS,
  Vector2{ 12, 7 } * BLOCK_SIZE_PX + CAT_RADIUS,
};

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

}  // namespace

HouseScreen::HouseScreen()
  : Screen(), player_pos_{ PLAYER_START } {}

void HouseScreen::init() {
  player_pos_ = PLAYER_START;
  update_camera();
}

void HouseScreen::update_camera() {
  camera_pos_.x = std::clamp(player_pos_.x - LCD_WIDTH / 2.0f, 0.0f,
                             static_cast<float>(MAX_CAMERA_X));
  camera_pos_.y = std::clamp(player_pos_.y - LCD_HEIGHT / 2.0f, 0.0f,
                             static_cast<float>(MAX_CAMERA_Y));
}

Screen* HouseScreen::update() {
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
    move_with_collision(player_pos_, { dir.x * step, dir.y * step },
                        PLAYER_COLLISION_RADIUS);
  }

  update_camera();

  return this;
}

void HouseScreen::draw() const {
  fill_screen_buffer(LCD_WHITE);

  const int camera_x = static_cast<int>(std::lround(camera_pos_.x));
  const int camera_y = static_cast<int>(std::lround(camera_pos_.y));
  const Vector2 player_screen{ player_pos_.x - camera_x, player_pos_.y - camera_y };

  // Scene: white interiors stay as background; only black outlines and solids
  // are drawn. The darkness pass hides whatever is not lit.
  draw_target(BED, camera_x, camera_y);

  for (const auto& rug : RUGS) {
    draw_target(rug, camera_x, camera_y);
  }

  draw_target(BOX, camera_x, camera_y);
  draw_rectangle_lines(screen_x(BOX.x + 4.0f, camera_x), screen_y(BOX.y + 4.0f, camera_y),
                       static_cast<int>(std::lround(BOX.width - 8.0f)),
                       static_cast<int>(std::lround(BOX.height - 8.0f)), LCD_BLACK);

  for (const auto& cat : CATS) {
    draw_circle_lines(screen_x(cat.x, camera_x), screen_y(cat.y, camera_y),
                      static_cast<int>(std::lround(CAT_RADIUS)), LCD_BLACK);
  }

  draw_filled_circle(static_cast<int>(std::lround(player_screen.x)),
                     static_cast<int>(std::lround(player_screen.y)),
                     static_cast<int>(PLAYER_RADIUS), LCD_BLACK);

  static ScreenPoint vertices[LIGHT_RAYS];
  build_light_polygon(player_pos_, camera_x, camera_y, vertices);
  draw_darkness_polygon(vertices, LIGHT_RAYS, to_subpixel(player_screen.x),
                        to_subpixel(player_screen.y));
}

void HouseScreen::close() {
}
