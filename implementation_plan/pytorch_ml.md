# PyTorch/ML Implementation

This document outlines the implementation plan for replacing stub functions in the PyTorch/ML components of FBNeo Metal.

## PyTorch to CoreML Conversion

```python
# Implementation in Python for torch_to_coreml.py:
import torch
import torch.jit
import coremltools as ct
import sys
import argparse

def convert_model(torch_path, coreml_path, input_shape=(1, 4, 84, 84), 
                 game_id="generic", model_type="policy"):
    """Convert a PyTorch model to CoreML format."""
    
    print(f"Converting model: {torch_path} -> {coreml_path}")
    print(f"Input shape: {input_shape}")
    
    try:
        # Load the PyTorch model
        model = torch.jit.load(torch_path)
        
        # Create example input
        example_input = torch.rand(*input_shape)
        
        # Trace the model for CoreML conversion
        traced_model = torch.jit.trace(model, example_input)
        
        # Convert to CoreML
        coreml_model = ct.convert(
            traced_model,
            inputs=[ct.TensorType(shape=input_shape, name="input")],
            compute_precision=ct.precision.FLOAT16,  # Use lower precision for performance
            minimum_deployment_target=ct.target.macOS13
        )
        
        # Add metadata
        coreml_model.user_defined_metadata["game_id"] = game_id
        coreml_model.user_defined_metadata["model_type"] = model_type
        coreml_model.user_defined_metadata["source"] = "PyTorch"
        coreml_model.user_defined_metadata["input_shape"] = str(input_shape)
        
        # Save the model
        coreml_model.save(coreml_path)
        print(f"Conversion successful: {coreml_path}")
        return True
        
    except Exception as e:
        print(f"Error converting model: {e}")
        return False

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert PyTorch model to CoreML")
    parser.add_argument("torch_path", help="Path to PyTorch model")
    parser.add_argument("coreml_path", help="Path to save CoreML model")
    parser.add_argument("--input-shape", default="1,4,84,84", 
                       help="Input shape (comma-separated)")
    parser.add_argument("--game-id", default="generic", help="Game identifier")
    parser.add_argument("--model-type", default="policy", help="Model type")
    
    args = parser.parse_args()
    
    # Parse input shape
    input_shape = tuple(map(int, args.input_shape.split(",")))
    
    # Convert the model
    success = convert_model(args.torch_path, args.coreml_path, 
                           input_shape, args.game_id, args.model_type)
    
    sys.exit(0 if success else 1)
```

## Objective-C++ Bridge for PyTorch Conversion

