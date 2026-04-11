#include <grrlib.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>
#include <time.h>
#include <math.h>

#include "font.h"

// raw2c output (IMPORTANT: correct names)
extern const unsigned char font[];
extern const int font_size;

#define MAX_PLATFORMS 6

typedef struct {
    float x, y, w, h;
    u32 color;
} Platform;

void ResetLevel(Platform *stages, float maxJump) {
    stages[0].x = 220;
    stages[0].y = 400;
    stages[0].w = 200;
    stages[0].h = 15;
    stages[0].color = 0xFFFFFFFF;

    for (int i = 1; i < MAX_PLATFORMS; i++) {
        stages[i].w = 100;
        stages[i].h = 15;
        stages[i].color = 0xFFFFFFFF;
        stages[i].y = stages[i - 1].y - 100;

        float minX = stages[i - 1].x - maxJump;
        float maxX = stages[i - 1].x + stages[i - 1].w + maxJump - stages[i].w;

        if (minX < 20) minX = 20;
        if (maxX > 520) maxX = 520;

        stages[i].x = minX + (rand() % (int)(maxX - minX + 1));
    }
}

int main(int argc, char **argv) {
    GRRLIB_Init();
    WPAD_Init();
    srand(time(NULL));

    // Load font texture from raw2c buffer
    GRRLIB_texImg *tex_font = GRRLIB_LoadTexturePNG(font);

    if (tex_font) {
        GRRLIB_InitTileSet(tex_font, 16, 16, 0);
    }

    float px = 300.0f, py = 200.0f, velY = 0.0f;
    int isJumping = 1;
    int score = 0;

    const float GRAVITY = 0.5f;
    const float JUMP_POWER = -12.0f;
    const float MAX_JUMP_POWER = -18.0f;
    const float SPEED = 5.0f;
    const float radius = 25.0f;
    const float MAX_JUMP_DIST = 215.0f;

    Platform stages[MAX_PLATFORMS];
    ResetLevel(stages, MAX_JUMP_DIST);

    while (1) {
        WPAD_ScanPads();

        u32 held = WPAD_ButtonsHeld(0);
        u32 down = WPAD_ButtonsDown(0);

        if (down & WPAD_BUTTON_HOME) break;

        // movement
        if (held & WPAD_BUTTON_DOWN) px += SPEED;
        if (held & WPAD_BUTTON_UP)   px -= SPEED;

        // jump
        if ((down & WPAD_BUTTON_2) && !isJumping) {
            velY = JUMP_POWER;
            isJumping = 1;
        }
        if ((down & WPAD_BUTTON_1) && !isJumping) {
            velY = MAX_JUMP_POWER;
            isJumping = 1;
        }

        velY += GRAVITY;
        py += velY;

        // camera + score
        if (py < 200) {
            float diff = 200 - py;
            py = 200;
            score += (int)diff;

            for (int i = 0; i < MAX_PLATFORMS; i++) {
                stages[i].y += diff;

                if (stages[i].y > 480) {
                    int highest = (i == 0) ? MAX_PLATFORMS - 1 : i - 1;
                    stages[i].y = stages[highest].y - 100;

                    float minX = stages[highest].x - MAX_JUMP_DIST;
                    float maxX = stages[highest].x + stages[highest].w + MAX_JUMP_DIST - stages[i].w;

                    if (minX < 20) minX = 20;
                    if (maxX > 520) maxX = 520;

                    stages[i].x = minX + (rand() % (int)(maxX - minX + 1));
                }
            }
        }

        // collision
        isJumping = 1;

        for (int i = 0; i < MAX_PLATFORMS; i++) {
            if (velY > 0 &&
                px > stages[i].x - 10 &&
                px < stages[i].x + stages[i].w + 10) {

                if (py + radius > stages[i].y &&
                    py + radius < stages[i].y + stages[i].h + velY) {

                    py = stages[i].y - radius;
                    velY = 0;
                    isJumping = 0;
                }
            }
        }

        // reset if fall
        if (py > 500) {
            px = 300;
            py = 200;
            velY = 0;
            score = 0;
            ResetLevel(stages, MAX_JUMP_DIST);
        }

        // render
        GRRLIB_FillScreen(0x111111FF);

        for (int i = 0; i < MAX_PLATFORMS; i++) {
            GRRLIB_Rectangle(stages[i].x, stages[i].y,
                             stages[i].w, stages[i].h,
                             stages[i].color, 1);
        }

        // player
        GRRLIB_Circle(px, py, radius, 0xA020F0FF, 1);

        // score
        if (tex_font) {
            GRRLIB_Printf(30, 30, tex_font, 0xFFFFFFFF, 2, "SCORE %d", score);
        }

        GRRLIB_Render();
    }

    if (tex_font) GRRLIB_FreeTexture(tex_font);

    GRRLIB_Exit();
    exit(0);
}