#!/usr/bin/env python3
"""
CoreML Export Pipeline for FBNeo AI Models

This script converts a trained PyTorch model to CoreML format,
using ONNX as an intermediate representation.

Usage:
    python export_coreml.py --input model.pt --output model.mlmodel [--input_shape 1,1,96,128] [--var_count 8]
"""

import argparse
import os
import sys
import torch
import numpy as np
import coremltools as ct

def parse_args():
    parser = argparse.ArgumentParser(description='Export PyTorch model to CoreML via ONNX')
    parser.add_argument('--input', required=True, help='Path to input PyTorch model (.pt)')
    parser.add_argument('--output', required=True, help='Path to output CoreML model (.mlmodel)')
    parser.add_argument('--input_shape', default='1,1,96,128', help='Input shape for image (batch,channel,height,width)')
    parser.add_argument('--var_count', type=int, default=8, help='Number of game variables')
    parser.add_argument('--validate', action='store_true', help='Validate output model against input model')
    parser.add_argument('--deployment_target', default='iOS14', help='Minimum deployment target (iOS14, iOS15, etc.)')
    parser.add_argument('--skip_onnx', action='store_true', help='Skip ONNX intermediate export (direct CoreML conversion)')
    return parser.parse_args()

def load_pytorch_model(model_path):
    """Load a TorchScript model"""
    print(f"Loading PyTorch model from {model_path}")
    try:
        model = torch.jit.load(model_path)
        model.eval()  # Set to evaluation mode
        return model
    except Exception as e:
        print(f"Error loading model: {e}")
        sys.exit(1)

def export_to_onnx(model, onnx_path, input_shape, var_count):
    """Export PyTorch model to ONNX format"""
    print(f"Exporting to ONNX: {onnx_path}")
    
    # Parse input shape
    shape = [int(dim) for dim in input_shape.split(',')]
    print(f"Image input shape: {shape}")
    
    # Create dummy inputs
    dummy_image = torch.randn(shape)
    dummy_vars = torch.randn(1, var_count)
    
    # Define input and output names
    input_names = ["image"]
    output_names = ["output"]
    
    if var_count > 0:
        input_names.append("game_variables")
    
    try:
        # Export the model
        torch.onnx.export(
            model,
            (dummy_image, dummy_vars) if var_count > 0 else dummy_image,
            onnx_path,
            input_names=input_names,
            output_names=output_names,
            verbose=False,
            opset_version=13,
            do_constant_folding=True,
            export_params=True
        )
        print(f"ONNX export successful: {onnx_path}")
        return True
    except Exception as e:
        print(f"Error exporting to ONNX: {e}")
        return False

def convert_onnx_to_coreml(onnx_path, mlmodel_path, input_shape, var_count, deployment_target):
    """Convert ONNX model to CoreML format"""
    print(f"Converting ONNX model to CoreML: {mlmodel_path}")
    
    # Parse input shape
    shape = [int(dim) for dim in input_shape.split(',')]
    
    # Specify deployment target
    target_dict = {
        'iOS14': ct.target.iOS14,
        'iOS15': ct.target.iOS15,
        'iOS16': ct.target.iOS16,
        'macOS12': ct.target.macOS12,
        'macOS13': ct.target.macOS13
    }
    target = target_dict.get(deployment_target, ct.target.iOS14)
    
    try:
        # Convert the model
        mlmodel = ct.converters.onnx.convert(
            model=onnx_path,
            minimum_deployment_target=target,
            image_input_names=["image"],
            # Specify input shapes if needed
            inputs=[
                ct.ImageType(
                    name="image",
                    shape=shape,
                    scale=1/255.0,
                    bias=[0, 0, 0],
                    color_layout="G"  # Grayscale
                )
            ] if var_count == 0 else None
        )
        
        # Set metadata
        mlmodel.author = "FBNeo AI"
        mlmodel.license = "Open Source"
        mlmodel.short_description = "Game AI model for FBNeo emulator"
        
        # Save the model
        mlmodel.save(mlmodel_path)
        print(f"CoreML export successful: {mlmodel_path}")
        return mlmodel
    except Exception as e:
        print(f"Error converting to CoreML: {e}")
        return None

