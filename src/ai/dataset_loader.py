#!/usr/bin/env python3
"""
Dataset Loader for FBNeo AI Training

This module provides classes and utilities for loading and processing
gameplay data logged by the AIDatasetLogger component.
"""

import os
import json
import numpy as np
import torch
from torch.utils.data import Dataset, DataLoader
from PIL import Image
import glob
from pathlib import Path
import random

class GameplayDataset(Dataset):
    """
    Dataset class for loading gameplay data collected by AIDatasetLogger
    
    Features:
    - Loads data from JSONL files
    - Supports filtering by game, player, or other criteria
    - Handles different input preprocessing options
    - Can augment data with random transformations
    """
    
    def __init__(self, 
                data_path, 
                image_size=(96, 128),
                max_frames=None,
                augment=False,
                filter_game=None,
                include_screen=True,
                cache_in_memory=False):
        """
        Initialize the dataset
        
        Args:
            data_path: Path to JSONL file or directory containing JSONL files
            image_size: Tuple (height, width) for image resizing
            max_frames: Maximum number of frames to load (None for all)
            augment: Whether to apply data augmentation
            filter_game: Filter by specific game ID
            include_screen: Whether to load image data (False loads only variables)
            cache_in_memory: Whether to cache all data in memory
        """
        self.data_path = data_path
        self.image_size = image_size
        self.max_frames = max_frames
        self.augment = augment
        self.filter_game = filter_game
        self.include_screen = include_screen
        self.cache_in_memory = cache_in_memory
        
        # Find all JSONL files
        self.data_files = []
        if os.path.isdir(data_path):
            self.data_files = glob.glob(os.path.join(data_path, "**/*.jsonl"), recursive=True)
        elif os.path.isfile(data_path) and data_path.endswith(".jsonl"):
            self.data_files = [data_path]
        
        # Load and parse all files
        self.entries = []
        self._load_data()
        
        # Cache for memory efficiency
        self.cached_data = {}
        if cache_in_memory:
            self._cache_data()
    
    def _load_data(self):
        """Load all data files and extract entries"""
        print(f"Loading data from {len(self.data_files)} files...")
        
        frame_count = 0
        for file_path in self.data_files:
            game_id = self._extract_game_id(file_path)
            
            # Skip if filtering by game and not matching
            if self.filter_game and game_id != self.filter_game:
                continue
                
            # Load the JSONL file
            with open(file_path, 'r') as f:
                for line in f:
                    # Stop if we've reached max frames
                    if self.max_frames and frame_count >= self.max_frames:
                        break
                    
                    # Parse JSON line
                    try:
                        entry = json.loads(line)
                        
                        # Store entry info (without loading images yet)
                        self.entries.append({
                            'file_path': file_path,
                            'line_data': entry,
                            'game_id': game_id
                        })
                        
                        frame_count += 1
                    except json.JSONDecodeError:
                        print(f"Warning: Could not parse line in {file_path}")
        
        print(f"Loaded {frame_count} frames from {len(self.data_files)} files")
    
    def _extract_game_id(self, file_path):
        """Extract game ID from filename"""
        filename = os.path.basename(file_path)
        parts = filename.split('_')
        
        # Assume filename format is game_id_timestamp.jsonl
        if len(parts) >= 2:
            return parts[0]
        return "unknown"
    
    def _cache_data(self):
        """Cache all dataset entries in memory for faster access"""
        print("Caching dataset in memory...")
        
        for idx in range(len(self.entries)):
            # This will load and cache everything
            self.__getitem__(idx)
    
    def __len__(self):
        """Return the number of entries in the dataset"""
        return len(self.entries)
    
    def __getitem__(self, idx):
        """Get a dataset item by index"""
        # Check if item is already cached
        if idx in self.cached_data:
            return self.cached_data[idx]
        
        # Get the entry
        entry = self.entries[idx]
        line_data = entry['line_data']
        
        # Extract game variables
        variables = self._extract_variables(line_data)
        
        # Extract action
        action = self._extract_action(line_data)
        
        # Extract screen image if available and needed
        image_tensor = None
        if self.include_screen:
            image_tensor = self._extract_image(line_data)
        
        # Create return data
        result = {
            'variables': variables,
            'action': action
        }
        
        if image_tensor is not None:
            result['image'] = image_tensor
        
        # Cache result if needed
        if self.cache_in_memory:
            self.cached_data[idx] = result
            
        return result
    
    def _extract_variables(self, line_data):
        """Extract game variables from the data entry"""
        variables = []
        
        # If 'variables' key exists, use that
        if 'variables' in line_data:
            variables = np.array(line_data['variables'], dtype=np.float32)
        
        # Check for specific variables if needed
        for var_name in ['p1_health', 'p2_health', 'timer', 'x_position', 'y_position']:
            if var_name in line_data:
                variables.append(float(line_data[var_name]))
        
        return torch.FloatTensor(variables)
    
    def _extract_action(self, line_data):
        """Extract action from the data entry"""
        action_array = np.zeros(12, dtype=np.float32)
        
        if 'action' in line_data:
            action = line_data['action']
            
            # Map action fields to indices
            action_mapping = {
                'up': 0, 'down': 1, 'left': 2, 'right': 3,
                'button1': 4, 'button2': 5, 'button3': 6,
                'button4': 7, 'button5': 8, 'button6': 9,
                'start': 10, 'coin': 11
            }
            
            # Set values in the action array
            for key, idx in action_mapping.items():
                if key in action and action[key]:
                    action_array[idx] = 1.0
        
        return torch.FloatTensor(action_array)
    
    def _extract_image(self, line_data):
        """Extract and process image data"""
        # If screen data is embedded as base64
        if 'screen_data' in line_data:
            import base64
            import io
            
            try:
                screen_data = base64.b64decode(line_data['screen_data'])
                image = Image.open(io.BytesIO(screen_data)).convert('L')  # Convert to grayscale
                
                # Resize to target size
                image = image.resize(self.image_size[::-1])  # PIL uses (width, height)
                
                # Apply augmentation if enabled
                if self.augment:
                    image = self._augment_image(image)
                
                # Convert to tensor and normalize
                image_tensor = torch.FloatTensor(np.array(image)) / 255.0
                image_tensor = image_tensor.unsqueeze(0)  # Add channel dimension
                
                return image_tensor
            except Exception as e:
                print(f"Error processing image: {e}")
                # Return empty tensor on error
                return torch.zeros((1, *self.image_size), dtype=torch.float32)
        
        # If screen path is given
        elif 'screen_path' in line_data:
            screen_path = line_data['screen_path']
            
            try:
                image = Image.open(screen_path).convert('L')
                
                # Resize to target size
                image = image.resize(self.image_size[::-1])
                
                # Apply augmentation if enabled
                if self.augment:
                    image = self._augment_image(image)
                
                # Convert to tensor and normalize
                image_tensor = torch.FloatTensor(np.array(image)) / 255.0
                image_tensor = image_tensor.unsqueeze(0)  # Add channel dimension
                
                return image_tensor
            except Exception as e:
                print(f"Error loading image {screen_path}: {e}")
                # Return empty tensor on error
                return torch.zeros((1, *self.image_size), dtype=torch.float32)
        
        # If no image data available
        else:
            # Return empty tensor
            return torch.zeros((1, *self.image_size), dtype=torch.float32)
    
    def _augment_image(self, image):
        """Apply random augmentations to the image"""
        # Random horizontal flip (with 50% probability)
        if random.random() > 0.5:
            image = image.transpose(Image.FLIP_LEFT_RIGHT)
        
        # Random brightness adjustment (±10%)
        brightness_factor = random.uniform(0.9, 1.1)
        enhancer = Image.ImageEnhance.Brightness(image)
        image = enhancer.enhance(brightness_factor)
        
        # Random contrast adjustment (±10%)
        contrast_factor = random.uniform(0.9, 1.1)
        enhancer = Image.ImageEnhance.Contrast(image)
        image = enhancer.enhance(contrast_factor)
        
        return image
    
    def get_dataloader(self, batch_size=32, shuffle=True, num_workers=4):
        """Create a DataLoader for this dataset"""
        return DataLoader(
            self,
            batch_size=batch_size,
            shuffle=shuffle,
            num_workers=num_workers,
            pin_memory=True,
        )
    
    def get_variable_dim(self):
        """Get the dimension of the variable input"""
        if len(self.entries) > 0:
            variables = self._extract_variables(self.entries[0]['line_data'])
            return variables.shape[0]
        return 0
    
    def get_action_dim(self):
        """Get the dimension of the action output"""
        if len(self.entries) > 0:
            action = self._extract_action(self.entries[0]['line_data'])
            return action.shape[0]
        return 0

