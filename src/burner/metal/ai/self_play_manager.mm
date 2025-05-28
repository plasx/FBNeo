#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "self_play_manager.h"
#include "metal_training_engine.h"
#include "ai_torch_policy.h"
#include "game_state.h"

// Implementation of SelfPlayManager
@implementation SelfPlayManager {
    NSMutableArray<HeadlessRunner*>* _runners;
    NSMutableArray<NSNumber*>* _runnerScores;
    NSMutableArray<NSString*>* _modelPaths;
    dispatch_queue_t _trainingQueue;
    NSOperationQueue* _distributedQueue;
    BOOL _isTraining;
    int _currentGeneration;
    SelfPlayConfig _config;
    id<MTLDevice> _metalDevice;
    id<MTLCommandQueue> _commandQueue;
    MetalTrainingEngine* _trainingEngine;
    
    // Curriculum learning
    float _currentDifficulty;
    NSMutableArray<CurriculumStage*>* _curriculumStages;
    int _currentStageIndex;
}

- (instancetype)initWithConfig:(const SelfPlayConfig&)config {
    self = [super init];
    if (self) {
        // Initialize variables
        _config = config;
        _runners = [NSMutableArray array];
        _runnerScores = [NSMutableArray array];
        _modelPaths = [NSMutableArray array];
        _isTraining = NO;
        _currentGeneration = 0;
        _currentDifficulty = 0.0f;
        _curriculumStages = [NSMutableArray array];
        _currentStageIndex = 0;
        
        // Create concurrent dispatch queue for distributed training
        _trainingQueue = dispatch_queue_create("com.fbneo.selfplay.training", DISPATCH_QUEUE_CONCURRENT);
        _distributedQueue = [[NSOperationQueue alloc] init];
        _distributedQueue.maxConcurrentOperationCount = config.numParallelEnvs;
        
        // Initialize Metal
        _metalDevice = MTLCreateSystemDefaultDevice();
        if (_metalDevice) {
            _commandQueue = [_metalDevice newCommandQueue];
            
            // Initialize training engine
            TrainingConfig trainingConfig;
            trainingConfig.learningRate = config.learningRate;
            trainingConfig.clipRatio = config.clipRatio;
            trainingConfig.gamma = config.gamma;
            trainingConfig.lambda = config.lambda;
            trainingConfig.valueCoeff = config.valueCoeff;
            trainingConfig.entropyCoeff = config.entropyCoeff;
            trainingConfig.miniBatchSize = config.batchSize;
            trainingConfig.bufferCapacity = config.replayBufferSize;
            trainingConfig.updateFrequency = config.updateFrequency;
            
            _trainingEngine = new MetalTrainingEngine();
            if (!_trainingEngine->initialize(trainingConfig)) {
                NSLog(@"Failed to initialize Metal training engine");
                delete _trainingEngine;
                _trainingEngine = nullptr;
                return nil;
            }
        } else {
            NSLog(@"No Metal device available");
            return nil;
        }
        
        // Setup curriculum learning if enabled
        if (config.useCurriculum) {
            [self setupCurriculum];
        }
        
        // Initialize with base model path if provided
        if (!config.baseModelPath.empty()) {
            NSString* baseModelPath = [NSString stringWithUTF8String:config.baseModelPath.c_str()];
            [_modelPaths addObject:baseModelPath];
        }
    }
    return self;
}

- (void)dealloc {
    [self stopTraining];
    
    // Clean up resources
    if (_trainingEngine) {
        delete _trainingEngine;
        _trainingEngine = nullptr;
    }
    
    // Stop and release all headless runners
    for (HeadlessRunner* runner in _runners) {
        [runner stopEmulation];
    }
    [_runners removeAllObjects];
}

