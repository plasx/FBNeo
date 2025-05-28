# FBNeo AI Models

This directory contains AI models for use with the FBNeo Metal AI implementation.

## Model Format

Models should be in TorchScript (.pt) format, exported from PyTorch using the `torch.jit.script` or `torch.jit.trace` methods.

## Expected Models

The system looks for the following models:

- `default_model.pt` - Default AI model used when no specific model is selected
- `[game]_[difficulty].pt` - Game-specific models with different difficulty levels (e.g., `sf2_medium.pt`)

## Model Structure

Each model should accept an `AIInputFrame` tensor as input and produce an `AIOutputAction` tensor as output.

### Input Tensor Format

The input tensor is a 1D array representing the game state, including:
- Player positions
- Health values
- Move states
- Frame advantage
- Distance between players
- Attack properties
- Game-specific state variables

### Output Tensor Format

The output tensor is a vector indicating:
- Button presses (0-1 for each button)
- Button releases (0-1 for each button)
- Joystick direction (multi-class or continuous values)
- Action confidence (0-1)

## Creating Custom Models

To create custom AI models, use the training scripts in the `tools/ai_training` directory. These provide utilities for:
- Collecting training data
- Training various model architectures
- Exporting to TorchScript format
- Testing and validating models

## Example Usage

```python
# Export a PyTorch model to TorchScript
import torch

class FBNeoAIModel(torch.nn.Module):
    def __init__(self):
        super(FBNeoAIModel, self).__init__()
        self.fc1 = torch.nn.Linear(128, 64)
        self.fc2 = torch.nn.Linear(64, 32)
        self.output = torch.nn.Linear(32, 10)  # Output size depends on action space
        
    def forward(self, x):
        x = torch.relu(self.fc1(x))
        x = torch.relu(self.fc2(x))
        return torch.sigmoid(self.output(x))

# Create and save the model
model = FBNeoAIModel()
scripted_model = torch.jit.script(model)
scripted_model.save("models/custom_model.pt")
```

Place the resulting .pt file in this directory to use it with FBNeo's AI system. 