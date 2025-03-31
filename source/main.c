#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <3ds.h>
#include <citro2d.h>
// #include "costable.h" // Usually included implicitly via citro2d
#include <limits.h>
#include <dirent.h>  // For directory operations
#include <strings.h> // For strcasecmp

// --- Screen Dimensions ---
#define TOP_SCREEN_WIDTH  400
#define TOP_SCREEN_HEIGHT 240
#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HEIGHT 240

// --- List Configuration ---
#define MAX_LIST_ITEMS 1024
#define MUSIC_DIR "/music"

// --- List Item Layout Constants ---
#define LIST_ITEM_HEIGHT 48.0f
#define PLACEHOLDER_SIZE 32.0f // Size of the square placeholder
#define ITEM_PADDING_X 8.0f
#define ITEM_PADDING_Y ((LIST_ITEM_HEIGHT - PLACEHOLDER_SIZE) / 2.0f)
#define TEXT_AREA_LEFT (ITEM_PADDING_X + PLACEHOLDER_SIZE + ITEM_PADDING_X) // Where text starts X
#define TEXT_SCALE_TITLE 0.6f  // Default text size
#define TEXT_SCALE_ARTIST 0.5f // Slightly smaller for artist
#define TEXT_LINE_HEIGHT (TEXT_SCALE_TITLE * 16.0f) // Approximate pixel height for spacing

// --- Selection & Interaction Constants ---
#define PAGE_SCROLL_AMOUNT (BOTTOM_SCREEN_HEIGHT - LIST_ITEM_HEIGHT) // How much D-Pad Left/Right scrolls
// #define SELECTION_COLOR C2D_Color32(0x00, 0x7F, 0xFF, 0x60) // Old blueish color
// #define SELECTION_BORDER_COLOR C2D_Color32(0x00, 0x7F, 0xFF, 0xC0) // Old border color
#define SELECTION_PADDING_X 5.0f // Horizontal padding for the selection highlight
#define SELECTION_COLOR C2D_Color32(0xFF, 0xFF, 0xFF, 0xB7) // New semi-transparent white
// Removed SELECTION_BORDER_COLOR as we are removing the explicit borders for a cleaner look


// --- Data Structure for List Items ---
typedef struct {
    char *filename; // Always store the original filename
    char *title;    // Title from metadata (NULL if none)
    char *artist;   // Artist from metadata (NULL if none)
    // Add other fields if needed (album, duration, etc.)
} MusicListItem;

// --- Static Text Data ---
C2D_TextBuf g_staticBuf;
C2D_Text g_pearPlayerText;

// --- Dynamic Text Data (for list items) ---
C2D_TextBuf g_dynamicBuf; // Increase size slightly for potentially more text
#define DYNAMIC_BUF_SIZE 12288

// --- List Data & State ---
MusicListItem* g_listItems[MAX_LIST_ITEMS]; // Array of POINTERS to list items
int g_actualNumListItems = 0;
float g_scrollPixelOffset = 0.0f;   // Current scroll position in pixels
float g_maxScrollPixelOffset = 0.0f; // Maximum scrollable distance
float g_totalListHeight = 0.0f;     // Total height of all items combined
int g_selectedIndex = -1;           // Index of the currently selected item

// --- Function Declarations ---
static void setupListItems(void);
static void sceneInit(void);
static void sceneRenderTop(void);
static void sceneRenderBottom(void);
static void sceneExit(void);
static void handleInput(void);
// Removed: static void updateScrollPhysics(void);
static bool hasMusicExtension(const char *filename);
static void ensureSelectionIsVisible(void);
// --- Placeholder Function (Replace with real metadata reader) ---
static void readMusicMetadata(const char* filepath, char** title, char** artist);

// --- Function Implementations ---

static bool hasMusicExtension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return false;
    if (strcasecmp(dot, ".mp3") == 0) return true;
    if (strcasecmp(dot, ".ogg") == 0) return true;
    if (strcasecmp(dot, ".wav") == 0) return true;
    if (strcasecmp(dot, ".flac") == 0) return true;
    if (strcasecmp(dot, ".m4a") == 0) return true;
    return false;
}

