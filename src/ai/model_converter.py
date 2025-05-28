#!/usr/bin/env python3
"""
PyTorch to CoreML Model Converter for FBNeo AI

This utility script converts trained PyTorch models to Apple CoreML format
for efficient inference on macOS and iOS devices, leveraging the Neural Engine
on Apple Silicon hardware.
"""

import os
import sys
import argparse
import torch
import coremltools as ct
import numpy as np
from pathlib import Path

def convert_torch_to_coreml(input_path, output_path, input_shape=None, quantize=False, verbose=False):
    """
    Convert a PyTorch model to CoreML format.
    
    Args:
        input_path: Path to the PyTorch model (.pt or .pth file)
        output_path: Path to save the CoreML model (.mlmodel file)
        input_shape: Input shape for the model (default: determined from model)
        quantize: Whether to quantize the model for smaller size (default: False)
        verbose: Whether to print verbose output (default: False)
    
    Returns:
        bool: True if conversion was successful, False otherwise
    """
    try:
        # Load the PyTorch model
        model = torch.jit.load(input_path)
        model.eval()  # Set to evaluation mode
        
        if verbose:
            print(f"Loaded PyTorch model from {input_path}")
            print(f"Model structure:\n{model}")
        
        # Determine input shape if not provided
        if input_shape is None:
            # Try to infer from model
            for param in model.parameters():
                if hasattr(param, 'shape') and len(param.shape) > 0:
                    # Assume first layer's input shape
                    if verbose:
                        print(f"Inferring input shape from model parameters")
                    input_shape = (1, param.shape[0])  # Batch size 1, feature dim from first layer
                    break
            
            # If still not determined, use default
            if input_shape is None:
                input_shape = (1, 32)  # Default: batch size 1, 32 features
                if verbose:
                    print(f"Using default input shape: {input_shape}")
            else:
                if verbose:
                    print(f"Inferred input shape: {input_shape}")
        else:
            if verbose:
                print(f"Using provided input shape: {input_shape}")
        
        # Create example input for tracing
        example_input = torch.rand(*input_shape)
        
        # Convert PyTorch model to CoreML
        if verbose:
            print("Converting to CoreML format...")
        
        # Create MLModel
        mlmodel = ct.convert(
            model,
            inputs=[ct.TensorType(shape=example_input.shape)],
            compute_precision=ct.precision.FLOAT16 if quantize else ct.precision.FLOAT32,
            convert_to="mlprogram"  # More efficient for Neural Engine
        )
        
        # Add metadata
        mlmodel.author = "FBNeo AI"
        mlmodel.license = "MIT"
        mlmodel.short_description = "Game AI model for FBNeo"
        mlmodel.version = "1.0"
        
        # Save the model
        if verbose:
            print(f"Saving CoreML model to {output_path}")
        
        mlmodel.save(output_path)
        
        if verbose:
            model_size_mb = os.path.getsize(output_path) / (1024 * 1024)
            print(f"Model saved successfully. Size: {model_size_mb:.2f} MB")
            
            # Print model details
            spec = mlmodel.get_spec()
            print("\nModel Specification:")
            print(f"Input(s):")
            for input_spec in spec.description.input:
                print(f"  - {input_spec.name}: {input_spec.type.WhichOneof('Type')}")
            
            print(f"Output(s):")
            for output_spec in spec.description.output:
                print(f"  - {output_spec.name}: {output_spec.type.WhichOneof('Type')}")
        
        return True
    
    except Exception as e:
        print(f"Error converting model: {e}")
        return False

def batch_convert(input_dir, output_dir, quantize=False, verbose=False):
    """
    Convert all PyTorch models in a directory to CoreML format.
    
    Args:
        input_dir: Directory containing PyTorch models
        output_dir: Directory to save CoreML models
        quantize: Whether to quantize the models (default: False)
        verbose: Whether to print verbose output (default: False)
    
    Returns:
        int: Number of successfully converted models
    """
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Find all PyTorch models
    pt_files = list(Path(input_dir).glob("*.pt")) + list(Path(input_dir).glob("*.pth"))
    
    if verbose:
        print(f"Found {len(pt_files)} PyTorch models in {input_dir}")
    
    # Convert each model
    success_count = 0
    for pt_file in pt_files:
        output_file = Path(output_dir) / f"{pt_file.stem}.mlmodel"
        
        if verbose:
            print(f"\nConverting {pt_file} to {output_file}")
        
        if convert_torch_to_coreml(pt_file, output_file, quantize=quantize, verbose=verbose):
            success_count += 1
    
    return success_count

def main():
    parser = argparse.ArgumentParser(description="Convert PyTorch models to CoreML format")
    parser.add_argument("--input", "-i", required=True, help="Input PyTorch model file or directory")
    parser.add_argument("--output", "-o", required=True, help="Output CoreML model file or directory")
    parser.add_argument("--batch", "-b", action="store_true", help="Batch convert all models in the input directory")
    parser.add_argument("--input-shape", "-s", nargs="+", type=int, help="Input shape for the model (e.g., 1 32 for a batch size of 1 with 32 features)")
    parser.add_argument("--quantize", "-q", action="store_true", help="Quantize the model to reduce size")
    parser.add_argument("--verbose", "-v", action="store_true", help="Print verbose output")
    
    args = parser.parse_args()
    
    # Check for required dependencies
    try:
        import torch
        import coremltools
    except ImportError as e:
        print(f"Error: Missing required dependency: {e}")
        print("Please install the required packages:")
        print("pip install torch coremltools")
        return 1
    
    if args.batch:
        # Batch convert all models in the directory
        success_count = batch_convert(args.input, args.output, args.quantize, args.verbose)
        total_count = len(list(Path(args.input).glob("*.pt"))) + len(list(Path(args.input).glob("*.pth")))
        
        print(f"\nConversion complete. Successfully converted {success_count} of {total_count} models.")
        return 0 if success_count > 0 else 1
    else:
        # Convert a single model
        input_shape = None
        if args.input_shape:
            input_shape = tuple(args.input_shape)
        
        success = convert_torch_to_coreml(args.input, args.output, input_shape, args.quantize, args.verbose)
        
        if success:
            print("\nConversion successful.")
            return 0
        else:
            print("\nConversion failed.")
            return 1

if __name__ == "__main__":
    sys.exit(main()) 