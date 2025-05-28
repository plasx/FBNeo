# FBNeo 2025 AI Implementation Plan

## Phase 1: Build System Stabilization (Current Phase)

### Goals
- Fix all C/C++ language compatibility issues
- Resolve macro expansion problems
- Create proper type declarations
- Ensure clean compilation of the Metal backend

### Current Status
- Created C/C++ compatibility header (`c_cpp_compatibility.h`)
- Implemented game genre variables for C compatibility
- Fixed CPU core configuration structs

### Remaining Tasks
- Update makefile.metal with improved C/C++ handling
- Create cleaner stub implementations
- Fix remaining header issues

## Phase 2: CoreML Integration (Next Phase)

### Goals
- Implement robust model loading mechanism
- Create model discovery and verification system
- Add conversion capabilities for multiple model formats
- Establish the basic AI inference pipeline

### Tasks

#### 1. Model Loading System
- [ ] Complete the CoreML model manager implementation
  ```objectivec
  // src/burner/metal/ai/coreml_manager.mm
  @implementation CoreMLManager
  - (BOOL)loadModelFromPath:(NSString *)path error:(NSError **)error {
      // Enhanced model loading with verification
      NSURL *modelURL = [NSURL fileURLWithPath:path];
      MLModel *model = [MLModel modelWithContentsOfURL:modelURL error:error];
      if (!model) return NO;
      
      // Validate model input/output structure
      if (![self validateModelStructure:model error:error]) return NO;
      
      // Configure for Neural Engine hardware acceleration
      MLModelConfiguration *config = [[MLModelConfiguration alloc] init];
      config.computeUnits = MLComputeUnitsAll;
      
      // Create prediction model with hardware acceleration
      self.predictionModel = [model modelWithConfiguration:config error:error];
      return (self.predictionModel != nil);
  }
  @end
  ```

#### 2. Model Discovery
- [ ] Create model search functionality in standard locations
  ```objectivec
  - (NSArray<NSURL *> *)discoverModelsInStandardLocations {
      NSMutableArray<NSURL *> *modelURLs = [NSMutableArray array];
      
      // Check application bundle
      [modelURLs addObjectsFromArray:[self modelsInDirectory:[[NSBundle mainBundle] resourceURL]]];
      
      // Check application support directory
      NSArray *appSupportPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, 
                                                                    NSUserDomainMask, YES);
      if (appSupportPaths.count > 0) {
          NSString *appSupportPath = [appSupportPaths[0] stringByAppendingPathComponent:@"FBNeo/Models"];
          [modelURLs addObjectsFromArray:[self modelsInDirectory:[NSURL fileURLWithPath:appSupportPath]]];
      }
      
      // Check user's Documents folder
      NSArray *documentPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, 
                                                                  NSUserDomainMask, YES);
      if (documentPaths.count > 0) {
          NSString *documentsPath = [documentPaths[0] stringByAppendingPathComponent:@"FBNeo/Models"];
          [modelURLs addObjectsFromArray:[self modelsInDirectory:[NSURL fileURLWithPath:documentsPath]]];
      }
      
      return modelURLs;
  }
  ```

#### 3. Model Verification
- [ ] Implement model structure validation for compatibility
  ```objectivec
  - (BOOL)validateModelStructure:(MLModel *)model error:(NSError **)error {
      // Verify input requirements
      MLModelDescription *description = model.modelDescription;
      
      // Check for required input features
      if (![description.inputDescriptionsByName objectForKey:@"gameFrame"]) {
          if (error) {
              *error = [NSError errorWithDomain:@"com.fbneo.ai" 
                                          code:1001 
                                      userInfo:@{NSLocalizedDescriptionKey: 
                                                @"Model is missing required 'gameFrame' input"}];
          }
          return NO;
      }
      
      // Check for required output features
      if (![description.outputDescriptionsByName objectForKey:@"actions"]) {
          if (error) {
              *error = [NSError errorWithDomain:@"com.fbneo.ai" 
                                          code:1002 
                                      userInfo:@{NSLocalizedDescriptionKey: 
                                                @"Model is missing required 'actions' output"}];
          }
          return NO;
      }
      
      return YES;
  }
  ```

#### 4. Basic C Interface
- [ ] Create C wrapper functions for Objective-C implementation
  ```c
  // src/burner/metal/ai/ai_interface.c
  #include "ai_interface.h"
  #include "ai_definitions.h"

  // Forward declaration of Objective-C functions
  extern bool coreml_initialize(void);
  extern bool coreml_load_model(const char* model_path);
  extern bool coreml_predict(const void* frame_data, size_t frame_size, 
                            void* output_actions, size_t output_size);
  extern void coreml_shutdown(void);

  // C API Implementation
  bool AI_Initialize(void) {
      return coreml_initialize();
  }

  bool AI_LoadModel(const char* model_path) {
      return coreml_load_model(model_path);
  }

  bool AI_Predict(const struct AIFrameData* frame_data, struct AIActions* actions) {
      return coreml_predict(frame_data->data, frame_data->size, 
                           actions, sizeof(struct AIActions));
  }

  void AI_Shutdown(void) {
      coreml_shutdown();
  }
  ```