// --- !!! IMPORTANT PLACEHOLDER !!! ---
// Replace this function with actual metadata reading using a library.
static void readMusicMetadata(const char* filepath, char** title, char** artist) {
    // Default to NULL
    *title = NULL;
    *artist = NULL;

    // ---=== Placeholder Logic (Simulates reading) ===---
    // Construct full path (necessary if metadata library needs it)
    // char fullpath[PATH_MAX];
    // snprintf(fullpath, sizeof(fullpath), "%s/%s", MUSIC_DIR, filename_only); // Be careful with buffer sizes

    // Example: Simulate based on filename containing keywords
    if (strstr(filepath, "_TA_") != NULL) { // e.g., "Some Artist_TA_Some Title.mp3"
        *title = strdup("Simulated Title");
        *artist = strdup("Simulated Artist");
    } else if (strstr(filepath, "_T_") != NULL) { // e.g., "Some Title Only_T_.mp3"
         *title = strdup("Simulated Title Only");
    } else if (strstr(filepath, "Error") != NULL) { // Handle error messages passed in filename slot
         *title = NULL; // No metadata for errors
         *artist = NULL;
    }
     // else: title and artist remain NULL, filename will be used.

    // ---=== End Placeholder Logic ===---

    // In a real implementation using e.g., TagLib:
    // TagLib::FileRef f(fullpath);
    // if (!f.isNull() && f.tag()) {
    //     TagLib::Tag *tag = f.tag();
    //     if (!tag->title().isEmpty()) {
    //         *title = strdup(tag->title().toCString(true)); // true for UTF-8
    //     }
    //     if (!tag->artist().isEmpty()) {
    //         *artist = strdup(tag->artist().toCString(true));
    //     }
    // }
    // Remember to handle memory allocation failures (strdup returns NULL).
}
// --- !!! END PLACEHOLDER !!! ---


static void setupListItems(void) {
    DIR *dir;
    struct dirent *entry;

    // Free any previously allocated items
    for (int i = 0; i < g_actualNumListItems; ++i) {
        if (g_listItems[i] != NULL) {
            free(g_listItems[i]->filename);
            free(g_listItems[i]->title);
            free(g_listItems[i]->artist);
            free(g_listItems[i]);
            g_listItems[i] = NULL;
        }
    }
    g_actualNumListItems = 0;

    dir = opendir(MUSIC_DIR);
    if (dir == NULL) {
        perror("opendir failed");
        // Create a dummy item for the error message
        MusicListItem* errorItem = (MusicListItem*)malloc(sizeof(MusicListItem));
        if (errorItem) {
            errorItem->filename = strdup("Error: /music not found");
            errorItem->title = NULL;
            errorItem->artist = NULL;
             if (!errorItem->filename) { // strdup failed
                 free(errorItem);
             } else {
                g_listItems[0] = errorItem;
                g_actualNumListItems = 1;
             }
        }
        g_selectedIndex = -1;
        return;
    }

    while ((entry = readdir(dir)) != NULL && g_actualNumListItems < MAX_LIST_ITEMS) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        if (hasMusicExtension(entry->d_name)) {
            MusicListItem* newItem = (MusicListItem*)malloc(sizeof(MusicListItem));
            if (!newItem) {
                perror("malloc failed for MusicListItem");
                break; // Stop adding items if memory runs out
            }

            // Initialize pointers
            newItem->filename = NULL;
            newItem->title = NULL;
            newItem->artist = NULL;

            // Store filename
            newItem->filename = strdup(entry->d_name);
            if (!newItem->filename) {
                perror("strdup failed for filename");
                free(newItem); // Clean up allocated struct
                continue; // Skip this file
            }

            // --- Call Metadata Reader (Placeholder) ---
            // Construct full path (example, adjust as needed by real reader)
             char fullpath[PATH_MAX]; // Ensure PATH_MAX is defined (limits.h or define it)
             snprintf(fullpath, sizeof(fullpath)-1, "%s/%s", MUSIC_DIR, newItem->filename);
             fullpath[sizeof(fullpath)-1] = '\0'; // Ensure null termination

            readMusicMetadata(fullpath, &newItem->title, &newItem->artist);
            // Note: readMusicMetadata allocates title/artist via strdup if found

            // Add the new item to the list
            g_listItems[g_actualNumListItems] = newItem;
            g_actualNumListItems++;
        }
    }
    closedir(dir);

    if (g_actualNumListItems == 0) {
        // Create a dummy item for the "not found" message
         MusicListItem* notFoundItem = (MusicListItem*)malloc(sizeof(MusicListItem));
        if (notFoundItem) {
            notFoundItem->filename = strdup("No music files found");
            notFoundItem->title = NULL;
            notFoundItem->artist = NULL;
             if (!notFoundItem->filename) { // strdup failed
                 free(notFoundItem);
             } else {
                g_listItems[0] = notFoundItem;
                g_actualNumListItems = 1;
             }
        }
        g_selectedIndex = -1;
    } else {
        g_selectedIndex = 0; // Select first item by default
        // Optional: Sort list here (would need a custom comparison function for the struct)
    }
}

