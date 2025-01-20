/**
 * sdl2_video_metal.mm
 *
 * Example SDL code bridging to Metal
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include "../intf/video/metal_renderer.mm" // or use a forward-declared header
#include <stdio.h>

// A few placeholders:
static SDL_Window* g_sdlWindow = nullptr;

extern "C" int FBNeo_InitVideoMetal(int width, int height)
{
    // 1. Create an SDL window
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    // The window must be created with SDL_WINDOW_OPENGL or SDL_WINDOW_METAL flags.
    // Actually, for direct Metal usage you usually want SDL_WINDOW_METAL:
    g_sdlWindow = SDL_CreateWindow("FBNeo + Metal",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   width, height,
                                   SDL_WINDOW_METAL);

    if (!g_sdlWindow) {
        printf("Failed to create SDL window: %s\n", SDL_GetError());
        return -1;
    }

    // 2. Get the NSWindow pointer via SDL_SysWMinfo
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(g_sdlWindow, &wmInfo)) {
#if defined(__APPLE__)
        // On macOS, info contains an NSWindow pointer
        void* nsWindowPtr = (__bridge void*)wmInfo.info.cocoa.window;

        // 3. Now we initialize Metal with that pointer
        bool success = MetalRendererInit(nsWindowPtr, width, height);
        if (!success) {
            return -1;
        }
#else
        // Not on macOS — handle other OS?
#endif
    } else {
        printf("SDL_GetWindowWMInfo failed.\n");
        return -1;
    }

    printf("FBNeo_InitVideoMetal: OK\n");
    return 0;
}

extern "C" void FBNeo_ShutdownVideoMetal()
{
    MetalRendererShutdown();
    if (g_sdlWindow) {
        SDL_DestroyWindow(g_sdlWindow);
        g_sdlWindow = nullptr;
    }
    SDL_Quit();
}

extern "C" void FBNeo_DrawFrameMetal(const void* frameBuffer, int width, int height)
{
    // Example usage:
    MetalRendererDrawFrame(frameBuffer, width, height);
    // Possibly also handle SDL event pumping if needed:
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            // handle quitting
        }
    }
}