- (void)setupCurriculum {
    // Create curriculum stages with increasing difficulty
    
    // Stage 1: Very easy - AI mostly observes
    CurriculumStage* stage1 = [[CurriculumStage alloc] init];
    stage1.difficulty = 0.1f;
    stage1.requiredScore = 0.3f;
    stage1.description = @"Observation phase - basic understanding";
    
    // Stage 2: Easy - AI starts taking actions
    CurriculumStage* stage2 = [[CurriculumStage alloc] init];
    stage2.difficulty = 0.3f;
    stage2.requiredScore = 0.5f;
    stage2.description = @"Basic actions - learning movement patterns";
    
    // Stage 3: Medium - AI faces moderate challenges
    CurriculumStage* stage3 = [[CurriculumStage alloc] init];
    stage3.difficulty = 0.6f;
    stage3.requiredScore = 0.65f;
    stage3.description = @"Intermediate challenges - strategic development";
    
    // Stage 4: Hard - AI faces significant challenges
    CurriculumStage* stage4 = [[CurriculumStage alloc] init];
    stage4.difficulty = 0.8f;
    stage4.requiredScore = 0.75f;
    stage4.description = @"Advanced challenges - mastering core mechanics";
    
    // Stage 5: Expert - Full difficulty
    CurriculumStage* stage5 = [[CurriculumStage alloc] init];
    stage5.difficulty = 1.0f;
    stage5.requiredScore = 0.85f;
    stage5.description = @"Expert level - complete mastery";
    
    // Add stages to curriculum
    [_curriculumStages addObject:stage1];
    [_curriculumStages addObject:stage2];
    [_curriculumStages addObject:stage3];
    [_curriculumStages addObject:stage4];
    [_curriculumStages addObject:stage5];
    
    // Set initial difficulty from first stage
    if (_curriculumStages.count > 0) {
        _currentDifficulty = _curriculumStages[0].difficulty;
    }
}

- (BOOL)startTraining {
    if (_isTraining) {
        NSLog(@"Training is already in progress");
        return NO;
    }
    
    // Ensure we have Metal device and training engine
    if (!_metalDevice || !_trainingEngine) {
        NSLog(@"Metal device or training engine not available");
        return NO;
    }
    
    // Create headless runners
    [self createHeadlessRunners];
    
    // Start training
    _isTraining = YES;
    
    // Launch training thread
    dispatch_async(_trainingQueue, ^{
        [self trainingLoop];
    });
    
    NSLog(@"Self-play training started with %lu environments", (unsigned long)_runners.count);
    return YES;
}

- (void)stopTraining {
    _isTraining = NO;
    
    // Wait for training to stop
    dispatch_sync(_trainingQueue, ^{
        // This just ensures we wait for the training loop to exit
    });
    
    // Stop all runners
    for (HeadlessRunner* runner in _runners) {
        [runner stopEmulation];
    }
    
    NSLog(@"Self-play training stopped");
}

- (void)createHeadlessRunners {
    // Clear existing runners
    for (HeadlessRunner* runner in _runners) {
        [runner stopEmulation];
    }
    [_runners removeAllObjects];
    [_runnerScores removeAllObjects];
    
    // Create new headless runners
    for (int i = 0; i < _config.numParallelEnvs; i++) {
        HeadlessRunner* runner = [[HeadlessRunner alloc] init];
        
        // Configure runner
        [runner setRomPath:[NSString stringWithUTF8String:_config.romPath.c_str()]];
        [runner setAIEnabled:YES];
        
        // Set difficulty based on current curriculum stage
        [runner setDifficulty:_currentDifficulty];
        
        // If we have a model, use it
        if (_modelPaths.count > 0) {
            // Use latest model
            NSString* latestModel = _modelPaths.lastObject;
            [runner loadAIModel:latestModel];
        }
        
        // Initialize runner
        if ([runner initializeEmulation]) {
            [_runners addObject:runner];
            [_runnerScores addObject:@0.0f];
        } else {
            NSLog(@"Failed to initialize headless runner %d", i);
        }
    }
    
    NSLog(@"Created %lu headless runners", (unsigned long)_runners.count);
}

- (void)trainingLoop {
    NSLog(@"Training loop started");
    
    while (_isTraining) {
        @autoreleasepool {
            // Run episodes in parallel
            [self runParallelEpisodes];
            
            // Update model if we have enough data
            if (_trainingEngine->shouldUpdate()) {
                [self updateModel];
                _currentGeneration++;
                
                // Save model periodically
                if (_currentGeneration % _config.saveFrequency == 0) {
                    [self saveCurrentModel];
                }
                
                // Update curriculum if enabled
                if (_config.useCurriculum) {
                    [self updateCurriculum];
                }
                
                // Log progress
                NSLog(@"Generation %d completed, average score: %.2f, difficulty: %.2f", 
                      _currentGeneration, [self getAverageScore], _currentDifficulty);
            }
        }
    }
    
    NSLog(@"Training loop ended");
}