class ReplayDatasetWriter:
    """
    Utility class for writing processed dataset entries for training
    
    This is useful for preprocessing and normalizing data before training
    """
    
    def __init__(self, output_path, compression=True):
        """
        Initialize the dataset writer
        
        Args:
            output_path: Path to write the processed dataset
            compression: Whether to compress the output
        """
        self.output_path = output_path
        self.compression = compression
        
        # Create directory if it doesn't exist
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        
        # Open file for writing
        self.file = open(output_path, 'w')
        self.count = 0
    
    def add_entry(self, variables, action, image_tensor=None):
        """
        Add an entry to the dataset
        
        Args:
            variables: Game variables (numpy array or list)
            action: Action taken (numpy array or list)
            image_tensor: Optional image tensor or path
        """
        entry = {
            'variables': variables.tolist() if hasattr(variables, 'tolist') else variables,
            'action': action.tolist() if hasattr(action, 'tolist') else action,
        }
        
        # Include image data if provided
        if image_tensor is not None:
            if isinstance(image_tensor, str):
                # Store path to image
                entry['screen_path'] = image_tensor
            else:
                # Convert tensor to base64 string
                import base64
                import io
                from PIL import Image
                
                # Convert to numpy and then PIL Image
                image_np = image_tensor.squeeze(0).numpy() * 255
                image = Image.fromarray(image_np.astype(np.uint8))
                
                # Save to byte buffer and encode
                buffer = io.BytesIO()
                image.save(buffer, format="PNG")
                entry['screen_data'] = base64.b64encode(buffer.getvalue()).decode('utf-8')
        
        # Write as JSON line
        self.file.write(json.dumps(entry) + '\n')
        self.count += 1
    
    def close(self):
        """Close the writer and optionally compress the output"""
        self.file.close()
        
        # Compress the file if requested
        if self.compression:
            import gzip
            import shutil
            
            # Compress the file
            with open(self.output_path, 'rb') as f_in:
                with gzip.open(self.output_path + '.gz', 'wb') as f_out:
                    shutil.copyfileobj(f_in, f_out)
            
            # Remove the uncompressed file
            os.remove(self.output_path)
            
            print(f"Compressed output to {self.output_path}.gz")
        
        print(f"Wrote {self.count} entries to {self.output_path}")
    
    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

