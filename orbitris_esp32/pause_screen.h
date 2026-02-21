#include "screen.h"

#include "button_grid_manager.h"
#include "game_utils.h"

constexpr auto PAUSE_BUTTONS_COUNT = 2;

class PauseScreen : public Screen {
public:
  PauseScreen();

  virtual void init() override;

  virtual Screen* update() override;

  virtual void draw() const override;

private:
  Vector2 text_size_;
  char text_buffer_[100];
  Button pause_buttons_[PAUSE_BUTTONS_COUNT];
  int grid_map_[PAUSE_BUTTONS_COUNT];
  ButtonGridManager manager_;
};
