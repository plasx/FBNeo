#!/usr/bin/env python3
"""
Distributed Training Module for FBNeo AI

This module provides classes and utilities for distributed training across multiple
machines or processes using ZeroMQ for communication.
"""

import os
import sys
import json
import time
import logging
import threading
import zmq
import torch
import numpy as np
import io
import socket
import uuid
import psutil
from typing import Dict, List, Tuple, Any, Optional, Union
from dataclasses import dataclass

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger("DistributedTrainer")

@dataclass
class WorkerInfo:
    """Information about a worker node"""
    id: str
    address: str
    status: str
    last_heartbeat: float
    samples_collected: int
    cpu_usage: float = 0.0
    memory_usage: float = 0.0
    gpu_usage: float = 0.0
    training_speed: float = 0.0  # frames per second

class ParameterServer:
    """
    Parameter server for distributed training.
    
    The parameter server is responsible for:
    1. Maintaining the master model
    2. Distributing model parameters to workers
    3. Receiving and applying gradients from workers
    4. Coordinating training across worker nodes
    """
    
    def __init__(self, config: Dict[str, Any]):
        """
        Initialize the parameter server
        
        Args:
            config: Configuration for the parameter server
        """
        self.config = config
        self.model_version = 0
        self.workers = {}
        self.active = False
        self.lock = threading.RLock()
        
        # Checkpoint settings
        self.checkpoint_dir = config.get("checkpoint_dir", "checkpoints")
        self.checkpoint_interval = config.get("checkpoint_interval", 100)
        self.last_checkpoint_time = time.time()
        
        # Dynamic resource allocation
        self.resource_monitor_interval = config.get("resource_monitor_interval", 30)
        self.enable_dynamic_allocation = config.get("enable_dynamic_allocation", True)
        
        # Load balancing
        self.gradient_weights = {}  # worker_id -> gradient weight
        
        # Recovery settings
        self.recovery_mode = config.get("recovery_mode", "latest")  # 'latest', 'best', 'specific'
        self.specific_checkpoint = config.get("specific_checkpoint", None)
        
        # Ensure checkpoint directory exists
        os.makedirs(self.checkpoint_dir, exist_ok=True)
        
        # ZeroMQ setup
        self.context = zmq.Context()
        
        # Socket for distributing parameters
        self.parameter_socket = self.context.socket(zmq.PUB)
        self.parameter_socket.bind(f"tcp://*:{config.get('parameter_port', 5555)}")
        
        # Socket for receiving gradients
        self.gradient_socket = self.context.socket(zmq.PULL)
        self.gradient_socket.bind(f"tcp://*:{config.get('gradient_port', 5556)}")
        
        # Socket for worker registration and heartbeats
        self.control_socket = self.context.socket(zmq.REP)
        self.control_socket.bind(f"tcp://*:{config.get('control_port', 5557)}")
        
        # Start the heartbeat and control message threads
        self.threads = []
        
        logger.info(f"Parameter server initialized on ports:")
        logger.info(f"  Parameters: {config.get('parameter_port', 5555)}")
        logger.info(f"  Gradients: {config.get('gradient_port', 5556)}")
        logger.info(f"  Control: {config.get('control_port', 5557)}")
    
    def start(self, model: torch.nn.Module):
        """
        Start the parameter server
        
        Args:
            model: Initial model to distribute
        """
        # Try to recover from checkpoint if needed
        recovered_model = self._try_recover_from_checkpoint()
        if recovered_model is not None:
            model = recovered_model
            logger.info("Successfully recovered model from checkpoint")
        
        self.model = model
        self.optimizer = torch.optim.Adam(model.parameters(), lr=self.config.get("lr", 3e-4))
        self.active = True
        
        # Start worker registration and heartbeat thread
        control_thread = threading.Thread(target=self._handle_control_messages)
        control_thread.daemon = True
        control_thread.start()
        self.threads.append(control_thread)
        
        # Start gradient collection thread
        gradient_thread = threading.Thread(target=self._collect_gradients)
        gradient_thread.daemon = True
        gradient_thread.start()
        self.threads.append(gradient_thread)
        
        # Start resource monitoring thread
        if self.enable_dynamic_allocation:
            resource_thread = threading.Thread(target=self._monitor_resources)
            resource_thread.daemon = True
            resource_thread.start()
            self.threads.append(resource_thread)
        
        # Start checkpoint thread
        checkpoint_thread = threading.Thread(target=self._periodic_checkpoint)
        checkpoint_thread.daemon = True
        checkpoint_thread.start()
        self.threads.append(checkpoint_thread)
        
        # Broadcast initial model
        self._broadcast_parameters()
        
        logger.info("Parameter server started")
    
    def stop(self):
        """Stop the parameter server"""
        self.active = False
        
        # Create final checkpoint
        self._save_checkpoint(is_final=True)
        
        time.sleep(1.0)  # Allow threads to finish
        
        # Close sockets
        self.parameter_socket.close()
        self.gradient_socket.close()
        self.control_socket.close()
        
        logger.info("Parameter server stopped")
    
    def _broadcast_parameters(self):
        """Broadcast current model parameters to all workers"""
        if not self.active:
            return
        
        with self.lock:
            # Increase version number
            self.model_version += 1
            
            # Serialize model parameters
            state_dict = self.model.state_dict()
            buffer = io.BytesIO()
            torch.save(state_dict, buffer)
            serialized = buffer.getvalue()
            
            # Send parameters with version number
            self.parameter_socket.send_multipart([
                str(self.model_version).encode('utf-8'),
                serialized
            ])
            
            logger.info(f"Broadcast model parameters (version {self.model_version})")
    
    def _handle_control_messages(self):
        """Handle worker registration and heartbeat messages"""
        while self.active:
            try:
                # Receive control message
                message = self.control_socket.recv_json()
                command = message.get("command")
                worker_id = message.get("worker_id")
                
                with self.lock:
                    if command == "register":
                        # Register new worker
                        self.workers[worker_id] = WorkerInfo(
                            id=worker_id,
                            address=message.get("address"),
                            status="active",
                            last_heartbeat=time.time(),
                            samples_collected=0,
                            cpu_usage=message.get("cpu_usage", 0.0),
                            memory_usage=message.get("memory_usage", 0.0),
                            gpu_usage=message.get("gpu_usage", 0.0),
                            training_speed=message.get("training_speed", 0.0)
                        )
                        self.gradient_weights[worker_id] = 1.0
                        response = {"status": "registered", "model_version": self.model_version}
                        logger.info(f"Worker {worker_id} registered from {message.get('address')}")
                    
                    elif command == "heartbeat":
                        # Update worker heartbeat
                        if worker_id in self.workers:
                            self.workers[worker_id].last_heartbeat = time.time()
                            self.workers[worker_id].status = message.get("status", "active")
                            self.workers[worker_id].samples_collected = message.get("samples_collected", 0)
                            self.workers[worker_id].cpu_usage = message.get("cpu_usage", 0.0)
                            self.workers[worker_id].memory_usage = message.get("memory_usage", 0.0)
                            self.workers[worker_id].gpu_usage = message.get("gpu_usage", 0.0)
                            self.workers[worker_id].training_speed = message.get("training_speed", 0.0)
                            response = {"status": "ok", "model_version": self.model_version}
                        else:
                            response = {"status": "unknown_worker"}
                    
                    elif command == "unregister":
                        # Unregister worker
                        if worker_id in self.workers:
                            del self.workers[worker_id]
                            if worker_id in self.gradient_weights:
                                del self.gradient_weights[worker_id]
                            response = {"status": "unregistered"}
                            logger.info(f"Worker {worker_id} unregistered")
                        else:
                            response = {"status": "unknown_worker"}
                    
                    elif command == "request_workload":
                        # Worker is requesting workload adjustment
                        capacity = message.get("capacity", 1.0)
                        if worker_id in self.workers:
                            # Assign workload based on capacity and server load
                            workload = self._calculate_worker_workload(worker_id, capacity)
                            response = {
                                "status": "ok", 
                                "workload": workload,
                                "episodes_per_batch": max(1, int(workload * 10))
                            }
                        else:
                            response = {"status": "unknown_worker"}
                    
                    else:
                        response = {"status": "unknown_command"}
                
                # Send response
                self.control_socket.send_json(response)
                
                # Remove inactive workers
                self._check_inactive_workers()
                
            except Exception as e:
                logger.error(f"Error handling control message: {e}")
                try:
                    # Try to send an error response
                    self.control_socket.send_json({"status": "error", "message": str(e)})
                except:
                    pass
    
    def _check_inactive_workers(self):
        """Remove workers that haven't sent a heartbeat recently"""
        current_time = time.time()
        with self.lock:
            inactive_workers = []
            for worker_id, worker in self.workers.items():
                if current_time - worker.last_heartbeat > self.config.get("worker_timeout", 60):
                    inactive_workers.append(worker_id)
            
            for worker_id in inactive_workers:
                logger.info(f"Removing inactive worker {worker_id}")
                del self.workers[worker_id]
                if worker_id in self.gradient_weights:
                    del self.gradient_weights[worker_id]
    
    def _collect_gradients(self):
        """Collect and apply gradients from workers"""
        while self.active:
            try:
                # Use poll with timeout to avoid blocking indefinitely
                if self.gradient_socket.poll(1000) == 0:  # 1000 ms timeout
                    continue
                
                # Receive gradient message
                message = self.gradient_socket.recv_multipart()
                
                if len(message) != 3:
                    logger.warning(f"Invalid gradient message, expected 3 parts, got {len(message)}")
                    continue
                
                worker_id = message[0].decode('utf-8')
                version = int(message[1].decode('utf-8'))
                serialized_gradients = message[2]
                
                if version != self.model_version:
                    logger.warning(f"Received gradients for version {version}, but current version is {self.model_version}")
                    continue
                
                # Deserialize gradients
                buffer = io.BytesIO(serialized_gradients)
                gradients = torch.load(buffer)
                
                # Apply gradients with worker weight
                with self.lock:
                    weight = self.gradient_weights.get(worker_id, 1.0)
                    self.apply_gradients(gradients, weight)
                    logger.info(f"Applied gradients from worker {worker_id} with weight {weight:.2f}")
                    
                    # Broadcast updated parameters
                    if self.model_version % self.config.get("broadcast_interval", 10) == 0:
                        self._broadcast_parameters()
                
            except Exception as e:
                logger.error(f"Error collecting gradients: {e}")
    
    def apply_gradients(self, gradients: Dict[str, torch.Tensor], weight: float = 1.0):
        """
        Apply gradients to the model
        
        Args:
            gradients: Dictionary of parameter name -> gradient tensor
            weight: Weight to apply to these gradients (for weighted averaging)
        """
        for name, param in self.model.named_parameters():
            if name in gradients:
                # Manually set the gradient with weighting
                if param.grad is None:
                    param.grad = gradients[name].clone() * weight
                else:
                    param.grad.add_(gradients[name], alpha=weight)
        
        # Step the optimizer
        self.optimizer.step()
        self.optimizer.zero_grad()
    
    def _monitor_resources(self):
        """Monitor worker resources and adjust workloads dynamically"""
        while self.active:
            try:
                with self.lock:
                    if len(self.workers) > 0:
                        self._balance_workloads()
                
                # Sleep for monitoring interval
                time.sleep(self.resource_monitor_interval)
                
            except Exception as e:
                logger.error(f"Error monitoring resources: {e}")
                time.sleep(5.0)  # Sleep before retry on error
    
    def _balance_workloads(self):
        """Balance workloads across workers based on their resources and performance"""
        # Calculate total capacity
        total_speed = sum(worker.training_speed for worker in self.workers.values())
        if total_speed <= 0:
            # Not enough data yet
            return
        
        # Adjust gradient weights based on training speed
        for worker_id, worker in self.workers.items():
            if worker.training_speed > 0:
                # Weight proportional to training speed
                self.gradient_weights[worker_id] = worker.training_speed / total_speed
            else:
                self.gradient_weights[worker_id] = 0.1  # Minimal weight if no speed data
        
        logger.info(f"Adjusted gradient weights based on worker performance")
    
    def _calculate_worker_workload(self, worker_id: str, capacity: float) -> float:
        """
        Calculate appropriate workload for a worker
        
        Args:
            worker_id: Worker ID
            capacity: Worker-reported capacity (0.0-1.0)
            
        Returns:
            Workload assignment (0.0-1.0)
        """
        # Simple workload assignment based on capacity
        return min(1.0, max(0.1, capacity))
    
    def _periodic_checkpoint(self):
        """Periodically save checkpoints"""
        while self.active:
            try:
                current_time = time.time()
                if (current_time - self.last_checkpoint_time >= 
                    self.config.get("checkpoint_interval_seconds", 300)):
                    self._save_checkpoint()
                    self.last_checkpoint_time = current_time
                
                # Sleep for a while
                time.sleep(10.0)
                
            except Exception as e:
                logger.error(f"Error during checkpoint: {e}")
                time.sleep(60.0)  # Sleep longer on error
    
    def _save_checkpoint(self, is_final: bool = False):
        """
        Save a checkpoint of the current model and optimizer state
        
        Args:
            is_final: Whether this is the final checkpoint before shutdown
        """
        with self.lock:
            # Create checkpoint filename
            if is_final:
                checkpoint_path = os.path.join(self.checkpoint_dir, f"final_model_v{self.model_version}.pt")
            else:
                checkpoint_path = os.path.join(self.checkpoint_dir, f"checkpoint_v{self.model_version}.pt")
            
            # Create checkpoint data
            checkpoint = {
                'model_state_dict': self.model.state_dict(),
                'optimizer_state_dict': self.optimizer.state_dict(),
                'model_version': self.model_version,
                'timestamp': time.time()
            }
            
            # Save checkpoint
            torch.save(checkpoint, checkpoint_path)
            
            # Save latest checkpoint reference
            latest_path = os.path.join(self.checkpoint_dir, "latest.pt")
            torch.save(checkpoint, latest_path)
            
            logger.info(f"Saved checkpoint to {checkpoint_path}")
    
    def _try_recover_from_checkpoint(self):
        """
        Try to recover model from checkpoint based on recovery mode
        
        Returns:
            Recovered model or None if recovery failed
        """
        try:
            checkpoint_path = None
            
            if self.recovery_mode == "specific" and self.specific_checkpoint:
                # Use specific checkpoint
                checkpoint_path = self.specific_checkpoint
            
            elif self.recovery_mode == "latest":
                # Use latest checkpoint
                latest_path = os.path.join(self.checkpoint_dir, "latest.pt")
                if os.path.exists(latest_path):
                    checkpoint_path = latest_path
            
            elif self.recovery_mode == "best":
                # Use best checkpoint (not implemented yet)
                best_path = os.path.join(self.checkpoint_dir, "best.pt")
                if os.path.exists(best_path):
                    checkpoint_path = best_path
            
            if checkpoint_path and os.path.exists(checkpoint_path):
                logger.info(f"Attempting to recover from checkpoint: {checkpoint_path}")
                
                # Load checkpoint
                checkpoint = torch.load(checkpoint_path)
                
                # Create new model and load state
                from training_pipeline import create_model
                model = create_model(self.config)
                model.load_state_dict(checkpoint['model_state_dict'])
                
                # Set model version
                self.model_version = checkpoint.get('model_version', 0)
                
                logger.info(f"Recovered model version {self.model_version}")
                return model
            
        except Exception as e:
            logger.error(f"Error recovering from checkpoint: {e}")
        
        return None

