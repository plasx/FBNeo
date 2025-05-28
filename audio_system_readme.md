# FBNeo Metal Audio System

## Overview
This document describes the implementation of the audio system for the FBNeo Metal port on macOS, focusing on CoreAudio integration for Marvel vs Capcom (CPS2).

## Audio System Components

### 1. Audio Configuration (metal_audio_simple.mm)
- Sample Rate: 44100 Hz
- Buffer Size: 735 samples (44100/60 to match 60fps)
- Channels: 2 (stereo)
- Format: 16-bit signed integer PCM
- AudioQueue buffers: 4 buffers for smooth playback

### 2. Audio Implementation (metal_audio_simple.mm)
- Uses AudioQueue Services for low-latency audio output
- Matches buffer size to frame rate (735 samples at 60fps)
- Supports volume control and audio enabling/disabling
- Handles buffer underruns and overruns gracefully
- Tracks audio statistics for debugging

### 3. Audio Interface (metal_audio_stubs.h)
- Declares function prototypes for the audio system
- Ensures consistent interface across the codebase

## Key Functions

### Metal_InitAudio()
- Initializes the audio system and allocates necessary resources
- Sets FBNeo audio parameters (nBurnSoundRate, nBurnSoundLen)
- Allocates audio buffer and sets pBurnSoundOut pointer
- Creates AudioQueue and starts playback

### Metal_ExitAudio()
- Cleans up audio resources
- Stops AudioQueue and disposes of resources
- Frees audio buffer

### Metal_UpdateAudio()
- Called once per frame to update audio data
- Calls BurnSoundRender to fill the audio buffer with sound data
- Maintains synchronization between audio and video

### Metal_AudioCallback()
- CoreAudio callback function for filling audio buffers
- Manages audio buffer underruns and overruns
- Applies volume settings to audio samples
- Handles silence when audio data is not available

## Frame Timing and Audio Synchronization

The audio system is carefully designed to match the frame rate of the emulation:

1. At 60 FPS, each frame takes 16.67ms
2. At 44100 Hz sample rate, each frame should produce 735 samples (44100/60)
3. The audio buffer size is set to exactly match this requirement

This ensures that audio and video remain in sync, preventing audio stuttering or drift.

## Implementation Notes

1. **Buffer Size Calculation**: We use 735 samples per frame which is the exact value for 44100Hz at 60fps (44100/60 = 735).

2. **AudioQueue Management**: We use 4 audio buffers to ensure smooth playback, which provides approximately 66.7ms of buffering.

3. **Volume Control**: Audio volume can be adjusted from 0.0 (silent) to 1.0 (full volume) using Metal_SetAudioVolume().

4. **Debugging Features**: The implementation includes tracking for buffer underruns/overruns and Metal_PrintAudioStatus() for diagnostic output.

5. **Integration with FBNeo**: The system sets pBurnSoundOut to point to our audio buffer, allowing the emulator core to write audio data directly to our buffer.

## How to Build and Test

```
./build_audio.sh
```

This will compile the emulator with the audio implementation and run it with Marvel vs Capcom if the ROM is available.

## Future Enhancements

1. Add user-configurable audio settings (volume, enable/disable)

2. Implement a more sophisticated buffer management system for variable frame rates

3. Add spectrum analyzer and oscilloscope visualizations for debugging

4. Support for different sample rates and formats

5. Consider migrating to AVAudioEngine for more modern audio processing capabilities 