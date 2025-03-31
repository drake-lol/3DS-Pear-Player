#include <3ds.h>
#include <stdio.h>

int main() {
    // Initialize graphics
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL); // Use top screen for text output

    printf("Hello, World!\nPress START to exit.");

    while (aptMainLoop()) {
        hidScanInput();
        
        // If START is pressed, exit the app
        if (hidKeysDown() & KEY_START)
            break;

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    // Clean up and exit
    gfxExit();
    return 0;
}