class TrainingWorker:
    """
    Worker node for distributed training.
    
    The worker is responsible for:
    1. Collecting experiences from the environment
    2. Computing gradients based on collected experiences
    3. Sending gradients to the parameter server
    4. Receiving updated model parameters from the server
    """
    
    def __init__(self, config: Dict[str, Any], worker_id: Optional[str] = None):
        """
        Initialize the training worker
        
        Args:
            config: Configuration for the worker
            worker_id: Unique identifier for this worker, generated if not provided
        """
        self.config = config
        self.worker_id = worker_id or f"worker-{os.getpid()}-{str(uuid.uuid4())[:8]}"
        self.active = False
        self.model_version = 0
        self.samples_collected = 0
        self.reconnect_attempts = 0
        self.max_reconnect_attempts = config.get("max_reconnect_attempts", 10)
        self.reconnect_delay = config.get("reconnect_delay", 5)
        self.training_start_time = 0
        self.frames_processed = 0
        self.training_speed = 0.0  # frames per second
        self.current_workload = 1.0  # workload allocation (0-1)
        self.workload_request_interval = config.get("workload_request_interval", 60)
        self.last_workload_request = 0
        
        # Master server information
        self.master_ip = config.get("master_ip", "localhost")
        self.parameter_port = config.get("parameter_port", 5555)
        self.gradient_port = config.get("gradient_port", 5556)
        self.control_port = config.get("control_port", 5557)
        
        # Initialize ZMQ
        self._initialize_sockets()
        
        logger.info(f"Worker {self.worker_id} connecting to master at {self.master_ip}")
    
    def _initialize_sockets(self):
        """Initialize ZeroMQ sockets with error handling"""
        try:
            # ZeroMQ setup
            self.context = zmq.Context()
            
            # Socket for receiving parameters
            self.parameter_socket = self.context.socket(zmq.SUB)
            self.parameter_socket.connect(f"tcp://{self.master_ip}:{self.parameter_port}")
            self.parameter_socket.setsockopt_string(zmq.SUBSCRIBE, "")
            
            # Socket for sending gradients
            self.gradient_socket = self.context.socket(zmq.PUSH)
            self.gradient_socket.connect(f"tcp://{self.master_ip}:{self.gradient_port}")
            
            # Socket for control messages
            self.control_socket = self.context.socket(zmq.REQ)
            self.control_socket.connect(f"tcp://{self.master_ip}:{self.control_port}")
            
            logger.info(f"Sockets initialized to master at {self.master_ip}")
            logger.info(f"  Parameters: {self.parameter_port}")
            logger.info(f"  Gradients: {self.gradient_port}")
            logger.info(f"  Control: {self.control_port}")
            
            self.reconnect_attempts = 0  # Reset counter on successful connect
            
        except Exception as e:
            logger.error(f"Error initializing sockets: {e}")
            raise
    
    def start(self, model: torch.nn.Module):
        """
        Start the worker
        
        Args:
            model: Initial model to use
        """
        self.model = model
        self.active = True
        self.training_start_time = time.time()
        
        # Try to register with parameter server with retry
        success = self._register_with_server()
        if not success:
            raise RuntimeError("Failed to register with parameter server after retry attempts")
        
        # Start heartbeat thread
        heartbeat_thread = threading.Thread(target=self._send_heartbeats)
        heartbeat_thread.daemon = True
        heartbeat_thread.start()
        
        # Start parameter update thread
        param_thread = threading.Thread(target=self._receive_parameters)
        param_thread.daemon = True
        param_thread.start()
        
        # Start resource monitoring thread
        resource_thread = threading.Thread(target=self._monitor_resources)
        resource_thread.daemon = True
        resource_thread.start()
        
        logger.info(f"Worker {self.worker_id} started")
    
    def _register_with_server(self) -> bool:
        """
        Register with the parameter server with retry
        
        Returns:
            True if successfully registered, False otherwise
        """
        for attempt in range(self.max_reconnect_attempts):
            try:
                # Register with parameter server
                register_message = {
                    "command": "register",
                    "worker_id": self.worker_id,
                    "address": self._get_local_ip()
                }
                
                # Add resource information
                resources = self._get_resource_usage()
                register_message.update(resources)
                
                self.control_socket.send_json(register_message)
                response = self.control_socket.recv_json()
                
                if response.get("status") == "registered":
                    self.model_version = response.get("model_version", 0)
                    logger.info(f"Successfully registered with parameter server (attempt {attempt+1})")
                    return True
                else:
                    logger.warning(f"Failed to register: {response.get('status')}")
            
            except Exception as e:
                logger.error(f"Error during registration (attempt {attempt+1}): {e}")
            
            if attempt < self.max_reconnect_attempts - 1:
                sleep_time = self.reconnect_delay * (attempt + 1)  # Exponential backoff
                logger.info(f"Retrying registration in {sleep_time} seconds...")
                time.sleep(sleep_time)
                
                # Reinitialize sockets for next attempt
                self._close_sockets()
                self._initialize_sockets()
        
        return False
    
    def stop(self):
        """Stop the worker"""
        self.active = False
        time.sleep(1.0)  # Allow threads to finish
        
        # Unregister from parameter server (best effort)
        try:
            unregister_message = {
                "command": "unregister",
                "worker_id": self.worker_id
            }
            
            self.control_socket.send_json(unregister_message)
            self.control_socket.recv_json()  # Wait for response
        except Exception as e:
            logger.warning(f"Error unregistering from server: {e}")
        
        # Close sockets
        self._close_sockets()
        
        logger.info(f"Worker {self.worker_id} stopped")
    
    def _close_sockets(self):
        """Close all ZMQ sockets"""
        try:
            if hasattr(self, 'parameter_socket'):
                self.parameter_socket.close()
            
            if hasattr(self, 'gradient_socket'):
                self.gradient_socket.close()
            
            if hasattr(self, 'control_socket'):
                self.control_socket.close()
            
            if hasattr(self, 'context'):
                self.context.term()
        except Exception as e:
            logger.warning(f"Error closing sockets: {e}")
    
    def _send_heartbeats(self):
        """Send regular heartbeats to the parameter server"""
        heartbeat_interval = self.config.get("heartbeat_interval", 10)
        while self.active:
            try:
                # Get current resource usage
                resources = self._get_resource_usage()
                
                heartbeat_message = {
                    "command": "heartbeat",
                    "worker_id": self.worker_id,
                    "status": "active",
                    "samples_collected": self.samples_collected,
                    "training_speed": self.training_speed,
                    **resources
                }
                
                self.control_socket.send_json(heartbeat_message)
                response = self.control_socket.recv_json()
                
                # Check if we should request a workload adjustment
                current_time = time.time()
                if current_time - self.last_workload_request > self.workload_request_interval:
                    self._request_workload_adjustment()
                    self.last_workload_request = current_time
                
                # Sleep for heartbeat interval
                time.sleep(heartbeat_interval)
                
            except Exception as e:
                logger.error(f"Error sending heartbeat: {e}")
                
                # Try to reconnect if too many errors
                self.reconnect_attempts += 1
                if self.reconnect_attempts > self.max_reconnect_attempts:
                    logger.error("Too many failures, attempting to reconnect")
                    try:
                        self._close_sockets()
                        self._initialize_sockets()
                        self.reconnect_attempts = 0
                    except Exception as reconnect_error:
                        logger.error(f"Reconnection failed: {reconnect_error}")
                
                time.sleep(max(1.0, heartbeat_interval / 2))  # Brief pause before retry
    
    def _receive_parameters(self):
        """Receive updated parameters from the parameter server"""
        while self.active:
            try:
                # Use poll with timeout to avoid blocking indefinitely
                if self.parameter_socket.poll(1000) == 0:  # 1000 ms timeout
                    continue
                
                # Receive parameter message
                message = self.parameter_socket.recv_multipart()
                
                if len(message) != 2:
                    logger.warning(f"Invalid parameter message, expected 2 parts, got {len(message)}")
                    continue
                
                version = int(message[0].decode('utf-8'))
                serialized_params = message[1]
                
                # Only update if this is a newer version
                if version <= self.model_version:
                    continue
                
                # Deserialize and update model
                buffer = io.BytesIO(serialized_params)
                state_dict = torch.load(buffer)
                
                # Update model parameters
                self.model.load_state_dict(state_dict)
                self.model_version = version
                
                logger.info(f"Updated model parameters to version {version}")
                
            except Exception as e:
                logger.error(f"Error receiving parameters: {e}")
                time.sleep(5.0)  # Sleep before retry
    
    def send_gradients(self, gradients: Dict[str, torch.Tensor]):
        """
        Send computed gradients to the parameter server
        
        Args:
            gradients: Dictionary of parameter name -> gradient tensor
        """
        try:
            # Serialize gradients
            buffer = io.BytesIO()
            torch.save(gradients, buffer)
            serialized = buffer.getvalue()
            
            # Send gradients with worker ID and version
            self.gradient_socket.send_multipart([
                self.worker_id.encode('utf-8'),
                str(self.model_version).encode('utf-8'),
                serialized
            ])
            
            logger.info(f"Sent gradients for model version {self.model_version}")
            
        except Exception as e:
            logger.error(f"Error sending gradients: {e}")
    
    def _get_local_ip(self):
        """Get the local IP address"""
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            # doesn't even have to be reachable
            s.connect(('10.255.255.255', 1))
            ip = s.getsockname()[0]
        except Exception:
            ip = '127.0.0.1'
        finally:
            s.close()
        return ip
    
    def _get_resource_usage(self):
        """Get current resource usage information"""
        try:
            # CPU usage
            cpu_percent = psutil.cpu_percent()
            
            # Memory usage
            memory = psutil.virtual_memory()
            memory_percent = memory.percent
            
            # GPU usage (if available)
            gpu_percent = 0.0
            try:
                import pynvml
                pynvml.nvmlInit()
                device_count = pynvml.nvmlDeviceGetCount()
                if device_count > 0:
                    handle = pynvml.nvmlDeviceGetHandleByIndex(0)
                    util = pynvml.nvmlDeviceGetUtilizationRates(handle)
                    gpu_percent = util.gpu
                pynvml.nvmlShutdown()
            except:
                # GPU monitoring not available
                pass
            
            return {
                "cpu_usage": cpu_percent,
                "memory_usage": memory_percent,
                "gpu_usage": gpu_percent
            }
            
        except Exception as e:
            logger.warning(f"Error getting resource usage: {e}")
            return {
                "cpu_usage": 0.0,
                "memory_usage": 0.0,
                "gpu_usage": 0.0
            }
    
    def _monitor_resources(self):
        """Monitor local resources and update training statistics"""
        monitor_interval = self.config.get("resource_monitor_interval", 10)
        while self.active:
            try:
                # Calculate training speed
                current_time = time.time()
                elapsed = current_time - self.training_start_time
                
                if elapsed > 0 and self.frames_processed > 0:
                    self.training_speed = self.frames_processed / elapsed
                
                time.sleep(monitor_interval)
                
            except Exception as e:
                logger.error(f"Error monitoring resources: {e}")
                time.sleep(monitor_interval)
    
    def update_frames_processed(self, new_frames: int):
        """
        Update the count of processed frames
        
        Args:
            new_frames: Number of new frames processed
        """
        self.frames_processed += new_frames
        self.samples_collected += new_frames
    
    def _request_workload_adjustment(self):
        """Request workload adjustment from the server based on capacity"""
        try:
            # Calculate capacity based on current resource usage
            resources = self._get_resource_usage()
            cpu_headroom = max(0, 90 - resources["cpu_usage"]) / 90.0
            memory_headroom = max(0, 90 - resources["memory_usage"]) / 90.0
            
            # Capacity is the minimum available headroom
            capacity = min(cpu_headroom, memory_headroom)
            
            request_message = {
                "command": "request_workload",
                "worker_id": self.worker_id,
                "capacity": capacity,
                "training_speed": self.training_speed
            }
            
            self.control_socket.send_json(request_message)
            response = self.control_socket.recv_json()
            
            if response.get("status") == "ok":
                self.current_workload = response.get("workload", 1.0)
                logger.info(f"Adjusted workload to {self.current_workload:.2f}")
                
                # Update episodes per batch if provided
                episodes_per_batch = response.get("episodes_per_batch")
                if episodes_per_batch is not None:
                    self.config["episodes_per_batch"] = episodes_per_batch
                    logger.info(f"Updated episodes_per_batch to {episodes_per_batch}")
            
        except Exception as e:
            logger.error(f"Error requesting workload adjustment: {e}")

