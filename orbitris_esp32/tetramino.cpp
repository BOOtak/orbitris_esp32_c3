#include "tetramino.h"

#include "draw.h"

Tetramino I_Block{
  { 2, 2 },
  { { { 0, 0, 0, 0 },
      { 1, 1, 1, 1 },
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 } },
    { { 0, 0, 1, 0 },
      { 0, 0, 1, 0 },
      { 0, 0, 1, 0 },
      { 0, 0, 1, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      { 1, 1, 1, 1 },
      { 0, 0, 0, 0 } },
    { { 0, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 } } }
};

Tetramino J_Block{
  { 1.5, 2.5 },
  { { { 0, 0, 0, 0 },
      { 1, 0, 0, 0 },
      { 1, 1, 1, 0 },
      { 0, 0, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      { 1, 1, 1, 0 },
      { 0, 0, 1, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 1, 1, 0, 0 } } }
};

Tetramino L_Block{
  { 1.5, 2.5 },
  { { { 0, 0, 0, 0 },
      { 0, 0, 1, 0 },
      { 1, 1, 1, 0 },
      { 0, 0, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 1, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      { 1, 1, 1, 0 },
      { 1, 0, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 1, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 } } }
};

Tetramino O_Block{
  { 2, 2 },
  { { { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 } } }
};

Tetramino S_Block{
  { 1.5, 2.5 },
  { { { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 1, 1, 0, 0 },
      { 0, 0, 0, 0 } },
    {
      { 0, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 1, 0 },
    },
    { { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 1, 1, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 1, 0, 0, 0 },
      { 1, 1, 0, 0 },
      { 0, 1, 0, 0 } } }
};

Tetramino T_Block{
  { 1.5, 2.5 },
  { { { 0, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 1, 1, 1, 0 },
      { 0, 0, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      { 1, 1, 1, 0 },
      { 0, 1, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 1, 1, 0, 0 },
      { 0, 1, 0, 0 } } }
};

Tetramino Z_Block{
  { 1.5, 2.5 },
  { { { 0, 0, 0, 0 },
      { 1, 1, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 0, 1, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      { 1, 1, 0, 0 },
      { 0, 1, 1, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 1, 1, 0, 0 },
      { 1, 0, 0, 0 } } }
};

Tetramino* Blocks[] = { &I_Block, &L_Block, &J_Block, &O_Block, &S_Block, &T_Block, &Z_Block };

Tetramino* get_random_block() {
  return Blocks[get_random_value(0, ARR_SIZE(Blocks) - 1)];
}

void draw_tile(int x, int y, int size) {
  draw_rectangle_lines(x, y, size, size, 0);
  draw_rectangle_checkerboard(x, y, size, size);
  draw_pixel(x, y, 1);
  draw_pixel(x + size - 1, y, 1);
  draw_pixel(x, y + size - 1, 1);
  draw_pixel(x + size - 1, y + size - 1, 1);
}

void draw_tetramino(const ActiveTetramino& tetramino) {
  if (tetramino.block == nullptr) {
    return;
  }

  int rot_index = tetramino.rot_index;
  uint8_t (*block)[4] = tetramino.block->data[rot_index];

  float startX = tetramino.pos.x - tetramino.block->center.x * TILE_W;
  float startY = tetramino.pos.y - tetramino.block->center.y * TILE_H;

  for (int i = 0; i < BLOCK_SIZE; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      if (block[i][j] == 0) {
        continue;
      }

      draw_tile(startX + j * TILE_W, startY + i * TILE_H, TILE_W);
    }
  }
}
