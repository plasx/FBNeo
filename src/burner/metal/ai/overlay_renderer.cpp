#include "overlay_renderer.h"
#include "../metal_common.h"
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <MetalFX/MetalFX.h>
#import <simd/simd.h>
#import <QuartzCore/CAMetalLayer.h>

// Forward declaration of Metal renderer
@class MetalRenderer;

// Initialize static instance
OverlayRenderer* OverlayRenderer::s_instance = nullptr;

// Get singleton instance
OverlayRenderer* OverlayRenderer::getInstance() {
    if (!s_instance) {
        s_instance = new OverlayRenderer();
    }
    return s_instance;
}

// Implementation class for OverlayRenderer
class OverlayRenderer::Impl {
public:
    Impl() : m_viewportWidth(0), m_viewportHeight(0), m_initialized(false),
             m_device(nil), m_commandQueue(nil), m_metalLayer(nil),
             m_metalFXUpscaler(nil), m_lowResRenderTarget(nil),
             m_upscalingEnabled(true) {
    }
    
    ~Impl() {
        // Clean up Metal resources
        if (m_metalFXUpscaler) {
            [m_metalFXUpscaler release];
            m_metalFXUpscaler = nil;
        }
        
        if (m_lowResRenderTarget) {
            [m_lowResRenderTarget release];
            m_lowResRenderTarget = nil;
        }
        
        if (m_commandQueue) {
            [m_commandQueue release];
            m_commandQueue = nil;
        }
    }
    
    bool initialize() {
        if (m_initialized) {
            return true;
        }
        
        // Get default Metal device
        m_device = MTLCreateSystemDefaultDevice();
        if (!m_device) {
            return false;
        }
        
        // Check if Metal 3 is supported for advanced features
        bool supportsMetal3 = [m_device supportsFamily:MTLGPUFamilyMetal3];
        
        // Create command queue
        m_commandQueue = [m_device newCommandQueue];
        if (!m_commandQueue) {
            return false;
        }
        
        // Initialize upscaling if Metal 3 is supported
        if (supportsMetal3) {
            initializeMetalFXUpscaling();
        } else {
            m_upscalingEnabled = false;
        }
        
        m_initialized = true;
        return true;
    }
    
    void initializeMetalFXUpscaling() {
        // Set up MetalFX for high-quality upscaling of debug overlays
        // This allows us to render debug information at a lower resolution
        // and upscale it efficiently for better performance
        
        // Create low-resolution render target at half size
        int lowResWidth = m_viewportWidth / 2;
        int lowResHeight = m_viewportHeight / 2;
        
        if (lowResWidth <= 0 || lowResHeight <= 0) {
            lowResWidth = 640;
            lowResHeight = 480;
        }
        
        // Create texture descriptor for low-res rendering
        MTLTextureDescriptor* lowResDesc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
            width:lowResWidth height:lowResHeight mipmapped:NO];
        lowResDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        lowResDesc.storageMode = MTLStorageModePrivate;
        
        // Create low-res render target
        m_lowResRenderTarget = [m_device newTextureWithDescriptor:lowResDesc];
        
