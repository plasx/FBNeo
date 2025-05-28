#!/usr/bin/env python3
"""
CoreML Exporter for FBNeo AI

This utility converts PyTorch models to CoreML format for use on Apple devices.
It supports automatic conversion of PPO policy networks from training.
"""

import os
import sys
import json
import argparse
import logging
import torch
import numpy as np
from typing import Dict, List, Any, Optional

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger("CoreMLExporter")

try:
    import coremltools as ct
except ImportError:
    logger.error("coremltools not installed. Please install with: pip install coremltools")
    sys.exit(1)

class CoreMLExporter:
    """
    Utility for exporting PyTorch models to CoreML format
    """
    def __init__(self):
        """Initialize the CoreML exporter"""
        logger.info("CoreML exporter initialized")
    
    def convert_ppo_model(self, model_path: str, output_path: str, 
                          input_dim: int, metadata: Optional[Dict[str, str]] = None) -> bool:
        """
        Convert a PPO model to CoreML format
        
        Args:
            model_path: Path to PyTorch model (.pt)
            output_path: Path to save CoreML model (.mlmodel)
            input_dim: Input dimension of the model
            metadata: Optional metadata to include in the model
            
        Returns:
            True if successful, False otherwise
        """
        try:
            # Make sure output directory exists
            os.makedirs(os.path.dirname(output_path), exist_ok=True)
            
            # Load PPO model
            from training_pipeline import create_model, PPONetwork
            
            # Create config for model
            config = {
                "input_dim": input_dim,
                "hidden_dim": 256,
                "num_actions": 16
            }
            
            # Load model
            model = create_model(config)
            model.load_state_dict(torch.load(model_path, map_location=torch.device('cpu')))
            model.eval()
            
            # Create a trace of just the policy part (not the value function)
            class PolicyWrapper(torch.nn.Module):
                def __init__(self, ppo_model):
                    super(PolicyWrapper, self).__init__()
                    self.ppo_model = ppo_model
                
                def forward(self, x):
                    action_logits, _ = self.ppo_model(x)
                    return torch.softmax(action_logits, dim=-1)
            
            policy_model = PolicyWrapper(model)
            
            # Create sample input
            sample_input = torch.rand(1, input_dim)
            
            # Trace the model
            traced_model = torch.jit.trace(policy_model, sample_input)
            
            # Convert to CoreML
            mlmodel = ct.convert(
                traced_model,
                inputs=[ct.TensorType(name="input", shape=sample_input.shape)],
                convert_to="mlprogram"
            )
            
            # Add metadata
            if metadata:
                for key, value in metadata.items():
                    mlmodel.user_defined_metadata[key] = value
            
            # Important model info
            mlmodel.user_defined_metadata["input_dim"] = str(input_dim)
            mlmodel.user_defined_metadata["model_type"] = "ppo_policy"
            mlmodel.user_defined_metadata["pytorch_version"] = torch.__version__
            mlmodel.user_defined_metadata["coremltools_version"] = ct.__version__
            
            # Set model description
            mlmodel.short_description = "FBNeo AI Policy Model"
            
            # Save the model
            mlmodel.save(output_path)
            logger.info(f"Model successfully converted and saved to {output_path}")
            
            return True
            
        except Exception as e:
            logger.error(f"Error converting model: {e}")
            return False
    
    def convert_model_directory(self, model_dir: str, output_dir: str, 
                                config_path: Optional[str] = None) -> int:
        """
        Convert all PyTorch models in a directory to CoreML format
        
        Args:
            model_dir: Directory containing PyTorch models
            output_dir: Directory to save CoreML models
            config_path: Optional path to configuration file
            
        Returns:
            Number of successfully converted models
        """
        # Load configuration if provided
        if config_path and os.path.exists(config_path):
            with open(config_path, 'r') as f:
                config = json.load(f)
            input_dim = config.get("input_dim", 128)
        else:
            input_dim = 128  # Default
        
        # Ensure output directory exists
        os.makedirs(output_dir, exist_ok=True)
        
        # Find all .pt files
        model_paths = []
        for file in os.listdir(model_dir):
            if file.endswith(".pt") and not file.startswith("optimizer"):
                model_paths.append(os.path.join(model_dir, file))
        
        if not model_paths:
            logger.warning(f"No model files found in {model_dir}")
            return 0
        
        # Convert each model
        success_count = 0
        for model_path in model_paths:
            base_name = os.path.basename(model_path)
            output_name = os.path.splitext(base_name)[0] + ".mlmodel"
            output_path = os.path.join(output_dir, output_name)
            
            logger.info(f"Converting {base_name}...")
            
            # Set metadata
            metadata = {
                "original_model": base_name,
                "game": os.path.basename(model_dir)
            }
            
            # Convert
            if self.convert_ppo_model(model_path, output_path, input_dim, metadata):
                success_count += 1
        
        logger.info(f"Converted {success_count}/{len(model_paths)} models")
        return success_count
    
    def optimize_model(self, mlmodel_path: str, output_path: Optional[str] = None) -> bool:
        """
        Optimize a CoreML model for inference on Apple devices
        
        Args:
            mlmodel_path: Path to CoreML model
            output_path: Path to save optimized model, if None overwrites input
            
        Returns:
            True if successful, False otherwise
        """
        try:
            if output_path is None:
                output_path = mlmodel_path
            
            # Load model
            mlmodel = ct.models.MLModel(mlmodel_path)
            
            # Apply optimizations
            optimized_model = ct.optimize.optimized_model(
                mlmodel,
                ct.optimize.OptimizationConfig(
                    mode="auto",
                    output_disable_multithread=False
                )
            )
            
            # Save optimized model
            optimized_model.save(output_path)
            logger.info(f"Model optimized and saved to {output_path}")
            
            return True
            
        except Exception as e:
            logger.error(f"Error optimizing model: {e}")
            return False
    
    def export_to_metal(self, mlmodel_path: str, output_path: Optional[str] = None) -> bool:
        """
        Export a CoreML model to Metal format for direct integration with FBNeo Metal renderer
        
        Args:
            mlmodel_path: Path to CoreML model
            output_path: Path to save Metal model, if None uses same name with .metal extension
            
        Returns:
            True if successful, False otherwise
        """
        try:
            # Default output path
            if output_path is None:
                base_dir = os.path.dirname(mlmodel_path)
                base_name = os.path.splitext(os.path.basename(mlmodel_path))[0]
                output_path = os.path.join(base_dir, f"{base_name}.metallib")
            
            # Load model
            mlmodel = ct.models.MLModel(mlmodel_path)
            
            # Set Metal compute options
            config = ct.ComputeConfig(compute_units=ct.ComputeUnits.ALL)
            
            # Convert model to Metal shader code
            metal_model = ct.optimize.copy_model_for_metal_compilation(mlmodel)
            
            # Compile Metal code
            metal_lib = ct.optimize.compile_metal_model(metal_model, output_path=output_path, 
                                                      compute_config=config)
            
            logger.info(f"Model exported to Metal and saved to {output_path}")
            
            return True
            
        except Exception as e:
            logger.error(f"Error exporting to Metal: {e}")
            return False
    
    def benchmark_model(self, mlmodel_path: str, input_dim: int, num_runs: int = 100) -> Dict[str, float]:
        """
        Benchmark a CoreML model's performance
        
        Args:
            mlmodel_path: Path to CoreML model
            input_dim: Input dimension
            num_runs: Number of inference runs for benchmark
            
        Returns:
            Dictionary with benchmark results
        """
        try:
            # Load model
            mlmodel = ct.models.MLModel(mlmodel_path)
            
            # Create random input
            sample_input = np.random.rand(1, input_dim).astype(np.float32)
            
            # Warm-up
            for _ in range(10):
                _ = mlmodel.predict({"input": sample_input})
            
            # Benchmark
            import time
            start_time = time.time()
            
            for _ in range(num_runs):
                _ = mlmodel.predict({"input": sample_input})
            
            end_time = time.time()
            
            # Calculate results
            total_time = end_time - start_time
            avg_time = total_time / num_runs
            fps = num_runs / total_time
            
            # Log results
            logger.info(f"Benchmark results for {os.path.basename(mlmodel_path)}:")
            logger.info(f"  Average inference time: {avg_time*1000:.2f} ms")
            logger.info(f"  Throughput: {fps:.2f} fps")
            
            return {
                "avg_time_ms": avg_time * 1000,
                "throughput_fps": fps,
                "total_time": total_time,
                "num_runs": num_runs
            }
            
        except Exception as e:
            logger.error(f"Error benchmarking model: {e}")
            return {
                "error": str(e)
            }

