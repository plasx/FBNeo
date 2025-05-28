#!/usr/bin/env python3
# FBNeo - CoreML Model Validation Script
# This script validates CoreML models to ensure they meet FBNeo requirements

import os
import sys
import argparse
import json
from typing import Dict, List, Tuple, Optional

# Check if required packages are available
try:
    import coremltools as ct
except ImportError:
    print("Error: CoreMLTools is required for validation")
    print("Install with: pip install coremltools")
    sys.exit(1)

# FBNeo AI model requirements
REQUIRED_METADATA = [
    "game_id",           # Game identifier
    "model_type",        # Type of model (e.g., "FBNeo AI")
]

SUPPORTED_GAMES = [
    "mvsc",              # Marvel vs Capcom
    "sfa3",              # Street Fighter Alpha 3
    "ssf2t",             # Super Street Fighter 2 Turbo
    "kof98",             # King of Fighters 98
    "dino",              # Cadillacs and Dinosaurs
    "1944",              # 1944: The Loop Master
    "generic"            # Generic model for any game
]

def validate_model(model_path: str, verbose: bool = False) -> Tuple[bool, Dict]:
    """
    Validate a CoreML model for compatibility with FBNeo
    
    Args:
        model_path: Path to the CoreML model file
        verbose: Whether to print detailed validation information
        
    Returns:
        (success, details): Tuple of validation result and details dictionary
    """
    if not os.path.exists(model_path):
        return False, {"error": f"Model file not found: {model_path}"}
    
    try:
        # Load the model
        model = ct.models.MLModel(model_path)
        spec = model.get_spec()
        
        # Extract model information
        results = {
            "path": model_path,
            "filename": os.path.basename(model_path),
            "is_valid": True,
            "issues": [],
            "metadata": {}
        }
        
        # Check input format
        inputs = model.input_description
        if not inputs:
            results["is_valid"] = False
            results["issues"].append("Model has no inputs")
        
        # Check for image input
        has_image_input = False
        for input_name, input_desc in model.input_description.items():
            if hasattr(input_desc, 'type') and input_desc.type == 'image':
                has_image_input = True
                results["metadata"]["input_type"] = "image"
                break
        
        if not has_image_input:
            # Check if we have an array input that could be an image
            for input_name, input_desc in model.input_description.items():
                if hasattr(input_desc, 'type') and input_desc.type == 'multiArray':
                    if len(input_desc.shape) >= 3:  # Batch, channels, height, width
                        has_image_input = True
                        results["metadata"]["input_type"] = "array"
                        results["metadata"]["input_shape"] = input_desc.shape
                        break
        
        if not has_image_input:
            results["is_valid"] = False
            results["issues"].append("Model does not have a valid image input")
        
        # Check output format
        outputs = model.output_description
        if not outputs:
            results["is_valid"] = False
            results["issues"].append("Model has no outputs")
        else:
            # Check if output contains action predictions
            for output_name, output_desc in outputs.items():
                if hasattr(output_desc, 'type') and output_desc.type == 'multiArray':
                    results["metadata"]["output_type"] = "array"
                    results["metadata"]["output_shape"] = output_desc.shape
                    results["metadata"]["output_name"] = output_name
        
        # Check metadata
        if hasattr(model, 'user_defined_metadata') and model.user_defined_metadata:
            for key, value in model.user_defined_metadata.items():
                results["metadata"][key] = value
            
            # Check for required metadata
            for req_meta in REQUIRED_METADATA:
                if req_meta not in model.user_defined_metadata:
                    results["issues"].append(f"Missing required metadata: {req_meta}")
            
            # Check game_id if present
            if "game_id" in model.user_defined_metadata:
                game_id = model.user_defined_metadata["game_id"]
                if game_id not in SUPPORTED_GAMES:
                    results["issues"].append(f"Unsupported game_id: {game_id}")
                    
                # Add to results
                results["metadata"]["game_id"] = game_id
        else:
            results["issues"].append("Model has no metadata")
        
        # Check for Neural Engine compatibility
        if "computeUnits" in dir(model):
            results["metadata"]["compute_units"] = str(model.computeUnits)
        
        # If there are any issues but is_valid is still True, it's a warning
        if results["issues"] and results["is_valid"]:
            results["warning"] = True
        
        return results["is_valid"], results
        
    except Exception as e:
        return False, {
            "path": model_path,
            "filename": os.path.basename(model_path),
            "is_valid": False,
            "error": str(e),
            "issues": ["Failed to load model"]
        }

def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(description="Validate CoreML models for FBNeo")
    parser.add_argument("--input", "-i", required=True, help="Input CoreML model path or directory")
    parser.add_argument("--verbose", "-v", action="store_true", help="Enable verbose output")
    parser.add_argument("--json", "-j", action="store_true", help="Output results as JSON")
    
    args = parser.parse_args()
    
    # Check if input is a file or directory
    if os.path.isfile(args.input):
        # Validate single model
        success, details = validate_model(args.input, args.verbose)
        
        if args.json:
            print(json.dumps(details, indent=2))
        else:
            if success:
                print(f"✓ {os.path.basename(args.input)} is valid for FBNeo")
                if "warning" in details and details["warning"]:
                    print("  Warnings:")
                    for issue in details["issues"]:
                        print(f"  - {issue}")
            else:
                print(f"✗ {os.path.basename(args.input)} is not valid for FBNeo")
                if "issues" in details:
                    print("  Issues:")
                    for issue in details["issues"]:
                        print(f"  - {issue}")
                if "error" in details:
                    print(f"  Error: {details['error']}")
    
    elif os.path.isdir(args.input):
        # Validate all models in directory
        results = []
        valid_count = 0
        total_count = 0
        
        for filename in os.listdir(args.input):
            if filename.endswith(".mlmodel") or filename.endswith(".mlpackage"):
                model_path = os.path.join(args.input, filename)
                success, details = validate_model(model_path, args.verbose)
                results.append(details)
                
                total_count += 1
                if success:
                    valid_count += 1
        
        if args.json:
            print(json.dumps(results, indent=2))
        else:
            print(f"Validated {total_count} models: {valid_count} valid, {total_count - valid_count} invalid")
            
            for details in results:
                if details["is_valid"]:
                    print(f"✓ {details['filename']} is valid for FBNeo")
                    if "warning" in details and details["warning"]:
                        print("  Warnings:")
                        for issue in details["issues"]:
                            print(f"  - {issue}")
                else:
                    print(f"✗ {details['filename']} is not valid for FBNeo")
                    if "issues" in details:
                        print("  Issues:")
                        for issue in details["issues"]:
                            print(f"  - {issue}")
                    if "error" in details:
                        print(f"  Error: {details['error']}")
                
                if args.verbose:
                    print("  Metadata:")
                    for key, value in details.get("metadata", {}).items():
                        print(f"  - {key}: {value}")
                    print("")
    
    else:
        print(f"Error: Input not found: {args.input}")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main()) 