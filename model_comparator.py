#!/usr/bin/env python3
"""
Model Comparator for FBNeo AI Models

This module provides functionality to compare the outputs of different AI models
on the same set of input data, helping identify divergences in behavior.
"""

import os
import sys
import json
import torch
import numpy as np
import matplotlib.pyplot as plt
import logging
import jsonlines
from collections import defaultdict
import argparse
from datetime import datetime

class ModelComparator:
    """
    Compares the outputs of different AI models on the same input data.
    
    This class loads AI models and runs inference on inputs extracted from
    replay files, comparing the outputs and identifying differences in behavior.
    """
    
    def __init__(self, model1_path, model2_path, device=None):
        """
        Initialize the comparator with two model paths.
        
        Args:
            model1_path: Path to the first model file (.pt)
            model2_path: Path to the second model file (.pt)
            device: Device to run inference on ('cpu', 'cuda', or None for auto)
        """
        self.model1_path = model1_path
        self.model2_path = model2_path
        
        # Determine device
        if device is None:
            self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        else:
            self.device = torch.device(device)
            
        # Initialize models
        self.model1 = None
        self.model2 = None
        self.model1_name = os.path.basename(model1_path)
        self.model2_name = os.path.basename(model2_path)
        
        # Results storage
        self.results = {
            'total_frames': 0,
            'different_outputs': 0,
            'differences': [],
            'agreement_rate': 0.0,
            'output_similarity': 0.0,
            'frame_differences': {}
        }
    
    def load_models(self):
        """Load both models from their paths."""
        try:
            self.model1 = torch.jit.load(self.model1_path, map_location=self.device)
            self.model1.eval()
            logging.info(f"Loaded model 1: {self.model1_path}")
        except Exception as e:
            logging.error(f"Failed to load model 1: {e}")
            return False
        
        try:
            self.model2 = torch.jit.load(self.model2_path, map_location=self.device)
            self.model2.eval()
            logging.info(f"Loaded model 2: {self.model2_path}")
        except Exception as e:
            logging.error(f"Failed to load model 2: {e}")
            return False
            
        return True
    
    def process_replay(self, replay_file):
        """
        Process a replay file, comparing model outputs for each frame.
        
        Args:
            replay_file: Path to the replay file (.jsonl)
            
        Returns:
            Dictionary of comparison results
        """
        # Load replay data
        frames = []
        try:
            with jsonlines.open(replay_file) as reader:
                for line in reader:
                    frames.append(line)
        except Exception as e:
            logging.error(f"Error loading replay file {replay_file}: {e}")
            return self.results
        
        # Sort frames by frame number if available
        if frames and 'frame' in frames[0]:
            frames.sort(key=lambda x: x.get('frame', 0))
        
        # Initialize results
        self.results['total_frames'] = len(frames)
        
        # Process each frame
        different_outputs = 0
        similarity_sum = 0
        
        for frame in frames:
            frame_num = frame.get('frame', 0)
            
            # Extract input data
            input_tensor = self._frame_to_input_tensor(frame)
            
            if input_tensor is None:
                continue
            
            # Run inference on both models
            output1 = self._run_inference(self.model1, input_tensor)
            output2 = self._run_inference(self.model2, input_tensor)
            
            if output1 is None or output2 is None:
                continue
                
            # Compare outputs
            is_different, similarity, differences = self._compare_outputs(output1, output2)
            
            similarity_sum += similarity
            
            if is_different:
                different_outputs += 1
                self.results['differences'].append({
                    'frame': frame_num,
                    'similarity': similarity,
                    'differences': differences
                })
                
                # Store detailed frame differences
                self.results['frame_differences'][frame_num] = {
                    'model1_output': output1.cpu().numpy().tolist(),
                    'model2_output': output2.cpu().numpy().tolist(),
                    'similarity': similarity,
                    'diff_indices': differences
                }
        
        # Calculate agreement rate
        if self.results['total_frames'] > 0:
            self.results['agreement_rate'] = 1.0 - (different_outputs / self.results['total_frames'])
            self.results['output_similarity'] = similarity_sum / self.results['total_frames']
        
        self.results['different_outputs'] = different_outputs
        
        return self.results
    
    def _frame_to_input_tensor(self, frame):
        """
        Convert a frame from the replay file to an input tensor for the models.
        
        Args:
            frame: Frame data from replay file
            
        Returns:
            Tensor suitable for model input, or None if conversion failed
        """
        try:
            # Extract features based on expected model input format
            features = []
            
            # Common game state variables
            for var in ['p1_x', 'p1_y', 'p1_health', 'p1_meter', 'p2_x', 'p2_y', 'p2_health', 'p2_meter',
                        'time_remaining', 'round', 'x_distance', 'y_distance']:
                if var in frame:
                    features.append(float(frame[var]))
                else:
                    features.append(0.0)  # Default value if missing
            
            # Convert to tensor
            input_tensor = torch.tensor(features, dtype=torch.float32, device=self.device).unsqueeze(0)
            
            return input_tensor
            
        except Exception as e:
            logging.error(f"Error converting frame to input tensor: {e}")
            return None
    
    def _run_inference(self, model, input_tensor):
        """
        Run inference on a model with the given input tensor.
        
        Args:
            model: PyTorch model
            input_tensor: Input tensor
            
        Returns:
            Output tensor, or None if inference failed
        """
        try:
            with torch.no_grad():
                output = model(input_tensor)
            return output
        except Exception as e:
            logging.error(f"Error running inference: {e}")
            return None
    
    def _compare_outputs(self, output1, output2, threshold=0.1):
        """
        Compare two model outputs to detect differences.
        
        Args:
            output1: Output tensor from model 1
            output2: Output tensor from model 2
            threshold: Difference threshold for considering outputs different
            
        Returns:
            Tuple of (is_different, similarity, different_indices)
        """
        # Calculate absolute differences
        diff = torch.abs(output1 - output2)
        
        # Check if any element exceeds threshold
        is_different = torch.any(diff > threshold).item()
        
        # Calculate similarity (1 - mean absolute difference)
        similarity = 1.0 - torch.mean(diff).item()
        
        # Find indices with significant differences
        different_indices = torch.nonzero(diff > threshold).cpu().numpy().tolist()
        
        return is_different, similarity, different_indices
    
    def visualize_results(self, output_dir=None):
        """
        Create visualizations of the comparison results.
        
        Args:
            output_dir: Directory to save visualization outputs
            
        Returns:
            True if visualization was created, False otherwise
        """
        if not self.results['differences']:
            logging.warning("No differences found to visualize")
            return False
        
        # Create output directory if needed
        if output_dir:
            os.makedirs(output_dir, exist_ok=True)
        
        # Create figure for similarity across frames
        plt.figure(figsize=(10, 6))
        
        frame_nums = [d['frame'] for d in self.results['differences']]
        similarities = [d['similarity'] for d in self.results['differences']]
        
        plt.plot(frame_nums, similarities, 'o-')
        plt.axhline(y=self.results['output_similarity'], color='r', linestyle='--', 
                   label=f'Average: {self.results["output_similarity"]:.3f}')
        
        plt.xlabel('Frame Number')
        plt.ylabel('Output Similarity')
        plt.title(f'Model Output Similarity: {self.model1_name} vs {self.model2_name}')
        plt.legend()
        plt.grid(True)
        
        if output_dir:
            plt.savefig(os.path.join(output_dir, 'similarity_by_frame.png'))
            plt.close()
        else:
            plt.show()
        
        # Create figure for output comparison of most different frames
        most_different = sorted(self.results['differences'], key=lambda x: x['similarity'])[:5]
        
        if most_different:
            fig, axes = plt.subplots(len(most_different), 1, figsize=(10, 3 * len(most_different)))
            if len(most_different) == 1:
                axes = [axes]
            
            for i, diff in enumerate(most_different):
                frame_num = diff['frame']
                frame_data = self.results['frame_differences'][frame_num]
                
                axes[i].bar(range(len(frame_data['model1_output'][0])), 
                           frame_data['model1_output'][0], 
                           alpha=0.5, label=self.model1_name)
                
                axes[i].bar(range(len(frame_data['model2_output'][0])), 
                           frame_data['model2_output'][0], 
                           alpha=0.5, label=self.model2_name)
                
                # Mark the most different outputs
                for diff_idx in frame_data['diff_indices']:
                    idx = diff_idx[1]  # Extract the index value
                    axes[i].axvline(x=idx, color='r', linestyle='--')
                
                axes[i].set_title(f'Frame {frame_num} (Similarity: {frame_data["similarity"]:.3f})')
                axes[i].set_xlabel('Output Index')
                axes[i].set_ylabel('Output Value')
                axes[i].legend()
                
            plt.tight_layout()
            
            if output_dir:
                plt.savefig(os.path.join(output_dir, 'most_different_frames.png'))
                plt.close()
            else:
                plt.show()
        
        # Create summary report
        summary = {
            'timestamp': datetime.now().isoformat(),
            'model1': self.model1_path,
            'model2': self.model2_path,
            'total_frames': self.results['total_frames'],
            'different_outputs': self.results['different_outputs'],
            'agreement_rate': self.results['agreement_rate'],
            'output_similarity': self.results['output_similarity']
        }
        
        if output_dir:
            with open(os.path.join(output_dir, 'comparison_summary.json'), 'w') as f:
                json.dump(summary, f, indent=2)
            
            # Save complete results
            with open(os.path.join(output_dir, 'comparison_full_results.json'), 'w') as f:
                # Filter out the full frame differences to keep file size reasonable
                results_copy = self.results.copy()
                # Only include the 20 most different frames
                diff_frames = sorted(results_copy['frame_differences'].keys(),
                                    key=lambda x: results_copy['frame_differences'][x]['similarity'])[:20]
                results_copy['frame_differences'] = {
                    k: results_copy['frame_differences'][k] for k in diff_frames
                }
                json.dump(results_copy, f, indent=2)
        
        return True