        // Set up MetalFX upscaler if supported
        if (@available(macOS 13.0, *)) {
            // Configure the upscaler
            MTLFXSpatialScalerDescriptor* upscalerDesc = [[MTLFXSpatialScalerDescriptor alloc] init];
            
            // Set input and output sizes
            upscalerDesc.inputWidth = lowResWidth;
            upscalerDesc.inputHeight = lowResHeight;
            upscalerDesc.outputWidth = m_viewportWidth;
            upscalerDesc.outputHeight = m_viewportHeight;
            
            // Configure quality and color processing
            upscalerDesc.colorProcessingMode = MTLFXSpatialScalerColorProcessingModePerceptual;
            
            // Create the upscaler
            NSError* error = nil;
            m_metalFXUpscaler = [m_device newFXSpatialScalerWithDescriptor:upscalerDesc error:&error];
            [upscalerDesc release];
            
            if (error || !m_metalFXUpscaler) {
                NSLog(@"Failed to create MetalFX upscaler: %@", error);
                m_upscalingEnabled = false;
            }
        } else {
            // MetalFX not available on this OS version
            m_upscalingEnabled = false;
        }
    }
    
    void beginFrame() {
        if (!m_initialized) {
            return;
        }
        
        // Start a command buffer for rendering
        m_currentCommandBuffer = [m_commandQueue commandBuffer];
        
        if (m_upscalingEnabled && m_lowResRenderTarget) {
            // Start rendering to low-res target for upscaling
            MTLRenderPassDescriptor* renderPassDesc = [MTLRenderPassDescriptor renderPassDescriptor];
            renderPassDesc.colorAttachments[0].texture = m_lowResRenderTarget;
            renderPassDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
            renderPassDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
            renderPassDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
            
            m_currentRenderEncoder = [m_currentCommandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
            [m_currentRenderEncoder setLabel:@"Overlay Low-Res Encoder"];
        } else {
            // Direct rendering without upscaling
            // This would typically use the main render target
            // Implementation would connect to the main renderer
        }
    }
    
    void endFrame() {
        if (!m_initialized || !m_currentCommandBuffer) {
            return;
        }
        
        // End the render encoder
        if (m_currentRenderEncoder) {
            [m_currentRenderEncoder endEncoding];
            m_currentRenderEncoder = nil;
        }
        
        if (m_upscalingEnabled && m_metalFXUpscaler && m_lowResRenderTarget) {
            // Apply upscaling from low-res to final target
            if (@available(macOS 13.0, *)) {
                // Get the final drawable
                id<CAMetalDrawable> drawable = [m_metalLayer nextDrawable];
                if (drawable) {
                    // Upscale from low-res to drawable
                    [m_metalFXUpscaler encodeToCommandBuffer:m_currentCommandBuffer
                                                sourceTexture:m_lowResRenderTarget
                                                destinationTexture:drawable.texture];
                    
                    // Present the drawable
                    [m_currentCommandBuffer presentDrawable:drawable];
                }
            }
        }
        
        // Commit the command buffer
        [m_currentCommandBuffer commit];
        m_currentCommandBuffer = nil;
    }
    
    void drawRect(float x, float y, float width, float height, 
                 float r, float g, float b, float a) {
        if (!m_initialized || !m_currentRenderEncoder) {
            return;
        }
        
        // Implementation would render a rectangle
        // This would use a shader to draw the rectangle
    }
    
    void drawRectOutline(float x, float y, float width, float height,
                        float r, float g, float b, float a, float thickness) {
        if (!m_initialized || !m_currentRenderEncoder) {
            return;
        }
        
        // Implementation would render a rectangle outline
        // This would use a shader to draw the lines
    }
    
    void drawLine(float x1, float y1, float x2, float y2,
                 float r, float g, float b, float a, float thickness) {
        if (!m_initialized || !m_currentRenderEncoder) {
            return;
        }
        
        // Implementation would render a line
        // This would use a shader to draw the line
    }
    
    void drawText(float x, float y, const char* text,
                 float r, float g, float b, float a, float fontSize) {
        if (!m_initialized || !m_currentRenderEncoder || !text) {
            return;
        }
        
        // Implementation would render text
        // This would use a text rendering system (likely CoreText)
    }
    
    void drawTextWithShadow(float x, float y, const char* text,
                           float r, float g, float b, float a, float fontSize) {
        // Draw shadow
        drawText(x + 1, y + 1, text, 0.0f, 0.0f, 0.0f, a * 0.7f, fontSize);
        
        // Draw main text
        drawText(x, y, text, r, g, b, a, fontSize);
    }
    
    int getViewportWidth() const {
        return m_viewportWidth;
    }
    
    int getViewportHeight() const {
        return m_viewportHeight;
    }
    
    void setViewportSize(int width, int height) {
        if (m_viewportWidth != width || m_viewportHeight != height) {
            m_viewportWidth = width;
            m_viewportHeight = height;
            
            // Recreate upscaler with new dimensions if needed
            if (m_upscalingEnabled && m_initialized) {
                // Clean up existing resources
                if (m_metalFXUpscaler) {
                    [m_metalFXUpscaler release];
                    m_metalFXUpscaler = nil;
                }
                
                if (m_lowResRenderTarget) {
                    [m_lowResRenderTarget release];
                    m_lowResRenderTarget = nil;
                }
                
                // Recreate with new size
                initializeMetalFXUpscaling();
            }
        }
    }
    
    void setMetalLayer(CAMetalLayer* layer) {
        m_metalLayer = layer;
    }
    
private:
    int m_viewportWidth;
    int m_viewportHeight;
    bool m_initialized;
    bool m_upscalingEnabled;
    
    // Metal resources
    id<MTLDevice> m_device;
    id<MTLCommandQueue> m_commandQueue;
    id<MTLCommandBuffer> m_currentCommandBuffer;
    id<MTLRenderCommandEncoder> m_currentRenderEncoder;
    CAMetalLayer* m_metalLayer;
    
    // MetalFX resources for upscaling
    id<MTLFXSpatialScaler> m_metalFXUpscaler;
    id<MTLTexture> m_lowResRenderTarget;
};

// OverlayRenderer implementation delegating to the Impl class
OverlayRenderer::OverlayRenderer() : m_impl(new Impl()) {
}

OverlayRenderer::~OverlayRenderer() {
}

bool OverlayRenderer::initialize() {
    return m_impl->initialize();
}

void OverlayRenderer::beginFrame() {
    m_impl->beginFrame();
}

void OverlayRenderer::endFrame() {
    m_impl->endFrame();
}

void OverlayRenderer::drawRect(float x, float y, float width, float height, 
                              float r, float g, float b, float a) {
    m_impl->drawRect(x, y, width, height, r, g, b, a);
}

void OverlayRenderer::drawRectOutline(float x, float y, float width, float height,
                                     float r, float g, float b, float a, float thickness) {
    m_impl->drawRectOutline(x, y, width, height, r, g, b, a, thickness);
}

void OverlayRenderer::drawLine(float x1, float y1, float x2, float y2,
                              float r, float g, float b, float a, float thickness) {
    m_impl->drawLine(x1, y1, x2, y2, r, g, b, a, thickness);
}

void OverlayRenderer::drawText(float x, float y, const char* text,
                              float r, float g, float b, float a, float fontSize) {
    m_impl->drawText(x, y, text, r, g, b, a, fontSize);
}

void OverlayRenderer::drawTextWithShadow(float x, float y, const char* text,
                                        float r, float g, float b, float a, float fontSize) {
    m_impl->drawTextWithShadow(x, y, text, r, g, b, a, fontSize);
}

int OverlayRenderer::getViewportWidth() const {
    return m_impl->getViewportWidth();
}

int OverlayRenderer::getViewportHeight() const {
    return m_impl->getViewportHeight();
}

void OverlayRenderer::setViewportSize(int width, int height) {
    m_impl->setViewportSize(width, height);
}

void OverlayRenderer::setMetalLayer(CAMetalLayer* layer) {
    m_impl->setMetalLayer(layer);
} 