def run_parameter_server(config_path: str):
    """
    Run a parameter server node
    
    Args:
        config_path: Path to configuration file
    """
    try:
        # Load configuration
        with open(config_path, 'r') as f:
            config = json.load(f)
        
        # Create and start parameter server
        from training_pipeline import create_model
        
        # Load initial model
        model = create_model(config)
        
        # Create parameter server
        server = ParameterServer(config)
        server.start(model)
        
        try:
            # Keep server running
            while True:
                time.sleep(1.0)
        except KeyboardInterrupt:
            logger.info("Keyboard interrupt, stopping parameter server")
        finally:
            server.stop()
        
    except Exception as e:
        logger.error(f"Error running parameter server: {e}")
        sys.exit(1)

def run_worker(config_path: str, worker_id: Optional[str] = None):
    """
    Run a worker node
    
    Args:
        config_path: Path to configuration file
        worker_id: Unique identifier for this worker, generated if not provided
    """
    try:
        # Load configuration
        with open(config_path, 'r') as f:
            config = json.load(f)
        
        # Create and start worker
        from training_pipeline import create_model, PPOTrainer
        
        # Create empty model (will be updated from server)
        model = create_model(config)
        
        # Create worker
        worker = TrainingWorker(config, worker_id)
        trainer = PPOTrainer(config)
        
        worker.start(model)
        
        try:
            # Training loop
            while True:
                # Collect experiences
                experiences = trainer.collect_experiences(model, 
                                                         worker.current_workload)
                num_frames = len(experiences)
                worker.update_frames_processed(num_frames)
                
                # Compute gradients
                gradients = trainer.compute_gradients(model, experiences)
                
                # Send gradients to parameter server
                worker.send_gradients(gradients)
                
                # Brief pause to avoid overwhelming server
                time.sleep(0.1)
                
        except KeyboardInterrupt:
            logger.info("Keyboard interrupt, stopping worker")
        finally:
            worker.stop()
        
    except Exception as e:
        logger.error(f"Error running worker: {e}")
        sys.exit(1)

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="Distributed Training for FBNeo AI")
    parser.add_argument("--mode", choices=["server", "worker"], required=True,
                        help="Run as parameter server or worker")
    parser.add_argument("--config", required=True,
                        help="Path to configuration file")
    parser.add_argument("--worker-id", help="Unique identifier for worker node")
    
    args = parser.parse_args()
    
    if args.mode == "server":
        run_parameter_server(args.config)
    else:
        run_worker(args.config, args.worker_id) 