## Phase 3: Tensor Operations (Week 2-3)

### Goals
- Implement optimized tensor operations using Metal
- Create batch processing capabilities
- Add support for different precision modes
- Optimize performance for Apple Silicon

### Tasks

#### 1. Basic Metal Tensor Operations
- [ ] Implement matrix multiplication shader
  ```metal
  // src/burner/metal/ai/tensor_ops.metal
  #include <metal_stdlib>
  using namespace metal;

  kernel void matrix_multiply(device const float* inA [[buffer(0)]],
                             device const float* inB [[buffer(1)]],
                             device float* result [[buffer(2)]],
                             constant uint& M [[buffer(3)]],
                             constant uint& N [[buffer(4)]],
                             constant uint& K [[buffer(5)]],
                             uint2 gid [[thread_position_in_grid]]) {
      // Ensure within bounds
      if (gid.x >= N || gid.y >= M) return;
      
      float sum = 0.0f;
      for (uint i = 0; i < K; i++) {
          sum += inA[gid.y * K + i] * inB[i * N + gid.x];
      }
      
      result[gid.y * N + gid.x] = sum;
  }
  ```

#### 2. Convolution Operations
- [ ] Implement 2D convolution for neural network layers
  ```metal
  kernel void convolution_2d(device const float* input [[buffer(0)]],
                            device const float* weights [[buffer(1)]],
                            device const float* bias [[buffer(2)]],
                            device float* output [[buffer(3)]],
                            constant uint& inputWidth [[buffer(4)]],
                            constant uint& inputHeight [[buffer(5)]],
                            constant uint& inputChannels [[buffer(6)]],
                            constant uint& outputChannels [[buffer(7)]],
                            constant uint& kernelSize [[buffer(8)]],
                            uint3 gid [[thread_position_in_grid]]) {
      // Implement efficient 2D convolution 
      // with optimizations for Metal execution
  }
  ```

#### 3. Batch Processing
- [ ] Add support for processing multiple frames at once
  ```objectivec
  - (BOOL)predictBatch:(NSArray<MLFeatureValue *> *)inputBatch 
           outputBatch:(NSMutableArray<MLFeatureValue *> *)outputBatch
                 error:(NSError **)error {
      // Implement batch prediction
      NSMutableArray<MLDictionaryFeatureProvider *> *inputProviders = [NSMutableArray array];
      
      for (MLFeatureValue *inputValue in inputBatch) {
          MLDictionaryFeatureProvider *provider = 
              [[MLDictionaryFeatureProvider alloc] initWithDictionary:@{@"gameFrame": inputValue}
                                                               error:error];
          if (!provider) return NO;
          [inputProviders addObject:provider];
      }
      
      // Process batch
      NSArray<MLFeatureProvider *> *outputProviders = 
          [self.batchModel predictionsFromBatch:inputProviders error:error];
      if (!outputProviders) return NO;
      
      // Extract results
      [outputBatch removeAllObjects];
      for (MLFeatureProvider *provider in outputProviders) {
          MLFeatureValue *actions = [provider featureValueForName:@"actions"];
          [outputBatch addObject:actions];
      }
      
      return YES;
  }
  ```

## Phase 4: Integration with Emulation Core (Week 3-4)

### Goals
- Integrate AI system with the main emulation loop
- Implement frame capture for AI processing
- Create action injection mechanism
- Add performance monitoring

### Tasks

#### 1. Frame Capture System
- [ ] Implement efficient frame capture from emulation core
  ```c
  // src/burner/metal/ai/frame_capture.c
  #include "frame_capture.h"
  #include "ai_definitions.h"

  // Buffer for frame data
  static uint8_t frame_buffer[MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT * 4];
  static struct AIFrameData capture_frame = {
      .data = frame_buffer,
      .width = 0,
      .height = 0,
      .channels = 4,
      .size = 0
  };

  // Capture the current frame
  struct AIFrameData* CaptureCurrentFrame(void) {
      // Get current screen dimensions
      unsigned int width, height;
      if (!GetCurrentScreenDimensions(&width, &height)) {
          return NULL;
      }
      
      // Ensure buffer is large enough
      if (width * height * 4 > sizeof(frame_buffer)) {
          return NULL;
      }
      
      // Capture the screen
      if (!CopyScreenBuffer(frame_buffer, width * height * 4)) {
          return NULL;
      }
      
      // Update frame data structure
      capture_frame.width = width;
      capture_frame.height = height;
      capture_frame.size = width * height * 4;
      
      return &capture_frame;
  }
  ```