def compare_models(replay_file, model1_path, model2_path, output_dir=None, visualize=True):
    """
    Compare two models on the same replay data.
    
    Args:
        replay_file: Path to replay file
        model1_path: Path to first model
        model2_path: Path to second model
        output_dir: Directory to save outputs
        visualize: Whether to create visualizations
        
    Returns:
        True if comparison was successful, False otherwise
    """
    # Create output directory with timestamp if not specified
    if output_dir is None and visualize:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        model1_name = os.path.basename(model1_path).replace('.pt', '')
        model2_name = os.path.basename(model2_path).replace('.pt', '')
        output_dir = f"model_comparison_{model1_name}_vs_{model2_name}_{timestamp}"
        os.makedirs(output_dir, exist_ok=True)
    
    # Initialize comparator
    comparator = ModelComparator(model1_path, model2_path)
    
    # Load models
    if not comparator.load_models():
        logging.error("Failed to load models")
        return False
    
    # Process replay
    logging.info(f"Processing replay file: {replay_file}")
    results = comparator.process_replay(replay_file)
    
    # Log results
    logging.info(f"Comparison complete:")
    logging.info(f"  Total frames: {results['total_frames']}")
    logging.info(f"  Different outputs: {results['different_outputs']}")
    logging.info(f"  Agreement rate: {results['agreement_rate']:.2%}")
    logging.info(f"  Output similarity: {results['output_similarity']:.3f}")
    
    # Create visualizations
    if visualize:
        comparator.visualize_results(output_dir)
        logging.info(f"Visualizations saved to: {output_dir}")
    
    return True

def main():
    # Setup logging
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )
    
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Compare AI model outputs on replay data')
    parser.add_argument('--replay', required=True, help='Path to replay file')
    parser.add_argument('--model1', required=True, help='Path to first model')
    parser.add_argument('--model2', required=True, help='Path to second model')
    parser.add_argument('--output-dir', help='Directory to save outputs')
    parser.add_argument('--no-visualize', dest='visualize', action='store_false',
                        help='Disable visualization generation')
    
    args = parser.parse_args()
    
    # Run comparison
    success = compare_models(
        args.replay,
        args.model1,
        args.model2,
        args.output_dir,
        args.visualize
    )
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main()) 