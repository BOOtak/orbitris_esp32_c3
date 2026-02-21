#include "screen.h"

#include "game_utils.h"

class PauseScreen : public Screen {
public:
  PauseScreen();

  virtual void init() override;

  virtual Screen* update() override;

  virtual void draw() const override;

private:
  Vector2 text_size_;
  char text_buffer_[100];
};
