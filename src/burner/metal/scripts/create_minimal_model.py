#!/usr/bin/env python3
# Minimal script to create a CoreML model for testing

import sys
import os
import argparse

try:
    import coremltools as ct
    from coremltools.models.neural_network import NeuralNetworkBuilder
except ImportError:
    print("CoreMLTools is required. Install with: pip install coremltools")
    sys.exit(1)

def create_minimal_model(output_path):
    """Create a minimal CoreML model for testing"""
    # Create a simple model specification
    input_name = "image"
    output_name = "output"
    
    # Define model inputs and outputs
    input_features = [
        ct.ImageType(name=input_name, shape=(1, 3, 224, 224))
    ]
    
    output_features = [
        ct.TensorType(name=output_name, shape=(8,))
    ]
    
    # Create a builder for a neural network
    builder = NeuralNetworkBuilder(input_features, output_features, "classifier")
    
    # Add layers to the model
    builder.add_flatten(
        name="flatten", 
        input_name=input_name,
        output_name="flattened",
        mode=0
    )
    
    # Add a fully connected layer with random weights
    builder.add_inner_product(
        name="fc",
        input_name="flattened",
        output_name=output_name,
        input_channels=3*224*224,
        output_channels=8,
        W=None,  # Random weights will be used
        b=None,  # Random bias will be used
        has_bias=True
    )
    
    # Create the MLModel
    model = ct.models.MLModel(builder.spec)
    
    # Add metadata
    model.user_defined_metadata = {
        "created_for": "FBNeo",
        "model_type": "test",
        "version": "1.0"
    }
    
    # Set model properties
    model.author = "FBNeo Test"
    model.license = "MIT"
    model.version = "1.0"
    model.short_description = "Minimal test model for FBNeo"
    
    # Save the model
    model.save(output_path)
    print(f"Minimal test model saved to {output_path}")
    return True

def main():
    parser = argparse.ArgumentParser(description="Create a minimal CoreML model for testing")
    parser.add_argument("--output", required=True, help="Output path for the CoreML model")
    args = parser.parse_args()
    
    # Create output directory if it doesn't exist
    os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)
    
    # Create and save the model
    create_minimal_model(args.output)

if __name__ == "__main__":
    main() 