#include "raylib.h"

#include <cmath>

#include "../orbitris_esp32/const.h"
#include "../orbitris_esp32/game_main.h"

constexpr auto WINDOW_SCALE = 3;

RenderTexture2D target;

void UpdateDrawFrame()
{
    BeginTextureMode(target);
    update_draw_frame();
    EndTextureMode();

    float scale = fmin((float)GetScreenWidth() / LCD_WIDTH, (float)GetScreenHeight() / LCD_HEIGHT);
    Rectangle targetTextureRect = {0.0f, 0.0f, (float)LCD_WIDTH, (float)-LCD_HEIGHT};

    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(target.texture, targetTextureRect,
                   {(GetScreenWidth() - ((float)LCD_WIDTH * scale)) * 0.5f, (GetScreenHeight() - ((float)LCD_HEIGHT * scale)) * 0.5f,
                    (float)LCD_WIDTH * scale, (float)LCD_HEIGHT * scale},
                   {0.0f, 0.0f}, 0.0f, WHITE);
    EndDrawing();
}

int main(int argc, char const *argv[])
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(LCD_WIDTH * WINDOW_SCALE, LCD_HEIGHT * WINDOW_SCALE, "Orbitris ESP32 adapted for Raylib");

    target = LoadRenderTexture(LCD_WIDTH, LCD_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    init_game();

    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }

    CloseWindow();

    return 0;
}