def preprocess_dataset(input_path, output_path, filter_game=None, image_size=(96, 128), normalize=True):
    """
    Preprocess a dataset for training
    
    Args:
        input_path: Path to input data directory or file
        output_path: Path to write the processed dataset
        filter_game: Filter by specific game ID
        image_size: Tuple (height, width) for image resizing
        normalize: Whether to normalize variables
    """
    # Load the dataset
    dataset = GameplayDataset(
        input_path,
        image_size=image_size,
        filter_game=filter_game,
        include_screen=True,
        cache_in_memory=False
    )
    
    # Compute normalization parameters if needed
    var_mean = None
    var_std = None
    
    if normalize:
        # Collect all variables
        all_variables = []
        for i in range(len(dataset)):
            item = dataset[i]
            all_variables.append(item['variables'].numpy())
        
        all_variables = np.vstack(all_variables)
        var_mean = np.mean(all_variables, axis=0)
        var_std = np.std(all_variables, axis=0)
        
        # Handle zero std values
        var_std[var_std == 0] = 1.0
    
    # Process and write out all data
    with ReplayDatasetWriter(output_path) as writer:
        for i in range(len(dataset)):
            item = dataset[i]
            
            # Normalize variables if needed
            variables = item['variables'].numpy()
            if normalize and var_mean is not None and var_std is not None:
                variables = (variables - var_mean) / var_std
            
            # Write the entry
            writer.add_entry(
                variables=variables,
                action=item['action'].numpy(),
                image_tensor=item.get('image', None)
            )
    
    # Save normalization parameters if used
    if normalize and var_mean is not None and var_std is not None:
        norm_params = {
            'mean': var_mean.tolist(),
            'std': var_std.tolist()
        }
        
        with open(output_path + '.norm.json', 'w') as f:
            json.dump(norm_params, f, indent=2)
    
    return len(dataset)

def main():
    """Command-line interface for the dataset loader and preprocessor"""
    import argparse
    
    parser = argparse.ArgumentParser(description='FBNeo AI Dataset Loader and Preprocessor')
    parser.add_argument('--input', required=True, help='Input data path (directory or JSONL file)')
    parser.add_argument('--output', help='Output path for processed data')
    parser.add_argument('--game', help='Filter by game ID')
    parser.add_argument('--preprocess', action='store_true', help='Preprocess the dataset')
    parser.add_argument('--width', type=int, default=128, help='Image width')
    parser.add_argument('--height', type=int, default=96, help='Image height')
    parser.add_argument('--normalize', action='store_true', help='Normalize variables')
    parser.add_argument('--info', action='store_true', help='Print dataset info')
    
    args = parser.parse_args()
    
    if args.info:
        # Load and print dataset info
        dataset = GameplayDataset(
            args.input,
            image_size=(args.height, args.width),
            filter_game=args.game,
            include_screen=False
        )
        
        print(f"Dataset info:")
        print(f"  Entries: {len(dataset)}")
        print(f"  Variable dimension: {dataset.get_variable_dim()}")
        print(f"  Action dimension: {dataset.get_action_dim()}")
        
        if len(dataset) > 0:
            # Print sample of first entry
            sample = dataset[0]
            print(f"\nSample entry:")
            print(f"  Variables: {sample['variables']}")
            print(f"  Action: {sample['action']}")
    
    elif args.preprocess:
        if not args.output:
            print("Error: --output is required for preprocessing")
            return 1
            
        # Preprocess the dataset
        count = preprocess_dataset(
            args.input,
            args.output,
            filter_game=args.game,
            image_size=(args.height, args.width),
            normalize=args.normalize
        )
        
        print(f"Preprocessed {count} entries")
    
    else:
        # Just load the dataset and report stats
        dataset = GameplayDataset(
            args.input,
            image_size=(args.height, args.width),
            filter_game=args.game
        )
        
        print(f"Loaded {len(dataset)} entries")
    
    return 0

if __name__ == "__main__":
    import sys
    sys.exit(main()) 