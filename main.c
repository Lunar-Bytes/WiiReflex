#include <grrlib.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>
#include "player.h" 

// --- LINKER STUBS ---
// We only keep the ones that are TRULY missing to avoid "multiple definition" errors.
void PNGU_SelectImageFromBuffer() {}
void PNGU_GetImageProperties() {}
void PNGU_DecodeTo4x4RGBA8() {}
void PNGU_ReleaseImageContext() {}
void BrotliDecoderDecompress() {} // FreeType needs this, but we'll stub it since we aren't using complex fonts.

typedef struct {
    float x, y, w, h;
    u32 color;
} Platform;

int main(int argc, char **argv) {
    GRRLIB_Init();
    WPAD_Init();

    GRRLIB_texImg *tex_player = GRRLIB_LoadTexture(player);

    float px = 300.0f, py = 100.0f, velY = 0.0f;
    int isJumping = 1;

    const float GRAVITY = 0.5f;
    const float JUMP_POWER = -12.0f;
    const float drawScale = 3.0f;
    const float playerSize = 16.0f * drawScale;

    Platform stages[3] = {
        {50, 400, 540, 20, 0xFFFFFFFF}, 
        {100, 280, 150, 15, 0xFF0000FF},
        {380, 180, 150, 15, 0x00FF00FF} 
    };

    while(1) {
        WPAD_ScanPads();
        u32 held = WPAD_ButtonsHeld(0);
        u32 down = WPAD_ButtonsDown(0);
        
        if (down & WPAD_BUTTON_HOME) break;

        // --- VERTICAL CONTROLS ---
        // Arrow Down -> Move Right
        if (held & WPAD_BUTTON_DOWN) px += 4.0f;
        // Arrow Up -> Move Left
        if (held & WPAD_BUTTON_UP)   px -= 4.0f;

        // Jump (WPAD_BUTTON_2 Button)
        if ((down & WPAD_BUTTON_2) && !isJumping) {
            velY = JUMP_POWER;
            isJumping = 1;
        }

        // Physics logic
        velY += GRAVITY;
        py += velY;

        // Collision logic
        isJumping = 1;
        for(int i = 0; i < 3; i++) {
            if (velY > 0 && px + (playerSize - 5) > stages[i].x && px + 5 < stages[i].x + stages[i].w) {
                if (py + playerSize > stages[i].y && py + playerSize < stages[i].y + stages[i].h + velY) {
                    py = stages[i].y - playerSize;
                    velY = 0;
                    isJumping = 0;
                }
            }
        }

        // --- DRAWING ---
        GRRLIB_FillScreen(0x111111FF);
        
        // The requested text
        GRRLIB_Printf(140, 40, NULL, 0xFFFFFFFF, 1, "Switch WII remove sideways");

        for(int i = 0; i < 3; i++) {
            GRRLIB_Rectangle(stages[i].x, stages[i].y, stages[i].w, stages[i].h, stages[i].color, 1);
        }

        // Keep the circle for debugging visibility
        GRRLIB_Circle(px + (playerSize/2), py + (playerSize/2), 10, 0xFFFFFFFF, 1);

        if(tex_player) {
            GRRLIB_DrawImg(px, py, tex_player, 0, drawScale, drawScale, 0xFFFFFFFF);
        }
        
        GRRLIB_Render();
    }

    if(tex_player) GRRLIB_FreeTexture(tex_player);
    GRRLIB_Exit();
    exit(0);
}