- (void)runParallelEpisodes {
    // Reset scores
    for (int i = 0; i < _runnerScores.count; i++) {
        _runnerScores[i] = @0.0f;
    }
    
    // Set up parallel tasks
    NSCountedSet* completedEpisodes = [[NSCountedSet alloc] init];
    NSLock* completionLock = [[NSLock alloc] init];
    
    // Launch parallel episodes
    for (int i = 0; i < _runners.count; i++) {
        // Create block operation for this runner
        NSBlockOperation* operation = [NSBlockOperation blockOperationWithBlock:^{
            HeadlessRunner* runner = _runners[i];
            
            // Run a full episode
            EpisodeResult result = [runner runEpisode:_config.maxEpisodeLength];
            
            // Record experience in training engine
            for (ExperienceEntry* entry in result.experiences) {
                _trainingEngine->addExperience(
                    entry.state,
                    entry.frame,
                    entry.action,
                    entry.reward,
                    entry.done
                );
            }
            
            // Update score
            [completionLock lock];
            _runnerScores[i] = @(result.totalReward);
            [completedEpisodes addObject:@(i)];
            [completionLock unlock];
        }];
        
        // Add operation to queue
        [_distributedQueue addOperation:operation];
    }
    
    // Wait for all episodes to complete
    [_distributedQueue waitUntilAllOperationsAreFinished];
}

- (void)updateModel {
    if (!_trainingEngine) {
        return;
    }
    
    // Perform policy update
    _trainingEngine->update();
    
    // Update runners with latest policy
    AITorchPolicy* policy = _trainingEngine->getLatestPolicy();
    if (policy) {
        for (HeadlessRunner* runner in _runners) {
            [runner updateAIPolicy:policy];
        }
    }
}

- (void)saveCurrentModel {
    if (!_trainingEngine) {
        return;
    }
    
    // Generate model path for this generation
    NSString* modelDir = [NSString stringWithUTF8String:_config.modelSavePath.c_str()];
    NSString* modelPath = [modelDir stringByAppendingPathComponent:
                          [NSString stringWithFormat:@"model_gen%d.coreml", _currentGeneration]];
    
    // Ensure directory exists
    NSFileManager* fileManager = [NSFileManager defaultManager];
    if (![fileManager fileExistsAtPath:modelDir]) {
        [fileManager createDirectoryAtPath:modelDir withIntermediateDirectories:YES attributes:nil error:nil];
    }
    
    // Save model
    if (_trainingEngine->saveModel([modelPath UTF8String])) {
        NSLog(@"Saved model for generation %d to %@", _currentGeneration, modelPath);
        [_modelPaths addObject:modelPath];
    } else {
        NSLog(@"Failed to save model for generation %d", _currentGeneration);
    }
}

- (void)updateCurriculum {
    if (_curriculumStages.count == 0 || _currentStageIndex >= _curriculumStages.count) {
        return;
    }
    
    // Get current stage
    CurriculumStage* currentStage = _curriculumStages[_currentStageIndex];
    
    // Calculate average score
    float avgScore = [self getAverageScore];
    
    // Check if we should advance to the next stage
    if (avgScore >= currentStage.requiredScore && _currentStageIndex < _curriculumStages.count - 1) {
        _currentStageIndex++;
        CurriculumStage* nextStage = _curriculumStages[_currentStageIndex];
        _currentDifficulty = nextStage.difficulty;
        
        NSLog(@"Advancing to curriculum stage %d: %@, difficulty: %.2f", 
              _currentStageIndex + 1, nextStage.description, _currentDifficulty);
        
        // Update difficulty for all runners
        for (HeadlessRunner* runner in _runners) {
            [runner setDifficulty:_currentDifficulty];
        }
    }
}

- (float)getAverageScore {
    if (_runnerScores.count == 0) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (NSNumber* score in _runnerScores) {
        sum += [score floatValue];
    }
    
    return sum / _runnerScores.count;
}

- (SelfPlayStatus)getStatus {
    SelfPlayStatus status;
    
    status.isTraining = _isTraining;
    status.currentGeneration = _currentGeneration;
    status.numParallelEnvs = (int)_runners.count;
    status.averageScore = [self getAverageScore];
    status.currentDifficulty = _currentDifficulty;
    
    if (_config.useCurriculum && _currentStageIndex < _curriculumStages.count) {
        status.curriculumStage = _currentStageIndex + 1;
        status.curriculumDescription = [_curriculumStages[_currentStageIndex].description UTF8String];
    } else {
        status.curriculumStage = 0;
        status.curriculumDescription = "";
    }
    
    return status;
}

@end

// HeadlessRunner implementation
@implementation HeadlessRunner {
    NSString* _romPath;
    BOOL _aiEnabled;
    float _difficulty;
    BOOL _isRunning;
    AITorchPolicy* _aiPolicy;
    NSString* _aiModelPath;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _romPath = nil;
        _aiEnabled = NO;
        _difficulty = 0.5f;
        _isRunning = NO;
        _aiPolicy = nullptr;
        _aiModelPath = nil;
    }
    return self;
}

- (void)dealloc {
    [self stopEmulation];
}

