// src/intf/video/sdl/sdl2_video_metal.mm
// SDL front-end code bridging to metal_renderer.mm

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include "../metal_renderer.mm"  // Or use a separate header if you prefer

static SDL_Window* g_sdlWindowMetal = nullptr;

extern bool MetalRendererInit(void* nsWindowPtr, int width, int height);
extern void MetalRendererShutdown();
extern void MetalRendererDrawFrame(const void* frameBuffer, int width, int height);

extern "C" int FBNeo_InitVideoMetal(int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }

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

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(g_sdlWindowMetal, &wmInfo)) {
#if defined(__APPLE__)
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
    // Handle events so the window is responsive
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            // Could set a global to exit
        }
    }

    MetalRendererDrawFrame(frameBuffer, width, height);
}