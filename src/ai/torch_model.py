#!/usr/bin/env python3
"""
PyTorch Model Implementation for FBNeo AI

This module provides the PyTorch implementation of neural network models
for the FBNeo AI training pipeline.
"""

import os
import logging
import numpy as np
from typing import Dict, List, Tuple, Optional, Any, Union

# Import torch and related libraries
try:
    import torch
    import torch.nn as nn
    import torch.nn.functional as F
    import torch.optim as optim
    from torch.utils.data import DataLoader
except ImportError:
    raise ImportError("PyTorch is required for this module. Install with 'pip install torch'")

# Import base model class
from training_pipeline import BaseModel, ModelConfig

# Set up logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('FBNeo.TorchModel')

class FBNeoNet(nn.Module):
    """Neural network architecture for FBNeo AI."""
    
    def __init__(self, input_dim: int, hidden_layers: List[int], output_dim: int, 
                 dropout_rate: float = 0.2, activation: str = "relu"):
        """
        Initialize the neural network.
        
        Args:
            input_dim: Dimension of input features
            hidden_layers: List of hidden layer sizes
            output_dim: Dimension of output actions
            dropout_rate: Dropout probability
            activation: Activation function to use
        """
        super(FBNeoNet, self).__init__()
        
        self.input_dim = input_dim
        self.hidden_layers = hidden_layers
        self.output_dim = output_dim
        self.dropout_rate = dropout_rate
        
        # Map activation string to function
        self.activation_map = {
            "relu": F.relu,
            "leaky_relu": F.leaky_relu,
            "tanh": torch.tanh,
            "sigmoid": torch.sigmoid,
            "elu": F.elu,
        }
        self.activation_fn = self.activation_map.get(activation.lower(), F.relu)
        
        # Build layers
        layers = []
        prev_dim = input_dim
        
        # Hidden layers
        for dim in hidden_layers:
            layers.append(nn.Linear(prev_dim, dim))
            layers.append(nn.BatchNorm1d(dim))
            if dropout_rate > 0:
                layers.append(nn.Dropout(dropout_rate))
            prev_dim = dim
        
        self.hidden_stack = nn.ModuleList(layers)
        
        # Output layer
        self.output_layer = nn.Linear(prev_dim, output_dim)
        
    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """Forward pass through the network."""
        # Process hidden layers
        for i, layer in enumerate(self.hidden_stack):
            if isinstance(layer, nn.Linear):
                x = layer(x)
            elif isinstance(layer, nn.BatchNorm1d):
                x = layer(x)
            elif isinstance(layer, nn.Dropout):
                x = layer(x)
                if i > 0 and isinstance(self.hidden_stack[i-1], nn.BatchNorm1d):
                    x = self.activation_fn(x)
        
        # Output layer
        x = self.output_layer(x)
        return x
    
    def predict_actions(self, x: torch.Tensor) -> torch.Tensor:
        """
        Predict actions from input features.
        
        For discrete actions, returns probabilities.
        """
        logits = self.forward(x)
        return torch.sigmoid(logits)  # Use sigmoid for multi-label classification

