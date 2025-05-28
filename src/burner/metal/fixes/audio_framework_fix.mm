#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

// This file exists solely to import AVFoundation classes needed for linking.
// These classes will be properly used by metal_audio.mm but need to be linked.

// Force the AVAudio classes to be linked
@class AVAudioEngine;
@class AVAudioFormat;
@class AVAudioPCMBuffer;
@class AVAudioPlayerNode;

// Dummy functions that reference these classes to force them to be linked
extern "C" {
    void* GetAVAudioEngineClass() {
        return (__bridge void*)NSClassFromString(@"AVAudioEngine");
    }
    
    void* GetAVAudioFormatClass() {
        return (__bridge void*)NSClassFromString(@"AVAudioFormat");
    }
    
    void* GetAVAudioPCMBufferClass() {
        return (__bridge void*)NSClassFromString(@"AVAudioPCMBuffer");
    }
    
    void* GetAVAudioPlayerNodeClass() {
        return (__bridge void*)NSClassFromString(@"AVAudioPlayerNode");
    }
}

@interface AudioFrameworkFix : NSObject
+ (void)initialize;
@end

@implementation AudioFrameworkFix
+ (void)initialize {
    // Just reference AVAudio classes to ensure they're linked
    NSLog(@"Initializing AudioFrameworkFix");
    Class audioEngineClass = [AVAudioEngine class];
    Class audioFormatClass = [AVAudioFormat class];
    Class audioPCMBufferClass = [AVAudioPCMBuffer class];
    Class audioPlayerNodeClass = [AVAudioPlayerNode class];
}
@end 