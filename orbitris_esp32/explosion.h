#pragma once

#include "tilemap.h"

void init_explosion(const Tilemap& map, const Vector2& center);

bool update_explosion();

void draw_explosion();