- (void)setRomPath:(NSString*)path {
    _romPath = path;
}

- (void)setAIEnabled:(BOOL)enabled {
    _aiEnabled = enabled;
}

- (void)setDifficulty:(float)difficulty {
    _difficulty = difficulty;
}

- (BOOL)initializeEmulation {
    // Initialize the FBNeo core for headless operation
    if (BurnLibInit() != 0) {
        NSLog(@"Failed to initialize BurnLib");
        return NO;
    }
    
    // Load ROM if provided
    if (_romPath && _romPath.length > 0) {
        const char* romPathCStr = [_romPath UTF8String];
        if (Metal_LoadROM(romPathCStr) != 0) {
            NSLog(@"Failed to load ROM: %@", _romPath);
            BurnLibExit();
            return NO;
        }
    } else {
        NSLog(@"No ROM path provided");
        BurnLibExit();
        return NO;
    }
    
    // Initialize AI policy if enabled
    if (_aiEnabled) {
        if (!_aiPolicy) {
            _aiPolicy = new AITorchPolicy();
        }
        
        // Load model if available
        if (_aiModelPath && _aiModelPath.length > 0) {
            if (!_aiPolicy->LoadModel([_aiModelPath UTF8String])) {
                NSLog(@"Failed to load AI model: %@", _aiModelPath);
                delete _aiPolicy;
                _aiPolicy = nullptr;
                _aiEnabled = NO;
            }
        }
    }
    
    _isRunning = YES;
    return YES;
}

- (void)stopEmulation {
    if (_isRunning) {
        BurnDrvExit();
        BurnLibExit();
        _isRunning = NO;
    }
    
    // Clean up AI policy
    if (_aiPolicy) {
        delete _aiPolicy;
        _aiPolicy = nullptr;
    }
}

- (BOOL)loadAIModel:(NSString*)modelPath {
    if (!_aiEnabled) {
        return NO;
    }
    
    if (!_aiPolicy) {
        _aiPolicy = new AITorchPolicy();
    }
    
    if (_aiPolicy->LoadModel([modelPath UTF8String])) {
        _aiModelPath = modelPath;
        return YES;
    }
    
    return NO;
}

- (void)updateAIPolicy:(AITorchPolicy*)policy {
    // Update with shared policy (useful for distributed training)
    if (_aiPolicy) {
        delete _aiPolicy;
    }
    
    // Clone the policy
    _aiPolicy = policy->Clone();
}

- (EpisodeResult)runEpisode:(int)maxFrames {
    EpisodeResult result;
    result.totalReward = 0.0f;
    
    if (!_isRunning) {
        return result;
    }
    
    // Reset game state
    BurnDrvReset();
    
    // Run for the specified number of frames
    int gameOver = 0;
    for (int frame = 0; frame < maxFrames && !gameOver; frame++) {
        // Get current game state and frame buffer
        GameState gameState;
        ExperienceEntry* entry = [[ExperienceEntry alloc] init];
        
        // Extract the game state
        entry.state = ExtractGameState();
        
        // Capture frame buffer
        AIInputFrame inputFrame;
        inputFrame.frameBuffer = pBurnDraw;
        inputFrame.width = BurnDrvGetVisibleSize()->w;
        inputFrame.height = BurnDrvGetVisibleSize()->h;
        inputFrame.timestamp = frame;
        inputFrame.gameState = entry.state;
        entry.frame = inputFrame;
        
        // Run AI inference if enabled
        AIOutputAction aiAction;
        if (_aiEnabled && _aiPolicy) {
            _aiPolicy->RunInference(inputFrame, aiAction);
            
            // Apply difficulty scaling - sometimes take random actions instead
            if (arc4random_uniform(100) < _difficulty * 100) {
                // Use AI action
                ApplyInputToGame(aiAction);
            } else {
                // Take random action
                AIOutputAction randomAction;
                GenerateRandomAction(randomAction);
                ApplyInputToGame(randomAction);
                aiAction = randomAction;
            }
            
            entry.action = aiAction;
        } else {
            // Take random action if AI not available
            GenerateRandomAction(aiAction);
            ApplyInputToGame(aiAction);
            entry.action = aiAction;
        }
        
        // Run frame
        BurnDrvFrame();
        
        // Extract reward and game over state
        float reward = CalculateReward(entry.state);
        entry.reward = reward;
        result.totalReward += reward;
        
        // Check if game is over
        gameOver = IsGameOver(entry.state);
        entry.done = gameOver;
        
        // Add entry to experience buffer
        [result.experiences addObject:entry];
    }
    
    return result;
}

