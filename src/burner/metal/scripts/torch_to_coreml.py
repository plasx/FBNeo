#!/usr/bin/env python3
# PyTorch to CoreML Converter for FBNeo
# This script converts PyTorch models to CoreML format for use with FBNeo

import os
import sys
import argparse
import json
import traceback
import numpy as np
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Union

# Try to import torch and coremltools
HAS_TORCH = False
HAS_COREML = False

try:
    import torch
    import torch.nn as nn
    import torch.jit
    HAS_TORCH = True
except ImportError:
    print("Warning: PyTorch is not available. Will create a minimal test model instead.")

try:
    import coremltools as ct
    from coremltools.models.neural_network import quantization_utils
    HAS_COREML = True
except ImportError:
    print("Error: CoreMLTools is required for conversion")
    print("Install with: pip install coremltools")
    sys.exit(1)

# Define the CNN model structure used for FBNeo AI
class FBNeoModel(nn.Module):
    def __init__(self, input_channels=3, input_height=224, input_width=224, num_actions=10):
        super(FBNeoModel, self).__init__()
        
        # Convolutional layers
        self.conv1 = nn.Conv2d(input_channels, 32, kernel_size=8, stride=4)
        self.conv2 = nn.Conv2d(32, 64, kernel_size=4, stride=2)
        self.conv3 = nn.Conv2d(64, 64, kernel_size=3, stride=1)
        
        # Calculate the size after convolutions
        conv_height = ((input_height - 8) // 4 + 1)
        conv_height = ((conv_height - 4) // 2 + 1)
        conv_height = ((conv_height - 3) // 1 + 1)
        
        conv_width = ((input_width - 8) // 4 + 1)
        conv_width = ((conv_width - 4) // 2 + 1)
        conv_width = ((conv_width - 3) // 1 + 1)
        
        flat_size = 64 * conv_height * conv_width
        
        # Fully connected layers
        self.fc1 = nn.Linear(flat_size, 512)
        
        # Action logits
        self.action_head = nn.Linear(512, num_actions)
        
        # Value output
        self.value_head = nn.Linear(512, 1)
        
        # Activation functions
        self.relu = nn.ReLU()
        
    def forward(self, x):
        # Normalize input if needed (0-255 -> 0-1)
        if x.max() > 1.0:
            x = x / 255.0
        
        # Convolutional layers
        x = self.relu(self.conv1(x))
        x = self.relu(self.conv2(x))
        x = self.relu(self.conv3(x))
        
        # Flatten
        x = x.view(x.size(0), -1)
        
        # Fully connected
        x = self.relu(self.fc1(x))
        
        # Action and value outputs
        action_logits = self.action_head(x)
        value = self.value_head(x)
        
        return action_logits, value

def create_dummy_model(output_path, game_type='generic'):
    """
    Create a dummy CoreML model when PyTorch is not available
    """
    print(f"Creating minimal test model for {game_type}")
    
    # Create a simple model that takes a 224x224 image and outputs 8 values
    input_features = [
        ct.ImageType(name="image", shape=(1, 3, 224, 224,), 
                     bias=[-1, -1, -1], scale=1/127.0, color_layout='RGB')
    ]
    
    output_features = [
        ct.TensorType(name="output", shape=(8,))
    ]
    
    # Create a simple model with random weights
    builder = ct.models.neural_network.NeuralNetworkBuilder(
        input_features=input_features,
        output_features=output_features,
        mode='classifier'
    )
    
    # Add a flatten layer
    builder.add_flatten(
        name="flatten",
        input_name="image",
        output_name="flattened",
        mode=0
    )
    
    # Add a fully connected layer
    builder.add_inner_product(
        name="dense",
        input_name="flattened",
        output_name="output",
        W=np.random.rand(8, 3*224*224).astype(np.float32),
        b=np.random.rand(8).astype(np.float32),
        has_bias=True,
        input_channels=3*224*224,
        output_channels=8
    )
    
    # Create the model
    model = ct.models.MLModel(builder.spec)
    
    # Add metadata
    model.author = "FBNeo AI Generator"
    model.license = "MIT"
    model.short_description = f"Test model for FBNeo {game_type}"
    model.version = "1.0"
    
    # Save the model
    model.save(output_path)
    print(f"Model saved to {output_path}")
    
    return True

def trace_model(model: nn.Module, input_shape: Tuple[int, ...]) -> torch.jit.ScriptModule:
    """Trace a PyTorch model to make it exportable"""
    model.eval()
    example_input = torch.rand(*input_shape)
    return torch.jit.trace(model, example_input)

def convert_to_coreml(traced_model: torch.jit.ScriptModule, 
                     input_shape: Tuple[int, ...],
                     output_path: str,
                     compute_units: str = "ALL",
                     metadata: Dict[str, str] = None) -> bool:
    """Convert a traced PyTorch model to CoreML format"""
    
    # Set up compute units
    compute_unit_map = {
        "CPU": ct.ComputeUnit.CPU_ONLY,
        "GPU": ct.ComputeUnit.CPU_AND_GPU,
        "ANE": ct.ComputeUnit.ALL,  # Neural Engine included
        "ALL": ct.ComputeUnit.ALL
    }
    
    compute_unit = compute_unit_map.get(compute_units.upper(), ct.ComputeUnit.ALL)
    
    # Set up inputs
    inputs = [ct.TensorType(name="input", shape=input_shape)]
    
    # Convert the model
    try:
        mlmodel = ct.convert(
            traced_model,
            inputs=inputs,
            compute_precision=ct.precision.FLOAT16,  # Use FP16 for better performance
            convert_to="mlprogram",  # Use ML Program for newer CoreML versions
            compute_units=compute_unit,
            minimum_deployment_target=ct.target.macOS13
        )
        
        # Add metadata
        if metadata:
            for key, value in metadata.items():
                mlmodel.user_defined_metadata[key] = value
                
        # Save the model
        mlmodel.save(output_path)
        print(f"Model converted successfully and saved to {output_path}")
        return True
        
    except Exception as e:
        print(f"Error converting model: {e}")
        traceback.print_exc()
        return False

def process_game_specific_settings(game_type: str) -> Dict:
    """Process game-specific settings for model creation"""
    
    # Default settings
    settings = {
        "input_channels": 3,
        "input_height": 224,
        "input_width": 224,
        "num_actions": 10,
        "metadata": {}
    }
    
    # Game-specific settings
    game_settings = {
        "mvsc": {
            "input_height": 224,
            "input_width": 384,
            "num_actions": 12,
            "metadata": {"game_id": "mvsc", "game_genre": "FIGHTING"}
        },
        "sfa3": {
            "input_height": 224,
            "input_width": 384,
            "num_actions": 12,
            "metadata": {"game_id": "sfa3", "game_genre": "FIGHTING"}
        },
        "ssf2t": {
            "input_height": 224,
            "input_width": 384,
            "num_actions": 12,
            "metadata": {"game_id": "ssf2t", "game_genre": "FIGHTING"}
        },
        "kof98": {
            "input_height": 224,
            "input_width": 304,
            "num_actions": 12,
            "metadata": {"game_id": "kof98", "game_genre": "FIGHTING"}
        },
        "dino": {
            "input_height": 224,
            "input_width": 384,
            "num_actions": 8,
            "metadata": {"game_id": "dino", "game_genre": "BEAT_EM_UP"}
        },
        "1944": {
            "input_height": 224,
            "input_width": 384,
            "num_actions": 6,
            "metadata": {"game_id": "1944", "game_genre": "SHMUP"}
        }
    }
    
    # Update settings if game type is recognized
    if game_type in game_settings:
        settings.update(game_settings[game_type])
    
    return settings

def convert_pytorch_to_coreml(input_path, output_path, game_type=None, quantize=False, enable_int8=False):
    """
    Convert PyTorch model to CoreML
    
    Args:
        input_path: Path to PyTorch model (.pt or .pth)
        output_path: Path to save CoreML model (.mlmodel)
        game_type: Game type for which the model was trained
        quantize: Whether to quantize the model
        enable_int8: Whether to use int8 quantization
    
    Returns:
        True if conversion was successful, False otherwise
    """
    if not HAS_TORCH:
        return create_dummy_model(output_path, game_type)
    
    try:
        # Load PyTorch model
        model = torch.load(input_path, map_location="cpu")
        model.eval()
        
        # Example input - adjust shape for your specific model
        example_input = torch.rand(1, 3, 224, 224)
        
        # Convert to CoreML
        mlmodel = ct.convert(
            model, 
            inputs=[ct.TensorType(name="input", shape=example_input.shape)],
            compute_precision=ct.precision.FLOAT32
        )
        
        # Add metadata
        mlmodel.user_defined_metadata["game_type"] = game_type or "generic"
        mlmodel.user_defined_metadata["created_by"] = "FBNeo PyTorch to CoreML Converter"
        mlmodel.user_defined_metadata["model_version"] = "1.0.0"
        
        # Quantize model if requested
        if quantize:
            if enable_int8:
                mlmodel = quantization_utils.quantize_weights(mlmodel, nbits=8)
            else:
                mlmodel = quantization_utils.quantize_weights(mlmodel, nbits=16)
        
        # Save model
        mlmodel.save(output_path)
        print(f"Model successfully converted and saved to {output_path}")
        
        # Validate model
        output_size = mlmodel.get_spec().description.output[0].type.multiArrayType.shape[0]
        print(f"Output size: {output_size}")
        
        return True
    
    except Exception as e:
        print(f"Error converting model: {e}")
        return False

def convert_from_dummy(output_path, game_type=None):
    """Create a dummy model for testing when no PyTorch model is available"""
    
    return create_dummy_model(output_path, game_type or "generic")

def main():
    parser = argparse.ArgumentParser(description="Convert PyTorch models to CoreML format for FBNeo")
    parser.add_argument("--input", help="Path to PyTorch model")
    parser.add_argument("--output", required=True, help="Path to save CoreML model")
    parser.add_argument("--game-type", help="Game type for model specialization")
    parser.add_argument("--quantize", action="store_true", help="Quantize model to reduce size")
    parser.add_argument("--int8", action="store_true", help="Use INT8 quantization (default is FP16)")
    parser.add_argument("--dummy", action="store_true", help="Create a dummy model for testing")
    
    args = parser.parse_args()
    
    # Create output directory if it doesn't exist
    os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)
    
    if args.dummy or args.input == "dummy":
        return convert_from_dummy(args.output, args.game_type)
    
    if not args.input:
        print("Error: Either --input or --dummy must be specified")
        return False
    
    if not os.path.exists(args.input):
        print(f"Error: Input file {args.input} not found")
        return False
    
    return convert_pytorch_to_coreml(args.input, args.output, args.game_type, args.quantize, args.int8)

if __name__ == "__main__":
    sys.exit(main()) 