```objective-c
// Implementation for TorchToCoreMLConverter class

@implementation TorchToCoreMLConverter

- (instancetype)init {
    self = [super init];
    if (self) {
        // Check if Python dependencies are available
        _hasPythonDependencies = [self checkPythonDependencies];
    }
    return self;
}

- (BOOL)checkPythonDependencies {
    NSTask *task = [[NSTask alloc] init];
    [task setLaunchPath:@"/usr/bin/env"];
    [task setArguments:@[@"python3", @"-c", @"import torch, torch.jit, coremltools"]];
    
    NSPipe *pipe = [NSPipe pipe];
    [task setStandardOutput:pipe];
    [task setStandardError:pipe];
    
    NSFileHandle *file = [pipe fileHandleForReading];
    
    @try {
        [task launch];
        [task waitUntilExit];
        
        int status = [task terminationStatus];
        return status == 0;
    } @catch (NSException *exception) {
        NSLog(@"Error checking Python dependencies: %@", exception);
        return NO;
    }
}

- (BOOL)convertModel:(NSString *)torchPath
           toCoreML:(NSString *)coreMLPath
         inputShape:(NSArray<NSNumber *> *)inputShape
            gameID:(NSString *)gameID {
    
    // If Python dependencies are not available, fall back to bundled converter
    if (!_hasPythonDependencies) {
        NSLog(@"Python dependencies not available, using bundled converter");
        return [self convertModelWithBundledScript:torchPath
                                         toCoreML:coreMLPath
                                       inputShape:inputShape
                                          gameID:gameID];
    }
    
    // Convert input shape to comma-separated string
    NSMutableString *shapeString = [NSMutableString string];
    for (NSNumber *dim in inputShape) {
        if (shapeString.length > 0) {
            [shapeString appendString:@","];
        }
        [shapeString appendFormat:@"%@", dim];
    }
    
    // Create Python command
    NSTask *task = [[NSTask alloc] init];
    [task setLaunchPath:@"/usr/bin/env"];
    [task setArguments:@[
        @"python3",
        [[NSBundle mainBundle] pathForResource:@"torch_to_coreml" ofType:@"py"],
        torchPath,
        coreMLPath,
        @"--input-shape", shapeString,
        @"--game-id", gameID
    ]];
    
    NSPipe *pipe = [NSPipe pipe];
    [task setStandardOutput:pipe];
    [task setStandardError:pipe];
    
    NSFileHandle *file = [pipe fileHandleForReading];
    
    @try {
        [task launch];
        
        NSData *data = [file readDataToEndOfFile];
        NSString *output = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        NSLog(@"Conversion output: %@", output);
        
        [task waitUntilExit];
        
        int status = [task terminationStatus];
        return status == 0;
    } @catch (NSException *exception) {
        NSLog(@"Error converting model: %@", exception);
        return NO;
    }
}

- (BOOL)convertModelWithBundledScript:(NSString *)torchPath
                             toCoreML:(NSString *)coreMLPath
                           inputShape:(NSArray<NSNumber *> *)inputShape
                              gameID:(NSString *)gameID {
    // Create a temporary directory for the conversion script
    NSString *tempDir = [NSTemporaryDirectory() stringByAppendingPathComponent:@"torch_to_coreml"];
    [[NSFileManager defaultManager] createDirectoryAtPath:tempDir
                              withIntermediateDirectories:YES
                                               attributes:nil
                                                    error:nil];
    
    // Create the conversion script
    NSString *scriptPath = [tempDir stringByAppendingPathComponent:@"torch_to_coreml.py"];
    
    NSString *script = @"import torch\n"
        "import torch.jit\n"
        "import coremltools as ct\n"
        "import sys\n"
        "import numpy as np\n"
        "\n"
        "def convert_model(torch_path, coreml_path, input_shape, game_id):\n"
        "    try:\n"
        "        print(f'Converting model: {torch_path} -> {coreml_path}')\n"
        "        # Load the model\n"
        "        model = torch.jit.load(torch_path)\n"
        "        \n"
        "        # Example input for tracing\n"
        "        example_input = torch.rand(*input_shape)\n"
        "        \n"
        "        # Trace the model\n"
        "        traced_model = torch.jit.trace(model, example_input)\n"
        "        \n"
        "        # Convert to CoreML\n"
        "        mlmodel = ct.convert(\n"
        "            traced_model,\n"
        "            inputs=[ct.TensorType(shape=input_shape)],\n"
        "            compute_precision=ct.precision.FLOAT16\n"
        "        )\n"
        "        \n"
        "        # Add metadata\n"
        "        mlmodel.user_defined_metadata['game_id'] = game_id\n"
        "        mlmodel.user_defined_metadata['source'] = 'PyTorch'\n"
        "        \n"
        "        # Save the model\n"
        "        mlmodel.save(coreml_path)\n"
        "        print('Conversion successful')\n"
        "        return True\n"
        "    except Exception as e:\n"
        "        print(f'Error: {e}')\n"
        "        return False\n"
        "\n"
        "if __name__ == '__main__':\n"
        "    torch_path = sys.argv[1]\n"
        "    coreml_path = sys.argv[2]\n"
        "    input_shape = tuple(map(int, sys.argv[3].split(',')))\n"
        "    game_id = sys.argv[4]\n"
        "    \n"
        "    success = convert_model(torch_path, coreml_path, input_shape, game_id)\n"
        "    sys.exit(0 if success else 1)\n";
    
    [script writeToFile:scriptPath atomically:YES encoding:NSUTF8StringEncoding error:nil];
    
    // Convert input shape to comma-separated string
    NSMutableString *shapeString = [NSMutableString string];
    for (NSNumber *dim in inputShape) {
        if (shapeString.length > 0) {
            [shapeString appendString:@","];
        }
        [shapeString appendFormat:@"%@", dim];
    }
    
    // Run the script
    NSTask *task = [[NSTask alloc] init];
    [task setLaunchPath:@"/usr/bin/env"];
    [task setArguments:@[
        @"python3",
        scriptPath,
        torchPath,
        coreMLPath,
        shapeString,
        gameID
    ]];
    
    NSPipe *pipe = [NSPipe pipe];
    [task setStandardOutput:pipe];
    [task setStandardError:pipe];
    
    NSFileHandle *file = [pipe fileHandleForReading];
    
    @try {
        [task launch];
        
        NSData *data = [file readDataToEndOfFile];
        NSString *output = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        NSLog(@"Conversion output: %@", output);
        
        [task waitUntilExit];
        
        int status = [task terminationStatus];
        return status == 0;
    } @catch (NSException *exception) {
        NSLog(@"Error converting model: %@", exception);
        return NO;
    }
}

@end
```