static void sceneInit(void)
{
    g_staticBuf  = C2D_TextBufNew(100);
    g_dynamicBuf = C2D_TextBufNew(DYNAMIC_BUF_SIZE); // Use larger buffer

    C2D_TextParse(&g_pearPlayerText, g_staticBuf, "Pear Player");
    C2D_TextOptimize(&g_pearPlayerText);

    setupListItems(); // Populates the list with MusicListItem structs

    g_scrollPixelOffset = 0.0f;
    // Removed: g_scrollVelocity = 0.0f;
    // Removed: g_isTouching = false;

    g_totalListHeight = (float)g_actualNumListItems * LIST_ITEM_HEIGHT;
    g_maxScrollPixelOffset = fmaxf(0.0f, g_totalListHeight - BOTTOM_SCREEN_HEIGHT);

    ensureSelectionIsVisible();
}

static void sceneRenderTop(void)
{
    // Render gradient background
    for (int y = 0; y < TOP_SCREEN_HEIGHT; ++y) {
        u8 gray = (u8)(((float)y * 128.0f) / (float)TOP_SCREEN_HEIGHT);
        C2D_DrawRectSolid(0, y, 0.0f, TOP_SCREEN_WIDTH, 1, C2D_Color32(gray, gray, gray, 0xFF));
    }
    // Render title
    C2D_DrawText(&g_pearPlayerText, C2D_AlignCenter | C2D_WithColor,
                 TOP_SCREEN_WIDTH / 2.0f, TOP_SCREEN_HEIGHT / 2.0f - 8.0f, 0.5f,
                 1.0f, 1.0f, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
}

static void sceneRenderBottom(void)
{
    C2D_TextBufClear(g_dynamicBuf);

    int firstItemIndex = (int)floorf(g_scrollPixelOffset / LIST_ITEM_HEIGHT);
    float startY = -(g_scrollPixelOffset - (firstItemIndex * LIST_ITEM_HEIGHT));

    for (int i = -1; ; ++i) {
        int currentItemIndex = firstItemIndex + i;
        float rowTopY = startY + i * LIST_ITEM_HEIGHT;
        float rowBottomY = rowTopY + LIST_ITEM_HEIGHT;

        if (rowTopY >= BOTTOM_SCREEN_HEIGHT) break; // Stop drawing if below screen
        if (rowBottomY <= 0) continue;             // Skip if entirely above screen
        if (currentItemIndex < 0 || currentItemIndex >= g_actualNumListItems) continue; // Skip invalid indices

        // Get pointer to the current item's data
        MusicListItem* currentItem = g_listItems[currentItemIndex];
        if (!currentItem) continue; // Should not happen, but safety check

        // --- Draw Selection Highlight ---
        if (currentItemIndex == g_selectedIndex) {
            // Draw the main highlight rectangle with padding
            // NOTE: Citro2D does not have a built-in function for rounded rectangles.
            // This draws a standard rectangle with the requested color and padding.
            float highlightX = SELECTION_PADDING_X;
            float highlightWidth = BOTTOM_SCREEN_WIDTH - (2.0f * SELECTION_PADDING_X);
            C2D_DrawRectSolid(highlightX, rowTopY, 0.3f, highlightWidth, LIST_ITEM_HEIGHT, SELECTION_COLOR);

            // Removed the old top/bottom border lines for a cleaner look,
            // as they cannot be easily rounded with basic C2D functions.
            // C2D_DrawRectSolid(0.0f, rowTopY, 0.35f, BOTTOM_SCREEN_WIDTH, 1.0f, SELECTION_BORDER_COLOR); // Top border REMOVED
            // C2D_DrawRectSolid(0.0f, rowBottomY - 1.0f, 0.35f, BOTTOM_SCREEN_WIDTH, 1.0f, SELECTION_BORDER_COLOR); // Bottom border REMOVED
        }

        // --- Draw Placeholder ---
        float placeholderX = ITEM_PADDING_X;
        float placeholderY = rowTopY + ITEM_PADDING_Y;
        C2D_DrawRectSolid(placeholderX, placeholderY, 0.4f,
                          PLACEHOLDER_SIZE, PLACEHOLDER_SIZE,
                          C2D_Color32(0x40, 0x40, 0x40, 0xFF));

        // --- Draw Text based on available metadata ---
        C2D_Text dynTextTitle, dynTextArtist, dynTextFilename;
        float textX = TEXT_AREA_LEFT; // Use defined constant

        // Case 1: Title and Artist available
        if (currentItem->title && currentItem->artist) {
            // Calculate Y positions for two lines (approximate vertical centering)
            float titleBaseY = rowTopY + LIST_ITEM_HEIGHT * 0.33f;
            float artistBaseY = rowTopY + LIST_ITEM_HEIGHT * 0.66f;
             // Adjust based on font metrics if available, otherwise guess midpoint
            float titleY = titleBaseY - (TEXT_SCALE_TITLE * 16.0f / 2.0f) + 2.0f; // Nudge up slightly
            float artistY = artistBaseY - (TEXT_SCALE_ARTIST * 16.0f / 2.0f);

            // Draw Title
            C2D_TextParse(&dynTextTitle, g_dynamicBuf, currentItem->title);
            C2D_TextOptimize(&dynTextTitle);
            C2D_DrawText(&dynTextTitle, C2D_WithColor | C2D_AlignLeft,
                         textX, titleY, 0.5f, TEXT_SCALE_TITLE, TEXT_SCALE_TITLE,
                         C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

            // Draw Artist (slightly smaller/dimmer?)
            C2D_TextParse(&dynTextArtist, g_dynamicBuf, currentItem->artist);
            C2D_TextOptimize(&dynTextArtist);
            C2D_DrawText(&dynTextArtist, C2D_WithColor | C2D_AlignLeft,
                         textX, artistY, 0.5f, TEXT_SCALE_ARTIST, TEXT_SCALE_ARTIST,
                         C2D_Color32(0xCC, 0xCC, 0xCC, 0xFF)); // Lighter gray

        }
        // Case 2: Only Title available
        else if (currentItem->title) {
             // Calculate Y position for single centered line
            float centeredY = rowTopY + (LIST_ITEM_HEIGHT / 2.0f) - (TEXT_SCALE_TITLE * 16.0f / 2.0f);

            // Draw Title centered
            C2D_TextParse(&dynTextTitle, g_dynamicBuf, currentItem->title);
            C2D_TextOptimize(&dynTextTitle);
            C2D_DrawText(&dynTextTitle, C2D_WithColor | C2D_AlignLeft,
                         textX, centeredY, 0.5f, TEXT_SCALE_TITLE, TEXT_SCALE_TITLE,
                         C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
        }
        // Case 3: No metadata (or error message), display filename
        else if (currentItem->filename) { // Should always have filename unless allocation failed badly
            // Calculate Y position for single centered line
            float centeredY = rowTopY + (LIST_ITEM_HEIGHT / 2.0f) - (TEXT_SCALE_TITLE * 16.0f / 2.0f);

            // Draw Filename centered
            C2D_TextParse(&dynTextFilename, g_dynamicBuf, currentItem->filename);
            C2D_TextOptimize(&dynTextFilename);
            C2D_DrawText(&dynTextFilename, C2D_WithColor | C2D_AlignLeft,
                         textX, centeredY, 0.5f, TEXT_SCALE_TITLE, TEXT_SCALE_TITLE,
                         C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
        }
    }
}

static void sceneExit(void)
{
    C2D_TextBufDelete(g_dynamicBuf);
    C2D_TextBufDelete(g_staticBuf);

    // Free the allocated list items and their contents
    for (int i = 0; i < g_actualNumListItems; ++i) {
        if (g_listItems[i] != NULL) {
            free(g_listItems[i]->filename); // Free the duplicated filename
            free(g_listItems[i]->title);    // Free the duplicated title (if any)
            free(g_listItems[i]->artist);   // Free the duplicated artist (if any)
            free(g_listItems[i]);           // Free the struct itself
            g_listItems[i] = NULL;
        }
    }
    g_actualNumListItems = 0;
    g_selectedIndex = -1;
}

// Ensures the currently selected item is visible on screen, adjusting scroll if needed.
static void ensureSelectionIsVisible(void) {
    if (g_selectedIndex < 0 || g_selectedIndex >= g_actualNumListItems) return;

    // Don't adjust if list fits screen (no scrolling possible)
    if (g_maxScrollPixelOffset <= 0.0f) return;

    float selectedItemTopY = (float)g_selectedIndex * LIST_ITEM_HEIGHT;
    float selectedItemBottomY = selectedItemTopY + LIST_ITEM_HEIGHT;

    float viewTopY = g_scrollPixelOffset;
    float viewBottomY = g_scrollPixelOffset + BOTTOM_SCREEN_HEIGHT;

    // If item is above the view, scroll up to show it at the top
    if (selectedItemTopY < viewTopY) {
        g_scrollPixelOffset = selectedItemTopY;
        // Removed: g_scrollVelocity = 0;
    }
    // If item is below the view, scroll down to show it at the bottom
    else if (selectedItemBottomY > viewBottomY) {
        g_scrollPixelOffset = selectedItemBottomY - BOTTOM_SCREEN_HEIGHT;
        // Removed: g_scrollVelocity = 0;
    }

    // Clamp scroll offset just in case calculations went slightly out
    // (Can happen if PAGE_SCROLL_AMOUNT doesn't align perfectly with item heights)
    if (g_scrollPixelOffset < 0.0f) g_scrollPixelOffset = 0.0f;
    if (g_maxScrollPixelOffset > 0 && g_scrollPixelOffset > g_maxScrollPixelOffset) {
         g_scrollPixelOffset = g_maxScrollPixelOffset;
    }
}


// Handles only D-Pad input for selection and scrolling.
static void handleInput(void)
{
    hidScanInput();
    u32 kDown = hidKeysDown();
    // Removed: u32 kHeld = hidKeysHeld();
    // Removed: u32 kUp = hidKeysUp();

    // Removed touch input handling section entirely

    // --- D-Pad Handling ---
    if (g_actualNumListItems > 0) {
        bool selectionChanged = false; // Flag to check if we need to ensure visibility

        // D-Pad Up/Down Selection
        if (kDown & KEY_DDOWN) {
            // Removed: g_isTouching = false; g_scrollVelocity = 0.0f;
            if (g_selectedIndex < g_actualNumListItems - 1) {
                g_selectedIndex++;
                selectionChanged = true;
            }
        } else if (kDown & KEY_DUP) {
            // Removed: g_isTouching = false; g_scrollVelocity = 0.0f;
            if (g_selectedIndex > 0) {
                g_selectedIndex--;
                selectionChanged = true;
            }
        }

        // D-Pad Left/Right Page Scroll
        if (kDown & KEY_DRIGHT) { // Page Down
             // Removed: g_isTouching = false; g_scrollVelocity = 0.0f;
             g_scrollPixelOffset += PAGE_SCROLL_AMOUNT;
             // Clamp scroll offset to the valid range
             if (g_maxScrollPixelOffset > 0 && g_scrollPixelOffset > g_maxScrollPixelOffset) {
                 g_scrollPixelOffset = g_maxScrollPixelOffset;
             } else if (g_maxScrollPixelOffset <= 0) { // Handle list shorter than screen
                 g_scrollPixelOffset = 0.0f;
             }
             // Recalculate selected index based on the new scroll position (optional but can feel better)
             // int firstVisible = floorf(g_scrollPixelOffset / LIST_ITEM_HEIGHT);
             // g_selectedIndex = firstVisible; // Or maybe first visible + some offset
             // For simplicity, we just scroll and rely on ensureSelectionIsVisible later if needed
             selectionChanged = true; // Might need to adjust view even if index didn't change
        } else if (kDown & KEY_DLEFT) { // Page Up
             // Removed: g_isTouching = false; g_scrollVelocity = 0.0f;
             g_scrollPixelOffset -= PAGE_SCROLL_AMOUNT;
             // Clamp scroll offset to the valid range
             if (g_scrollPixelOffset < 0.0f) {
                 g_scrollPixelOffset = 0.0f;
             }
             // Recalculate selected index based on the new scroll position (optional)
             // int firstVisible = floorf(g_scrollPixelOffset / LIST_ITEM_HEIGHT);
             // g_selectedIndex = firstVisible;
             selectionChanged = true; // Might need to adjust view
        }

        // Ensure the selected item is visible after any D-Pad action that might change it
        if (selectionChanged) {
            ensureSelectionIsVisible();
        }
    }
}

// Removed updateScrollPhysics function entirely, as there's no velocity or elasticity.

// --- Main Application Entry Point ---
int main()
{
    // --- Initialize Services ---
    romfsInit();
    gfxInitDefault();
    cfguInit();
    Result rc = qtmcInit(); // Filesystem service
    if (R_FAILED(rc)) {
        // Handle error appropriately - maybe draw an error on screen later
        // This basic example will just proceed and fail in setupListItems
         gfxExit();
         return 1;
    }

    // --- Initialize Citro3D & Citro2D ---
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 4);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS * 2);
    C2D_Prepare();

    // --- Create Render Targets ---
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // --- Initialize Scene Data ---
    sceneInit(); // Sets up text, list data (incl. metadata placeholders)

    // --- Main Loop ---
    while (aptMainLoop())
    {
        handleInput(); // Handles D-Pad input for selection/scrolling

        if (hidKeysDown() & KEY_START) break; // Exit condition

        // Removed: updateScrollPhysics(); // Physics are no longer calculated

        // --- Rendering ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // Top Screen
        C2D_TargetClear(top, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
        C2D_SceneBegin(top);
        sceneRenderTop();

        // Bottom Screen
        C2D_TargetClear(bottom, C2D_Color32(0x11, 0x11, 0x11, 0xFF));
        C2D_SceneBegin(bottom);
        sceneRenderBottom(); // Renders list based on g_scrollPixelOffset

        C3D_FrameEnd(0);
    }

    // --- Deinitialization ---
    sceneExit(); // Clean up list items and buffers

    C2D_Fini();
    C3D_Fini();
    qtmcExit();
    cfguExit();
    gfxExit();
    romfsExit();

    return 0;
}