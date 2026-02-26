#pragma once

#include "screen.h"

class AuthorsScreen : public Screen {
public:
  AuthorsScreen();

  void init() override;

  Screen* update() override;

  void draw() const override;
};
