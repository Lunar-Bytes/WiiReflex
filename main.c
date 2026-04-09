#include <grrlib.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>
#include <time.h>
#include <math.h>
#include "player.h" 

// --- LINKER STUBS ---
void PNGU_SelectImageFromBuffer() {}
void PNGU_GetImageProperties() {}
void PNGU_DecodeTo4x4RGBA8() {}
void PNGU_ReleaseImageContext() {}
void BrotliDecoderDecompress() {} 

#define MAX_PLATFORMS 6

typedef struct {
    float x, y, w, h;
    u32 color;
} Platform;

int main(int argc, char **argv) {
    GRRLIB_Init();
    WPAD_Init();
    srand(time(NULL)); 

    GRRLIB_texImg *tex_player = GRRLIB_LoadTexture(player);

    float px = 300.0f, py = 200.0f, velY = 0.0f;
    int isJumping = 1;
    
    // --- NEW: Score tracking ---
    int score = 0; 
    
    // Physics constants
    const float GRAVITY = 0.5f;
    const float JUMP_POWER = -12.0f;
    const float MAX_JUMP_POWER = -18.0f;
    const float SPEED = 4.0f;
    const float drawScale = 3.0f;
    const float playerSize = 16.0f * drawScale;

    // Platform generation constraints
    const float MAX_JUMP_DIST = 215.0f; 

    Platform stages[MAX_PLATFORMS];
    
    // Initialize first platform under player
    stages[0].x = 220;
    stages[0].y = 400;
    stages[0].w = 200;
    stages[0].h = 15;
    stages[0].color = 0xFFFFFFFF;

    // Build the initial "staircase" of possible platforms
    for(int i = 1; i < MAX_PLATFORMS; i++) {
        stages[i].w = 100;
        stages[i].h = 15;
        stages[i].color = 0xFFFFFFFF;
        stages[i].y = stages[i-1].y - 100; 
        
        float minX = stages[i-1].x - MAX_JUMP_DIST;
        float maxX = stages[i-1].x + stages[i-1].w + MAX_JUMP_DIST - stages[i].w;
        
        if (minX < 20) minX = 20;
        if (maxX > 520) maxX = 520;
        
        stages[i].x = minX + (rand() % (int)(maxX - minX + 1));
    }

    while(1) {
        WPAD_ScanPads();
        u32 held = WPAD_ButtonsHeld(0);
        u32 down = WPAD_ButtonsDown(0);
        if (down & WPAD_BUTTON_HOME) break;

        // --- CONTROLS (Sideways) ---
        if (held & WPAD_BUTTON_DOWN) px += SPEED; 
        if (held & WPAD_BUTTON_UP)   px -= SPEED; 

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

        // --- INFINITE SCROLLING LOGIC ---
        if (py < 200) {
            float diff = 200 - py;
            py = 200;
            
            // --- NEW: Increase score as the camera scrolls up ---
            score += (int)diff; 
            
            for(int i = 0; i < MAX_PLATFORMS; i++) {
                stages[i].y += diff;
                
                if (stages[i].y > 480) {
                    int highestIdx = (i == 0) ? MAX_PLATFORMS - 1 : i - 1;
                    stages[i].y = stages[highestIdx].y - 100; 
                    
                    float minX = stages[highestIdx].x - MAX_JUMP_DIST;
                    float maxX = stages[highestIdx].x + stages[highestIdx].w + MAX_JUMP_DIST - stages[i].w;
                    
                    if (minX < 20) minX = 20;
                    if (maxX > 520) maxX = 520;
                    
                    stages[i].x = minX + (rand() % (int)(maxX - minX + 1));
                }
            }
        }

        // Collision
        isJumping = 1;
        for(int i = 0; i < MAX_PLATFORMS; i++) {
            if (velY > 0 && px + (playerSize - 5) > stages[i].x && px + 5 < stages[i].x + stages[i].w) {
                if (py + playerSize > stages[i].y && py + playerSize < stages[i].y + stages[i].h + velY) {
                    py = stages[i].y - playerSize;
                    velY = 0;
                    isJumping = 0;
                }
            }
        }

        // Rescue if falling off bottom
        if (py > 480) { 
            py = 0; px = 300; velY = 0; 
            score = 0; // Reset score if you fall!
        }

        // --- DRAWING ---
        GRRLIB_FillScreen(0x111111FF);

        for(int i = 0; i < MAX_PLATFORMS; i++) {
            GRRLIB_Rectangle(stages[i].x, stages[i].y, stages[i].w, stages[i].h, stages[i].color, 1);
        }

        // Draw Player (Purple)
        if(tex_player) {
            GRRLIB_DrawImg(px, py, tex_player, 0, drawScale, drawScale, 0x800080FF);
        } else {
            GRRLIB_Rectangle(px, py, playerSize, playerSize, 0x800080FF, 1);
        }
        
        // --- NEW: Draw Text ---
        // Top Left: Score
        GRRLIB_Printf(20, 20, NULL, 0xFFFFFFFF, 1, "SCORE: %d", score);
        // Top Right: Controls reminder
        GRRLIB_Printf(380, 20, NULL, 0xFFFFFFFF, 1, "1: High Jump");
        GRRLIB_Printf(380, 40, NULL, 0xFFFFFFFF, 1, "2: Normal Jump");
        GRRLIB_Printf(340, 60, NULL, 0xFFFFFFFF, 1, "Move: Left, Right arrow");
        GRRLIB_Printf(340, 80, NULL, 0xFFFFFFFF, 1, "Switch Wii remote sideways");
        
        GRRLIB_Render();
    }

    if(tex_player) GRRLIB_FreeTexture(tex_player);
    GRRLIB_Exit();
    exit(0);
}