class TorchModel(BaseModel):
    """PyTorch implementation of FBNeo AI model."""
    
    def __init__(self, config: ModelConfig):
        """
        Initialize the PyTorch model.
        
        Args:
            config: Model configuration
        """
        super(TorchModel, self).__init__(config)
        
        # Set PyTorch-specific settings
        self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        logger.info(f"Using device: {self.device}")
        
        # Set random seed for PyTorch
        torch.manual_seed(config["shuffle_seed"])
        if torch.cuda.is_available():
            torch.cuda.manual_seed_all(config["shuffle_seed"])
    
    def build_model(self) -> None:
        """Build the PyTorch model architecture."""
        # Create network
        self.model = FBNeoNet(
            input_dim=self.config["input_dim"],
            hidden_layers=self.config["hidden_layers"],
            output_dim=self.config["output_dim"],
            dropout_rate=self.config["dropout_rate"],
            activation=self.config["activation"]
        )
        self.model.to(self.device)
        
        # Create optimizer
        self.optimizer = optim.Adam(
            self.model.parameters(),
            lr=self.config["learning_rate"],
            weight_decay=self.config["weight_decay"]
        )
        
        # Create loss function for multi-label classification
        self.criterion = nn.BCEWithLogitsLoss()
        
        # Create learning rate scheduler
        self.scheduler = optim.lr_scheduler.ReduceLROnPlateau(
            self.optimizer, 
            mode='min', 
            factor=0.5, 
            patience=3, 
            verbose=True
        )
        
        logger.info(f"Model built with {sum(p.numel() for p in self.model.parameters())} parameters")
    
    def train_epoch(self, train_loader: DataLoader, val_loader: Optional[DataLoader] = None) -> Dict[str, float]:
        """
        Train for one epoch.
        
        Args:
            train_loader: DataLoader for training data
            val_loader: Optional DataLoader for validation data
            
        Returns:
            Dict of metrics for the epoch
        """
        # Set to training mode
        self.model.train()
        
        train_loss = 0.0
        correct_predictions = 0
        total_samples = 0
        
        # Training loop
        for batch_idx, (inputs, targets) in enumerate(train_loader):
            # Move to device
            inputs = inputs.to(self.device)
            targets = targets.to(self.device)
            
            # Zero gradients
            self.optimizer.zero_grad()
            
            # Forward pass
            outputs = self.model(inputs)
            
            # Calculate loss
            loss = self.criterion(outputs, targets)
            
            # Add regularization if configured
            if self.config["l1_regularization"] > 0:
                l1_reg = 0.0
                for param in self.model.parameters():
                    l1_reg += torch.norm(param, 1)
                loss += self.config["l1_regularization"] * l1_reg
            
            # Backward pass and optimize
            loss.backward()
            self.optimizer.step()
            
            # Update metrics
            train_loss += loss.item()
            
            # Calculate accuracy for multi-label classification
            predicted = torch.sigmoid(outputs) > 0.5
            correct_predictions += (predicted == targets.bool()).float().sum().item()
            total_samples += targets.numel()
            
            # Log progress
            if (batch_idx + 1) % 50 == 0:
                logger.debug(f"Batch {batch_idx+1}/{len(train_loader)}: Loss: {loss.item():.4f}")
        
        # Calculate average loss and accuracy
        avg_train_loss = train_loss / len(train_loader)
        train_accuracy = correct_predictions / total_samples
        
        # Record learning rate
        current_lr = self.optimizer.param_groups[0]['lr']
        self.history['learning_rate'].append(current_lr)
        
        # Evaluate on validation set if provided
        val_metrics = {"loss": None, "accuracy": None}
        if val_loader:
            val_metrics = self.evaluate(val_loader)
            
            # Update learning rate scheduler
            self.scheduler.step(val_metrics['loss'])
        
        # Return metrics
        metrics = {
            "train_loss": avg_train_loss,
            "train_accuracy": train_accuracy
        }
        
        if val_loader:
            metrics.update({
                "val_loss": val_metrics['loss'],
                "val_accuracy": val_metrics['accuracy']
            })
            
        return metrics
    
    def evaluate(self, data_loader: DataLoader) -> Dict[str, float]:
        """
        Evaluate the model on a dataset.
        
        Args:
            data_loader: DataLoader for evaluation data
            
        Returns:
            Dict of evaluation metrics
        """
        # Set to evaluation mode
        self.model.eval()
        
        eval_loss = 0.0
        correct_predictions = 0
        total_samples = 0
        
        # Disable gradient computation for evaluation
        with torch.no_grad():
            for inputs, targets in data_loader:
                # Move to device
                inputs = inputs.to(self.device)
                targets = targets.to(self.device)
                
                # Forward pass
                outputs = self.model(inputs)
                
                # Calculate loss
                loss = self.criterion(outputs, targets)
                eval_loss += loss.item()
                
                # Calculate accuracy
                predicted = torch.sigmoid(outputs) > 0.5
                correct_predictions += (predicted == targets.bool()).float().sum().item()
                total_samples += targets.numel()
        
        # Calculate average loss and accuracy
        avg_loss = eval_loss / len(data_loader)
        accuracy = correct_predictions / total_samples
        
        return {
            "loss": avg_loss,
            "accuracy": accuracy
        }
    
    def save_model(self, filepath: str) -> None:
        """
        Save the model to a file.
        
        Args:
            filepath: Path to save the model
        """
        # Create directory if it doesn't exist
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        
        # Save model state dictionary
        state_dict = {
            'model_state_dict': self.model.state_dict(),
            'optimizer_state_dict': self.optimizer.state_dict(),
            'scheduler_state_dict': self.scheduler.state_dict(),
            'config': self.config.config,
            'history': self.history
        }
        
        torch.save(state_dict, filepath)
        logger.info(f"Model saved to {filepath}")
    
    def load_model(self, filepath: str) -> None:
        """
        Load the model from a file.
        
        Args:
            filepath: Path to load the model from
        """
        # Check if file exists
        if not os.path.exists(filepath):
            raise FileNotFoundError(f"Model file not found: {filepath}")
        
        # Load state dictionary
        state_dict = torch.load(filepath, map_location=self.device)
        
        # Rebuild model if necessary
        if self.model is None:
            # Update config from saved state
            if 'config' in state_dict:
                self.config.config.update(state_dict['config'])
            
            # Build model architecture
            self.build_model()
        
        # Load model state
        self.model.load_state_dict(state_dict['model_state_dict'])
        
        # Load optimizer state if available
        if 'optimizer_state_dict' in state_dict and self.optimizer is not None:
            self.optimizer.load_state_dict(state_dict['optimizer_state_dict'])
        
        # Load scheduler state if available
        if 'scheduler_state_dict' in state_dict and self.scheduler is not None:
            self.scheduler.load_state_dict(state_dict['scheduler_state_dict'])
        
        # Load history if available
        if 'history' in state_dict:
            self.history = state_dict['history']
        
        logger.info(f"Model loaded from {filepath}")
    
    def export_model(self, filepath: str, format: str = "onnx") -> None:
        """
        Export the model to a specified format.
        
        Args:
            filepath: Path to save the exported model
            format: Export format ("onnx", "torchscript", etc.)
        """
        # Create directory if it doesn't exist
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        
        # Set model to eval mode for export
        self.model.eval()
        
        if format.lower() == "onnx":
            try:
                import onnx
                import onnxruntime
            except ImportError:
                logger.error("ONNX and ONNX Runtime are required for ONNX export. "
                           "Install with 'pip install onnx onnxruntime'")
                raise
            
            # Create dummy input tensor
            dummy_input = torch.randn(1, self.config["input_dim"], device=self.device)
            
            # Export to ONNX
            torch.onnx.export(
                self.model,                     # model being run
                dummy_input,                   # model input
                filepath,                      # where to save the model
                export_params=True,            # store the trained parameter weights
                opset_version=12,              # the ONNX version to export
                do_constant_folding=True,      # optimization
                input_names=['input'],         # model's input names
                output_names=['output'],       # model's output names
                dynamic_axes={
                    'input': {0: 'batch_size'},
                    'output': {0: 'batch_size'}
                }
            )
            
            # Verify the ONNX model
            onnx_model = onnx.load(filepath)
            onnx.checker.check_model(onnx_model)
            
            logger.info(f"Model exported to ONNX format: {filepath}")
            
        elif format.lower() == "torchscript":
            # Create dummy input tensor
            dummy_input = torch.randn(1, self.config["input_dim"], device=self.device)
            
            # Export to TorchScript
            traced_script_module = torch.jit.trace(self.model, dummy_input)
            traced_script_module.save(filepath)
            
            logger.info(f"Model exported to TorchScript format: {filepath}")
            
        elif format.lower() == "coreml":
            try:
                import coremltools as ct
            except ImportError:
                logger.error("CoreMLTools is required for CoreML export. "
                           "Install with 'pip install coremltools'")
                raise
            
            # Create dummy input tensor
            dummy_input = torch.randn(1, self.config["input_dim"], device=self.device)
            
            # Convert to CoreML
            traced_model = torch.jit.trace(self.model, dummy_input)
            
            # Convert to CoreML model
            mlmodel = ct.convert(
                traced_model,
                inputs=[ct.TensorType(name="input", shape=dummy_input.shape)]
            )
            
            # Save the CoreML model
            mlmodel.save(filepath)
            
            logger.info(f"Model exported to CoreML format: {filepath}")
            
        else:
            raise ValueError(f"Unsupported export format: {format}")
    
    def predict(self, inputs: np.ndarray) -> np.ndarray:
        """
        Run inference on input data.
        
        Args:
            inputs: Input data as numpy array
            
        Returns:
            Predicted actions as numpy array
        """
        # Convert to tensor
        if isinstance(inputs, np.ndarray):
            inputs = torch.from_numpy(inputs).float()
        
        # Add batch dimension if needed
        if inputs.dim() == 1:
            inputs = inputs.unsqueeze(0)
        
        # Move to device
        inputs = inputs.to(self.device)
        
        # Set to evaluation mode
        self.model.eval()
        
        # Run inference
        with torch.no_grad():
            outputs = self.model.predict_actions(inputs)
        
        # Convert to numpy and return
        return outputs.cpu().numpy() 