## Reinforcement Learning Environment

```cpp
// Implementation for reinforcement learning environment:

class RLEnvironment {
public:
    RLEnvironment() : frameSkip(4), rewardScale(1.0f), done(false) {}
    
    // Initialize the environment
    bool initialize(const std::string& romPath, const std::string& gameId) {
        // Initialize the headless runner
        if (!runner.initialize(headlessConfig)) {
            return false;
        }
        
        // Start the game
        if (!runner.start(romPath, gameId)) {
            return false;
        }
        
        // Set up callbacks
        runner.setActionCallback([this](uint32_t* inputs) {
            // Apply the current actions to the inputs
            applyActionsToInputs(inputs);
        });
        
        runner.setFrameCallback([this](uint8_t* pixels, int width, int height, int pitch) {
            // Store the current observation
            updateObservation(pixels, width, height, pitch);
        });
        
        runner.setRewardCallback([this]() {
            // Calculate the current reward
            return calculateReward();
        });
        
        return true;
    }
    
    // Reset the environment
    void reset() {
        // Reset the game state
        runner.reset();
        
        // Clear the current actions
        currentActions.action_count = 0;
        
        // Reset the done flag
        done = false;
    }
    
    // Step the environment with the given actions
    float step(const AIActions& actions) {
        // Store the actions
        currentActions = actions;
        
        // Run the specified number of frames
        runner.runFrames(frameSkip);
        
        // Check if the episode is done
        done = checkIfDone();
        
        // Return the reward
        return currentReward * rewardScale;
    }
    
    // Check if the episode is done
    bool isDone() const {
        return done;
    }
    
    // Get the current observation
    const AIFrameData& getObservation() const {
        return currentObservation;
    }
    
private:
    // Headless runner
    HeadlessRunner runner;
    
    // Configuration
    HeadlessConfig headlessConfig;
    
    // Current state
    AIActions currentActions;
    AIFrameData currentObservation;
    float currentReward;
    
    // Parameters
    int frameSkip;
    float rewardScale;
    bool done;
    
    // Apply the current actions to the inputs
    void applyActionsToInputs(uint32_t* inputs) {
        // Initialize inputs to 0
        memset(inputs, 0, sizeof(uint32_t) * 8);
        
        // Apply each action
        for (uint32_t i = 0; i < currentActions.action_count; i++) {
            const AIAction& action = currentActions.actions[i];
            
            if (action.type == AI_ACTION_BUTTON) {
                // Set the corresponding bit for the button
                inputs[action.input_id / 32] |= (1 << (action.input_id % 32));
            } else if (action.type == AI_ACTION_JOYSTICK) {
                // Map analog values to digital for now
                if (action.value > 0.5f) {
                    inputs[action.input_id / 32] |= (1 << (action.input_id % 32));
                }
            }
        }
    }
    
    // Update the current observation
    void updateObservation(uint8_t* pixels, int width, int height, int pitch) {
        // Allocate memory for the observation if needed
        if (!currentObservation.data) {
            currentObservation.data = malloc(width * height * 4);
            currentObservation.width = width;
            currentObservation.height = height;
            currentObservation.channels = 4;
            currentObservation.size = width * height * 4;
        }
        
        // Copy the pixel data
        for (int y = 0; y < height; y++) {
            memcpy(
                (uint8_t*)currentObservation.data + (y * width * 4),
                pixels + (y * pitch),
                width * 4
            );
        }
    }
    
    // Calculate the current reward
    float calculateReward() {
        // Get the memory values
        int p1Health = runner.readMemory(0x1000, 1); // Example offset for player 1 health
        int p2Health = runner.readMemory(0x1001, 1); // Example offset for player 2 health
        
        // Calculate reward based on health difference
        currentReward = (p1Health - p2Health) / 100.0f;
        
        return currentReward;
    }
    
    // Check if the episode is done
    bool checkIfDone() {
        // Get the game state
        int gameState = runner.readMemory(0x1002, 1); // Example offset for game state
        
        // Check if the round is over
        return gameState == 0; // Example value for round over
    }
};
```