def main():
    # Parse arguments
    parser = argparse.ArgumentParser(description="Convert PyTorch models to CoreML format")
    
    subparsers = parser.add_subparsers(dest="command", help="Command to run")
    
    # Convert single model
    convert_parser = subparsers.add_parser("convert", help="Convert a single model")
    convert_parser.add_argument("--model", required=True, help="Path to PyTorch model")
    convert_parser.add_argument("--output", required=True, help="Path to save CoreML model")
    convert_parser.add_argument("--input-dim", type=int, default=128, help="Input dimension")
    convert_parser.add_argument("--optimize", action="store_true", help="Optimize the model")
    
    # Convert directory
    batch_parser = subparsers.add_parser("batch", help="Convert all models in a directory")
    batch_parser.add_argument("--model-dir", required=True, help="Directory containing PyTorch models")
    batch_parser.add_argument("--output-dir", required=True, help="Directory to save CoreML models")
    batch_parser.add_argument("--config", help="Path to configuration file")
    batch_parser.add_argument("--optimize", action="store_true", help="Optimize the models")
    
    # Optimize model
    optimize_parser = subparsers.add_parser("optimize", help="Optimize a CoreML model")
    optimize_parser.add_argument("--model", required=True, help="Path to CoreML model")
    optimize_parser.add_argument("--output", help="Path to save optimized model")
    
    # Export to Metal
    metal_parser = subparsers.add_parser("metal", help="Export a CoreML model to Metal")
    metal_parser.add_argument("--model", required=True, help="Path to CoreML model")
    metal_parser.add_argument("--output", help="Path to save Metal model")
    
    # Benchmark model
    benchmark_parser = subparsers.add_parser("benchmark", help="Benchmark a CoreML model")
    benchmark_parser.add_argument("--model", required=True, help="Path to CoreML model")
    benchmark_parser.add_argument("--input-dim", type=int, default=128, help="Input dimension")
    benchmark_parser.add_argument("--runs", type=int, default=100, help="Number of inference runs")
    
    args = parser.parse_args()
    
    # Create exporter
    exporter = CoreMLExporter()
    
    # Run command
    if args.command == "convert":
        success = exporter.convert_ppo_model(args.model, args.output, args.input_dim)
        if success and args.optimize:
            exporter.optimize_model(args.output)
        
    elif args.command == "batch":
        exporter.convert_model_directory(args.model_dir, args.output_dir, args.config)
        if args.optimize:
            # Optimize all converted models
            for file in os.listdir(args.output_dir):
                if file.endswith(".mlmodel"):
                    model_path = os.path.join(args.output_dir, file)
                    exporter.optimize_model(model_path)
        
    elif args.command == "optimize":
        exporter.optimize_model(args.model, args.output)
        
    elif args.command == "metal":
        exporter.export_to_metal(args.model, args.output)
        
    elif args.command == "benchmark":
        exporter.benchmark_model(args.model, args.input_dim, args.runs)
        
    else:
        parser.print_help()

if __name__ == "__main__":
    main() 