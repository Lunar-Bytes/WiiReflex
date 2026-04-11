#include <grrlib.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>
#include <time.h>
#include <math.h>

#include "font.h"

// Message system
float msgTimer = 0;
float msgAlpha = 0;
int msgState = 0;

// raw2c output
extern const unsigned char font[];
extern const int font_size;

#define MAX_PLATFORMS 6

typedef struct {
    float x, y, w, h;
    u32 color;

    int hasEnemy;
    float shootCooldown;
} Platform;

// Score
int score = 0;
int lastPlatform = -1;

void ResetLevel(Platform *stages, float maxJump) {
    stages[0].x = 220;
    stages[0].y = 400;
    stages[0].w = 200;
    stages[0].h = 15;
    stages[0].color = 0xFFFFFFFF;
    stages[0].hasEnemy = 0;
    stages[0].shootCooldown = 0;

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

        stages[i].hasEnemy = 0;
        stages[i].shootCooldown = 180;

        // Spawn sniper platform
        if (score > 50 && (rand() % 128 == 0)) {
            stages[i].hasEnemy = 1;
            stages[i].color = 0x880000FF;
        }
    }
}

int main(int argc, char **argv) {
    GRRLIB_Init();
    WPAD_Init();
    srand(time(NULL));

    GRRLIB_texImg *tex_font = GRRLIB_LoadTexturePNG(font);

    GRRLIB_SetHandle(tex_font, 0, 0);
    GRRLIB_SetBlend(GRRLIB_BLEND_ALPHA);

    if (tex_font) {
        GRRLIB_InitTileSet(tex_font, 25, 26, 32);
    }

    float px = 300.0f, py = 200.0f, velY = 0.0f;
    int isJumping = 1;

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

        // Movement
        if (held & WPAD_BUTTON_DOWN) px += SPEED;
        if (held & WPAD_BUTTON_UP)   px -= SPEED;

        // Jump
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

        // Camera scroll
        if (py < 200) {
            float diff = 200 - py;
            py = 200;

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

                    stages[i].hasEnemy = 0;
                    stages[i].shootCooldown = 180;

                    if (score > 50 && (rand() % 128 == 0)) {
                        stages[i].hasEnemy = 1;
                        stages[i].color = 0x880000FF;
                    } else {
                        stages[i].color = 0xFFFFFFFF;
                    }
                }
            }
        }

        // Collision
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

                    if (lastPlatform != i) {
                        score++;
                        lastPlatform = i;
                    }
                }
            }
        }

        // Enemy Shooting
        for(int i = 0; i < MAX_PLATFORMS; i++) {
            if (!stages[i].hasEnemy) continue;

            stages[i].shootCooldown--;

            if (stages[i].shootCooldown <= 0) {
                stages[i].shootCooldown = 180;

                if (abs(px - stages[i].x) < 200 &&
                    abs(py - stages[i].y) < 200) {

                    px = 300;
                    py = 200;
                    velY = 0;
                    score = 0;
                    lastPlatform = -1;

                    msgTimer = 0;
                    msgAlpha = 0;
                    msgState = 1;

                    ResetLevel(stages, MAX_JUMP_DIST);
                }
            }
        }

        // Fall reset
        if (py > 500) {
            px = 300;
            py = 200;
            velY = 0;
            score = 0;
            lastPlatform = -1;

            ResetLevel(stages, MAX_JUMP_DIST);
        }

        // Message animation
        if (msgState == 1) {
            msgAlpha += 0.02f;
            if (msgAlpha >= 1.0f) {
                msgAlpha = 1.0f;
                msgState = 2;
                msgTimer = 180;
            }
        }
        else if (msgState == 2) {
            msgTimer--;
            if (msgTimer <= 0) msgState = 3;
        }
        else if (msgState == 3) {
            msgAlpha -= 0.02f;
            if (msgAlpha <= 0) {
                msgAlpha = 0;
                msgState = 0;
            }
        }

        // Render
        GRRLIB_FillScreen(0x111111FF);

        // Platforms
        for (int i = 0; i < MAX_PLATFORMS; i++) {

            GRRLIB_Rectangle(
                stages[i].x,
                stages[i].y,
                stages[i].w,
                stages[i].h,
                stages[i].color,
                1
            );

            if (stages[i].hasEnemy) {

                // Border
                GRRLIB_Rectangle(
                    stages[i].x + stages[i].w/2 - 10,
                    stages[i].y - 18,
                    20, 20,
                    0x000000FF,
                    1
                );

                // Enemy
                GRRLIB_Rectangle(
                    stages[i].x + stages[i].w/2 - 8,
                    stages[i].y - 16,
                    16, 16,
                    0x550000FF,
                    1
                );
            }
        }

        // Player
        GRRLIB_Circle(px, py, radius, 0xA020F0FF, 1);

        // Score
        if (tex_font) {
            GRRLIB_Printf(30, 30, tex_font, 0xFFFFFFFF, 1.0f, "SCORE %d", score);
        }

        // Death message
        if (msgState != 0 && tex_font) {
            GRRLIB_Printf(
                350,
                440,
                tex_font,
                ((int)(msgAlpha * 255) << 24) | 0xFFFFFF,
                1.0f,
                "You got shot"
            );
        }

        GRRLIB_Render();
    }

    if (tex_font) GRRLIB_FreeTexture(tex_font);

    GRRLIB_Exit();
    exit(0);
}