## Self-Play Implementation

```cpp
// Implementation for self-play:

class SelfPlayManager {
public:
    SelfPlayManager() : maxEpisodes(1000), episodesCompleted(0) {}
    
    // Initialize the manager
    bool initialize(const std::string& romPath, const std::string& gameId) {
        // Create two environments
        if (!env1.initialize(romPath, gameId) || !env2.initialize(romPath, gameId)) {
            return false;
        }
        
        // Load the models
        if (!loadModels()) {
            return false;
        }
        
        return true;
    }
    
    // Load the models for self-play
    bool loadModels() {
        // Load model 1
        if (!model1.loadFromFile("models/model1.mlmodel")) {
            return false;
        }
        
        // Load model 2
        if (!model2.loadFromFile("models/model2.mlmodel")) {
            return false;
        }
        
        return true;
    }
    
    // Run self-play
    void runSelfPlay() {
        // Reset the environments
        env1.reset();
        env2.reset();
        
        // Reset episode statistics
        episodeReward1 = 0.0f;
        episodeReward2 = 0.0f;
        
        // Run until both environments are done
        while (!env1.isDone() && !env2.isDone()) {
            // Get observations
            const AIFrameData& obs1 = env1.getObservation();
            const AIFrameData& obs2 = env2.getObservation();
            
            // Get actions from models
            AIActions actions1, actions2;
            model1.predict(obs1, actions1);
            model2.predict(obs2, actions2);
            
            // Take a step in each environment
            float reward1 = env1.step(actions1);
            float reward2 = env2.step(actions2);
            
            // Update episode rewards
            episodeReward1 += reward1;
            episodeReward2 += reward2;
        }
        
        // Update counters
        episodesCompleted++;
        
        // Log results
        printf("Episode %d: Model 1 reward = %.2f, Model 2 reward = %.2f\n",
              episodesCompleted, episodeReward1, episodeReward2);
        
        // Store the results for analysis
        results.push_back({episodeReward1, episodeReward2});
    }
    
    // Run a tournament
    void runTournament(int numEpisodes) {
        maxEpisodes = numEpisodes;
        episodesCompleted = 0;
        results.clear();
        
        while (episodesCompleted < maxEpisodes) {
            runSelfPlay();
        }
        
        // Analyze the results
        analyzeResults();
    }
    
    // Analyze the results
    void analyzeResults() {
        int wins1 = 0, wins2 = 0, draws = 0;
        
        for (const auto& result : results) {
            if (result.first > result.second) {
                wins1++;
            } else if (result.second > result.first) {
                wins2++;
            } else {
                draws++;
            }
        }
        
        printf("Tournament results: Model 1 wins = %d, Model 2 wins = %d, Draws = %d\n",
              wins1, wins2, draws);
    }
    
private:
    // Environments
    RLEnvironment env1, env2;
    
    // Models
    PolicyModel model1, model2;
    
    // Episode statistics
    float episodeReward1, episodeReward2;
    
    // Tournament parameters
    int maxEpisodes;
    int episodesCompleted;
    
    // Results
    std::vector<std::pair<float, float>> results;
};
```

## Implementation Steps

1. **PyTorch to CoreML Conversion**
   - Implement Python conversion script
   - Create Objective-C++ wrapper for conversion
   - Add support for different model architectures
   - Include metadata in converted models

2. **Reinforcement Learning Environment**
   - Implement headless environment for training
   - Create observation and action space handling
   - Add reward calculation
   - Implement episode management

3. **Self-Play Implementation**
   - Create tournament system for model comparison
   - Implement results tracking and analysis
   - Add support for progressive learning
   - Create visualization tools for game states

4. **Training Pipeline**
   - Implement data collection from gameplay
   - Create model training utilities
   - Add hyperparameter optimization
   - Implement distributed training support

5. **Integration with UI**
   - Add model selection interface
   - Create training configuration panel
   - Implement visualization of AI actions
   - Add real-time monitoring of model performance

## Testing Strategy

For each component:
1. Create small test models for quick validation
2. Verify model conversion accuracy
3. Test learning on simplified games
4. Compare against existing RL implementations
5. Benchmark performance on different hardware

## Dependencies

- Python 3.7+
- PyTorch 1.7+
- CoreMLTools 4.0+
- FBNeo headless mode
- Metal Performance Shaders 