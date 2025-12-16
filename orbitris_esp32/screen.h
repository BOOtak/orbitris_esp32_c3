#pragma once

class Screen {
public:
  Screen();

  virtual ~Screen() = default;

  virtual void init();

  virtual Screen* update() = 0;

  virtual void draw() const;

  virtual void close();
};

namespace screens {
extern Screen* game_screen;
extern Screen* game_over_screen;
}