def convert_torch_to_coreml_direct(model_path, mlmodel_path, input_shape, var_count, deployment_target):
    """Convert PyTorch model directly to CoreML without ONNX intermediate"""
    print(f"Directly converting PyTorch model to CoreML: {mlmodel_path}")
    
    model = load_pytorch_model(model_path)
    
    # Parse input shape
    shape = [int(dim) for dim in input_shape.split(',')]
    
    # Create example inputs
    example_image = torch.randn(shape)
    example_vars = torch.randn(1, var_count)
    
    # Specify inputs for conversion
    inputs = [example_image] if var_count == 0 else [example_image, example_vars]
    
    # Specify deployment target
    target_dict = {
        'iOS14': ct.target.iOS14,
        'iOS15': ct.target.iOS15,
        'iOS16': ct.target.iOS16,
        'macOS12': ct.target.macOS12,
        'macOS13': ct.target.macOS13
    }
    target = target_dict.get(deployment_target, ct.target.iOS14)
    
    try:
        # Convert the model
        traced_model = torch.jit.trace(model, inputs)
        
        mlmodel = ct.convert(
            traced_model,
            inputs=[
                ct.ImageType(
                    name="image",
                    shape=shape,
                    scale=1/255.0,
                    bias=[0, 0, 0],
                    color_layout="G"  # Grayscale
                )
            ] if var_count == 0 else [
                ct.ImageType(
                    name="image",
                    shape=shape,
                    scale=1/255.0,
                    bias=[0, 0, 0],
                    color_layout="G"  # Grayscale
                ),
                ct.TensorType(name="game_variables", shape=(1, var_count))
            ],
            minimum_deployment_target=target
        )
        
        # Set metadata
        mlmodel.author = "FBNeo AI"
        mlmodel.license = "Open Source"
        mlmodel.short_description = "Game AI model for FBNeo emulator"
        
        # Save the model
        mlmodel.save(mlmodel_path)
        print(f"CoreML export successful: {mlmodel_path}")
        return mlmodel
    except Exception as e:
        print(f"Error converting to CoreML directly: {e}")
        return None

def validate_model(torch_model, coreml_model, input_shape, var_count):
    """Validate that CoreML model outputs match PyTorch model outputs"""
    print("\nValidating CoreML model output against PyTorch model...")
    
    # Parse input shape
    shape = [int(dim) for dim in input_shape.split(',')]
    
    # Create random test inputs
    test_image = np.random.rand(*shape).astype(np.float32)
    test_vars = np.random.rand(1, var_count).astype(np.float32)
    
    # Run inference on PyTorch model
    torch_input_image = torch.from_numpy(test_image)
    torch_input_vars = torch.from_numpy(test_vars)
    
    with torch.no_grad():
        torch_output = torch_model(torch_input_image, torch_input_vars).cpu().numpy()
    
    # Run inference on CoreML model
    coreml_inputs = {"image": test_image[0]}
    if var_count > 0:
        coreml_inputs["game_variables"] = test_vars[0]
    
    coreml_output = coreml_model.predict(coreml_inputs)
    coreml_result = coreml_output["output"]
    
    # Compare outputs
    if isinstance(coreml_result, np.ndarray):
        mse = np.mean((torch_output - coreml_result) ** 2)
        print(f"Mean squared error between PyTorch and CoreML: {mse:.6f}")
        
        if mse < 1e-3:
            print("Validation PASSED! ✅")
            return True
        else:
            print("Validation WARNING: Models produce different outputs")
            return False
    else:
        print("Validation ERROR: CoreML output is not a tensor")
        return False

def main():
    args = parse_args()
    
    # Ensure input file exists
    if not os.path.exists(args.input):
        print(f"Error: Input file {args.input} does not exist")
        return 1
    
    # Ensure output directory exists
    output_dir = os.path.dirname(args.output)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # Load the PyTorch model
    torch_model = load_pytorch_model(args.input)
    
    if args.skip_onnx:
        # Convert directly to CoreML
        coreml_model = convert_torch_to_coreml_direct(
            args.input, 
            args.output, 
            args.input_shape, 
            args.var_count, 
            args.deployment_target
        )
    else:
        # Export to ONNX first
        onnx_path = args.output + ".onnx"
        if export_to_onnx(torch_model, onnx_path, args.input_shape, args.var_count):
            # Convert ONNX to CoreML
            coreml_model = convert_onnx_to_coreml(
                onnx_path, 
                args.output, 
                args.input_shape, 
                args.var_count, 
                args.deployment_target
            )
        else:
            return 1
    
    if coreml_model is None:
        print("CoreML conversion failed")
        return 1
    
    # Validate the model if requested
    if args.validate:
        if not validate_model(torch_model, coreml_model, args.input_shape, args.var_count):
            print("Warning: Model validation did not pass")
    
    print(f"\nExport completed successfully!")
    print(f"PyTorch model: {args.input}")
    if not args.skip_onnx:
        print(f"ONNX model: {onnx_path}")
    print(f"CoreML model: {args.output}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main()) 