- (GameState)ExtractGameState {
    GameState state;
    
    // Implement game-specific state extraction here
    // This needs to be customized based on the game
    
    return state;
}

- (void)GenerateRandomAction:(AIOutputAction&)action {
    // Generate random actions
    for (int i = 0; i < MAX_ACTIONS; i++) {
        action.actionProbabilities[i] = (float)arc4random_uniform(100) / 100.0f;
    }
    
    // Set some random buttons
    int numButtons = MIN(MAX_BUTTONS, 8);
    for (int i = 0; i < numButtons; i++) {
        action.buttons[i] = (arc4random_uniform(100) < 15) ? 1 : 0; // 15% chance of button press
    }
    
    // Set random directions
    action.up = (arc4random_uniform(100) < 25) ? 1 : 0;
    action.down = (arc4random_uniform(100) < 25) ? 1 : 0;
    action.left = (arc4random_uniform(100) < 25) ? 1 : 0;
    action.right = (arc4random_uniform(100) < 25) ? 1 : 0;
    
    // Avoid conflicting directions
    if (action.up && action.down) {
        action.down = 0;
    }
    if (action.left && action.right) {
        action.right = 0;
    }
}

- (void)ApplyInputToGame:(const AIOutputAction&)action {
    // Convert AI action to game input
    // This needs to be customized based on the game input system
    
    // Example: Convert to FBNeo input system
    // This is a simplified version - actual implementation depends on the game
    uint8_t directionalBits = action.ToDirectionalBits();
    uint16_t buttonBits = action.ToButtonBits();
    
    // Apply to game input system
    extern void ApplyInputBitsToGame(uint8_t directionalBits, uint16_t buttonBits, int playerIndex);
    ApplyInputBitsToGame(directionalBits, buttonBits, 0); // Player 1
}

- (float)CalculateReward:(const GameState&)state {
    // Implement game-specific reward calculation
    // This needs to be customized based on the game
    
    // Example: Simple reward based on score increase
    static int lastScore = 0;
    int scoreDelta = state.player1Score - lastScore;
    lastScore = state.player1Score;
    
    // Base reward
    float reward = 0.01f; // Small positive reward for surviving
    
    // Reward for score increases
    if (scoreDelta > 0) {
        reward += scoreDelta * 0.1f;
    }
    
    // Penalty for taking damage
    if (state.player1Health < state.player1PrevHealth) {
        reward -= 1.0f;
    }
    
    // Bonus for defeating enemies
    if (state.enemiesDefeated > 0) {
        reward += state.enemiesDefeated * 2.0f;
    }
    
    // Bonus for completing level
    if (state.levelCompleted) {
        reward += 10.0f;
    }
    
    return reward;
}

- (BOOL)IsGameOver:(const GameState&)state {
    // Determine if the game is over based on game state
    // This needs to be customized based on the game
    
    // Example: Game over when player health reaches zero or level is completed
    return (state.player1Health <= 0 || state.gameOver || state.levelCompleted);
}

@end

// CurriculumStage implementation
@implementation CurriculumStage
@end

// ExperienceEntry implementation
@implementation ExperienceEntry
@end

// C++ wrapper for the SelfPlayManager
bool SelfPlayManagerWrapper::initialize(const SelfPlayConfig& config) {
    NSString* romPath = [NSString stringWithUTF8String:config.romPath.c_str()];
    NSString* modelSavePath = [NSString stringWithUTF8String:config.modelSavePath.c_str()];
    NSString* baseModelPath = config.baseModelPath.empty() ? nil : [NSString stringWithUTF8String:config.baseModelPath.c_str()];
    
    selfPlayManager = [[SelfPlayManager alloc] initWithConfig:config];
    return (selfPlayManager != nil);
}

bool SelfPlayManagerWrapper::startTraining() {
    if (!selfPlayManager) {
        return false;
    }
    
    return [selfPlayManager startTraining];
}

void SelfPlayManagerWrapper::stopTraining() {
    if (selfPlayManager) {
        [selfPlayManager stopTraining];
    }
}

SelfPlayStatus SelfPlayManagerWrapper::getStatus() {
    if (!selfPlayManager) {
        SelfPlayStatus emptyStatus;
        emptyStatus.isTraining = false;
        emptyStatus.currentGeneration = 0;
        emptyStatus.numParallelEnvs = 0;
        emptyStatus.averageScore = 0.0f;
        emptyStatus.currentDifficulty = 0.0f;
        emptyStatus.curriculumStage = 0;
        emptyStatus.curriculumDescription = "";
        return emptyStatus;
    }
    
    return [selfPlayManager getStatus];
} 