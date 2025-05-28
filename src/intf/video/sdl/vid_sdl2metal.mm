// Metal via SDL2
#include "burner.h"
#include "vid_support.h"
#include "vid_softfx.h"

#include <SDL.h>
#include <SDL_metal.h>
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

extern char videofiltering[3];

static id<MTLDevice> metalDevice = nil;
static id<MTLCommandQueue> metalCommandQueue = nil;
static id<MTLRenderPipelineState> metalPipelineState = nil;
static id<MTLTexture> metalTexture = nil;
static id<MTLBuffer> metalVertexBuffer = nil;
static CAMetalLayer* metalLayer = nil;

static int nInitedSubsytems = 0;
static int nGamesWidth = 0, nGamesHeight = 0; // screen size
extern SDL_Window* sdlWindow;
static unsigned char* gamescreen = NULL;

static int nTextureWidth = 512;
static int nTextureHeight = 512;

static int nSize;
static int nUseBlitter;

static int nRotateGame = 0;
static bool bFlipped = false;

static char Windowtitle[512];

static int BlitFXExit()
{
    if (metalTexture) {
        [metalTexture release];
        metalTexture = nil;
    }
    if (metalVertexBuffer) {
        [metalVertexBuffer release];
        metalVertexBuffer = nil;
    }
    if (metalPipelineState) {
        [metalPipelineState release];
        metalPipelineState = nil;
    }
    if (metalCommandQueue) {
        [metalCommandQueue release];
        metalCommandQueue = nil;
    }
    if (metalLayer) {
        [metalLayer release];
        metalLayer = nil;
    }

    free(gamescreen);
    gamescreen = NULL;

    SDL_DestroyWindow(sdlWindow);
    sdlWindow = NULL;
    
    nRotateGame = 0;

    return 0;
}

static int GetTextureSize(int Size)
{
    int nTextureSize = 128;

    while (nTextureSize < Size)
    {
        nTextureSize <<= 1;
    }

    return nTextureSize;
}

static int BlitFXInit()
{
    int nMemLen = 0;

    nVidImageDepth = bDrvOkay ? 16 : 32;
    nVidImageBPP = (nVidImageDepth + 7) >> 3;
    nBurnBpp = nVidImageBPP;

    SetBurnHighCol(nVidImageDepth);

    if (!nRotateGame)
    {
        nVidImageWidth = nGamesWidth;
        nVidImageHeight = nGamesHeight;
    }
    else
    {
        nVidImageWidth = nGamesHeight;
        nVidImageHeight = nGamesWidth;
    }

    nVidImagePitch = nVidImageWidth * nVidImageBPP;
    nBurnPitch = nVidImagePitch;

    nMemLen = nVidImageWidth * nVidImageHeight * nVidImageBPP;

#ifdef FBNEO_DEBUG
    printf("nVidImageWidth=%d nVidImageHeight=%d nVidImagePitch=%d\n", nVidImageWidth, nVidImageHeight, nVidImagePitch);
    printf("nTextureWidth=%d nTextureHeight=%d TexturePitch=%d\n", nTextureWidth, nTextureHeight, nTextureWidth * nVidImageBPP);
#endif

    gamescreen = (unsigned char*)malloc(nMemLen);
    if (gamescreen)
    {
        memset(gamescreen, 0, nMemLen);
        pVidImage = gamescreen;
        return 0;
    }
    else
    {
        pVidImage = NULL;
        return 1;
    }

    return 0;
}

static int Exit()
{
    BlitFXExit();

    if (!(nInitedSubsytems & SDL_INIT_VIDEO))
    {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    nInitedSubsytems = 0;

    return 0;
}

void init_metal()
{
#ifdef FBNEO_DEBUG
    printf("metal config\n");
#endif

    // Get Metal device
    metalDevice = MTLCreateSystemDefaultDevice();
    if (!metalDevice) {
        printf("Failed to create Metal device\n");
        return;
    }

    // Create command queue
    metalCommandQueue = [metalDevice newCommandQueue];
    if (!metalCommandQueue) {
        printf("Failed to create Metal command queue\n");
        return;
    }

    // Create Metal layer
    metalLayer = [CAMetalLayer layer];
    metalLayer.device = metalDevice;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.framebufferOnly = YES;
    metalLayer.frame = sdlWindow.contentView.bounds;

    // Create texture
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                          width:nTextureWidth
                                                                                         height:nTextureHeight
                                                                                      mipmapped:NO];
    metalTexture = [metalDevice newTextureWithDescriptor:textureDesc];

    // Create vertex buffer
    float vertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
    };
    metalVertexBuffer = [metalDevice newBufferWithBytes:vertices
                                                length:sizeof(vertices)
                                               options:MTLResourceStorageModeShared];

    // Create render pipeline
    id<MTLLibrary> defaultLibrary = [metalDevice newDefaultLibrary];
    id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];

    MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.vertexFunction = vertexFunction;
    pipelineDesc.fragmentFunction = fragmentFunction;
    pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

    NSError* error = nil;
    metalPipelineState = [metalDevice newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
    if (!metalPipelineState) {
        printf("Failed to create Metal pipeline state: %s\n", error.localizedDescription.UTF8String);
        return;
    }

#ifdef FBNEO_DEBUG
    printf("metal config done . . . \n");
#endif
}

