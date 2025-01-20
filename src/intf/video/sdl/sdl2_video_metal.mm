//
// sdl2_video_metal.mm
//
// SDL front-end code that calls the metal_renderer code.
//
// We'll create 3 extern "C" functions that the rest of FBNeo can call:
//  1) FBNeo_InitVideoMetal()
//  2) FBNeo_ShutdownVideoMetal()
//  3) FBNeo_DrawFrameMetal()
//
// They are separate from "Interface" calls so that we can specifically
// create/destroy the SDL window for Metal usage.
//

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include "../metal_renderer.mm"  // We'll include the definitions (or a header) so we can call them

// We'll store the SDL window globally
static SDL_Window* g_sdlWindowMetal = nullptr;

// extern the metal_renderer init calls
extern bool MetalRendererInit(void* nsWindowPtr, int width, int height);
extern void MetalRendererShutdown();
extern void MetalRendererDrawFrame(const void* frameBuffer, int width, int height);

extern "C" int FBNeo_InitVideoMetal(int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }

    // Create an SDL window with the METAL flag
    g_sdlWindowMetal = SDL_CreateWindow(
        "FBNeo - Metal",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE
    );

    if (!g_sdlWindowMetal) {
        SDL_Log("Failed to create SDL window: %s", SDL_GetError());
        return -1;
    }

    // Now get the NSWindow pointer using SDL_SysWMinfo
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(g_sdlWindowMetal, &wmInfo)) {
    #if defined(__APPLE__)
        // On macOS, info.cocoa.window is an NSWindow*
        void* nsWindowPtr = (__bridge void*)wmInfo.info.cocoa.window;

        bool ok = MetalRendererInit(nsWindowPtr, width, height);
        if (!ok) {
            SDL_Log("MetalRendererInit failed.\n");
            return -1;
        }
        return 0;
    #else
        return -1;
    #endif
    } else {
        SDL_Log("SDL_GetWindowWMInfo failed.\n");
        return -1;
    }
}

extern "C" void FBNeo_ShutdownVideoMetal()
{
    MetalRendererShutdown();

    if (g_sdlWindowMetal) {
        SDL_DestroyWindow(g_sdlWindowMetal);
        g_sdlWindowMetal = nullptr;
    }
    SDL_Quit();
}

extern "C" void FBNeo_DrawFrameMetal(const void* frameBuffer, int width, int height)
{
    // We'll pump SDL events here so the window remains responsive
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            // You could handle shutdown logic or set a global to exit
        }
    }

    MetalRendererDrawFrame(frameBuffer, width, height);
}