#### 2. Action Injection
- [ ] Create system to inject AI actions into emulation
  ```c
  // src/burner/metal/ai/action_injection.c
  #include "action_injection.h"
  #include "ai_definitions.h"

  // Apply AI generated actions to the emulation
  bool ApplyAIActions(const struct AIActions* actions) {
      if (!actions) return false;
      
      // Map AI actions to emulator inputs
      for (int i = 0; i < actions->action_count; i++) {
          struct AIAction action = actions->actions[i];
          
          // Apply the action based on type
          switch (action.type) {
              case AI_ACTION_BUTTON:
                  // Map to specific button press/release
                  SetInputState(action.input_id, action.value > 0.5f);
                  break;
                  
              case AI_ACTION_JOYSTICK:
                  // Map to analog joystick value
                  SetAnalogInput(action.input_id, action.value);
                  break;
                  
              // Additional action types
              
              default:
                  // Unknown action type
                  return false;
          }
      }
      
      return true;
  }
  ```

#### 3. Main AI Loop Integration
- [ ] Implement AI processing in main emulation loop
  ```c
  // src/burner/metal/ai_integration.c
  #include "ai_interface.h"
  #include "frame_capture.h"
  #include "action_injection.h"

  // AI state
  static bool ai_enabled = false;
  static int ai_frame_skip = 2;  // Process every 3rd frame
  static int frame_counter = 0;

  // Initialize AI system
  bool InitializeAI(void) {
      return AI_Initialize();
  }

  // Load the specified AI model
  bool LoadAIModel(const char* model_path) {
      return AI_LoadModel(model_path);
  }

  // Process a single frame with AI
  void ProcessFrameWithAI(void) {
      if (!ai_enabled) return;
      
      // Skip frames as needed
      frame_counter++;
      if (frame_counter % (ai_frame_skip + 1) != 0) return;
      
      // Capture current frame
      struct AIFrameData* frame = CaptureCurrentFrame();
      if (!frame) return;
      
      // Process with AI
      struct AIActions actions = {0};
      if (!AI_Predict(frame, &actions)) return;
      
      // Apply resulting actions
      ApplyAIActions(&actions);
  }

  // Enable/disable AI processing
  void SetAIEnabled(bool enabled) {
      ai_enabled = enabled;
  }

  // Shutdown AI system
  void ShutdownAI(void) {
      ai_enabled = false;
      AI_Shutdown();
  }
  ```

## Phase 5: User Interface and Configuration (Week 4-5)

### Goals
- Create user interface for AI features
- Implement configuration system for AI behavior
- Add visualization of AI decisions
- Create model management interface

### Tasks

#### 1. AI Configuration Interface
- [ ] Create settings UI for AI configuration
- [ ] Implement persistence for AI settings
- [ ] Add control for model selection
- [ ] Create performance tuning interface

#### 2. AI Visualization
- [ ] Add overlays to visualize AI predictions
- [ ] Create debug views for model outputs
- [ ] Implement heatmaps for attention mechanisms

#### 3. Model Management
- [ ] Create model browser and selection UI
- [ ] Add model information display
- [ ] Implement model import functionality

## Timeline and Milestones

### Week 1 (Current)
- ✅ Fix C/C++ compatibility issues
- ✅ Create core compatibility header
- ⚪ Update makefile for mixed C/C++ compilation

### Week 2
- ⚪ Complete basic CoreML integration
- ⚪ Implement model loading system
- ⚪ Create C API for AI functionality

### Week 3
- ⚪ Implement optimized tensor operations
- ⚪ Create batch processing capabilities
- ⚪ Add frame capture system

### Week 4
- ⚪ Integrate AI system with emulation core
- ⚪ Implement action injection
- ⚪ Create performance monitoring

### Week 5
- ⚪ Develop user interface for AI features
- ⚪ Implement configuration system
- ⚪ Add visualization capabilities

## Resource Requirements

### Development Resources
- Apple Silicon Mac with Metal 3.0+ support
- Xcode 15+ with Metal shader debugging
- CoreML model development tools
- Sample trained models for testing

### Testing Resources
- Various game ROMs covering different genres
- Performance benchmarking tools
- Reference AI implementations for comparison

## Success Criteria

The FBNeo 2025 AI implementation will be considered successful when:

1. The system can load and run CoreML models for game AI
2. Frame processing occurs with minimal performance impact
3. AI features are accessible through a user-friendly interface
4. Multiple model formats are supported through conversion
5. Features are optimized for Apple Silicon platform
6. Documentation is complete for both users and developers 