static int VidSScaleImage(RECT* pRect)
{
    // Same as OpenGL implementation
    int nScrnWidth, nScrnHeight;

    int nGameAspectX = 4, nGameAspectY = 3;
    int nWidth = pRect->width;
    int nHeight = pRect->height;

    if (bVidFullStretch)
    { // Arbitrary stretch
        return 0;
    }

    if (bDrvOkay)
    {
        if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL)
        {
            BurnDrvGetAspect(&nGameAspectY, &nGameAspectX);
        }
        else
        {
            BurnDrvGetAspect(&nGameAspectX, &nGameAspectY);
        }
    }

    nScrnWidth = nGameAspectX;
    nScrnHeight = nGameAspectY;

    int nWidthScratch = nHeight * nVidScrnAspectY * nGameAspectX * nScrnWidth /
        (nScrnHeight * nVidScrnAspectX * nGameAspectY);

    if (nWidthScratch > nWidth)
    { // The image is too wide
        if (nGamesWidth < nGamesHeight)
        { // Vertical games
            nHeight = nWidth * nVidScrnAspectY * nGameAspectY * nScrnWidth /
                (nScrnHeight * nVidScrnAspectX * nGameAspectX);
        }
        else
        { // Horizontal games
            nHeight = nWidth * nVidScrnAspectX * nGameAspectY * nScrnHeight /
                (nScrnWidth * nVidScrnAspectY * nGameAspectX);
        }
    }
    else
    {
        nWidth = nWidthScratch;
    }

    // Center the image
    pRect->x = (pRect->width - nWidth) / 2;
    pRect->y = (pRect->height - nHeight) / 2;
    pRect->width = nWidth;
    pRect->height = nHeight;

    return 0;
}

static int Init()
{
    nInitedSubsytems = SDL_WasInit(SDL_INIT_VIDEO);

    if (!(nInitedSubsytems & SDL_INIT_VIDEO))
    {
        SDL_InitSubSystem(SDL_INIT_VIDEO);
    }

    nGamesWidth = nVidImageWidth;
    nGamesHeight = nVidImageHeight;

    nTextureWidth = GetTextureSize(nGamesWidth);
    nTextureHeight = GetTextureSize(nGamesHeight);

    if (BlitFXInit())
    {
        Exit();
        return 1;
    }

    init_metal();

    return 0;
}

static int Frame(bool bRedraw)
{
    if (pVidImage == NULL)
    {
        return 1;
    }

    if (bRedraw)
    {
        VidRedraw();
    }
    else
    {
        VidFrame();
    }

    return 0;
}

static void SurfToTex()
{
    [metalTexture replaceRegion:MTLRegionMake2D(0, 0, nVidImageWidth, nVidImageHeight)
                    mipmapLevel:0
                      withBytes:gamescreen
                    bytesPerRow:nVidImagePitch];
}

static void TexToQuad()
{
    id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
    if (!drawable) return;

    id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:drawable.texture];

    [renderEncoder setRenderPipelineState:metalPipelineState];
    [renderEncoder setVertexBuffer:metalVertexBuffer offset:0 atIndex:0];
    [renderEncoder setFragmentTexture:metalTexture atIndex:0];
    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];

    [renderEncoder endEncoding];
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

static int Paint(int bValidate)
{
    if (bValidate)
    {
        SurfToTex();
        TexToQuad();
    }

    return 0;
}

static int vidScale(RECT* pRect, int nWidth, int nHeight)
{
    if (pRect == NULL) {
        return 1;
    }

    pRect->x = 0;
    pRect->y = 0;
    pRect->width = nWidth;
    pRect->height = nHeight;

    return 0;
}

static int GetSettings(InterfaceInfo* pInfo)
{
    return 0;
}
struct VidOut VidOutSDL2Metal = { Init, Exit, Frame, Paint, vidScale, GetSettings, _T("SDL Metal Video output") }; 
