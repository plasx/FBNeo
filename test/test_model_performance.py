import os
import subprocess
import json
import time
import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime
from pathlib import Path

class ModelPerformanceTester:
    """
    Test harness for evaluating FBNeo AI model performance, latency, and correctness.
    """
    
    def __init__(self, fbneo_path, model_path, output_dir="./test_results"):
        """
        Initialize the tester with paths to FBNeo executable and model file.
        
        Args:
            fbneo_path: Path to the FBNeo executable
            model_path: Path to the AI model file (.pt or .mlmodel)
            output_dir: Directory to store test results
        """
        self.fbneo_path = Path(fbneo_path)
        self.model_path = Path(model_path)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True, parents=True)
        
        self.results = {}
        
    def test_inference_performance(self, rom_name, duration_seconds=60, 
                                  ai_enabled=True, headless=True, 
                                  collect_metrics=True, iterations=3):
        """
        Run the emulator with specified ROM and collect performance metrics.
        
        Args:
            rom_name: Name of the ROM to test
            duration_seconds: How long to run the test for
            ai_enabled: Whether to enable AI
            headless: Whether to run in headless mode
            collect_metrics: Whether to collect detailed metrics
            iterations: Number of test iterations for statistical significance
            
        Returns:
            Dictionary of performance metrics
        """
        print(f"Testing model performance on {rom_name}...")
        
        # Prepare log file
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        log_file = self.output_dir / f"{rom_name}_{timestamp}.log"
        
        all_metrics = []
        
        for i in range(iterations):
            print(f"  Running iteration {i+1}/{iterations}...")
            
            # Build command
            cmd = [
                str(self.fbneo_path),
                f"--rom={rom_name}",
                f"--ai-model={self.model_path}",
                "--ai-player=2"
            ]
            
            if ai_enabled:
                cmd.append("--ai-enable")
            
            if headless:
                cmd.append("--headless")
                
            if collect_metrics:
                cmd.append("--log-ai-perf")
                
            # Run the process with timeout
            try:
                with open(log_file, 'w') as f:
                    process = subprocess.Popen(cmd, stdout=f, stderr=f)
                    time.sleep(duration_seconds)
                    process.terminate()
                    process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                print("  Warning: Process did not terminate gracefully, killing...")
                process.kill()
            
            # Analyze the log file
            iteration_metrics = self._analyze_performance_log(log_file)
            if iteration_metrics:
                all_metrics.append(iteration_metrics)
        
        # Aggregate metrics across iterations
        if all_metrics:
            aggregated_metrics = {
                "rom_name": rom_name,
                "model_path": str(self.model_path),
                "timestamp": timestamp,
                "test_duration": duration_seconds,
                "iterations": iterations,
                "avg_inference_time_ms": np.mean([m["avg_inference_time_ms"] for m in all_metrics]),
                "max_inference_time_ms": np.max([m["max_inference_time_ms"] for m in all_metrics]),
                "min_inference_time_ms": np.min([m["min_inference_time_ms"] for m in all_metrics]),
                "std_inference_time_ms": np.mean([m["std_inference_time_ms"] for m in all_metrics]),
                "avg_frame_time_ms": np.mean([m["avg_frame_time_ms"] for m in all_metrics]),
                "inference_times": [m["inference_times"] for m in all_metrics][0],  # Keep one set for plotting
                "frame_times": [m["frame_times"] for m in all_metrics][0],  # Keep one set for plotting
            }
            
            # Store results
            self.results[f"{rom_name}_{timestamp}"] = aggregated_metrics
            
            # Generate plots
            self._generate_performance_plots(aggregated_metrics)
            
            return aggregated_metrics
        
        return None
    
    def _analyze_performance_log(self, log_file):
        """
        Process the log file to extract performance metrics.
        
        Args:
            log_file: Path to the log file
            
        Returns:
            Dictionary of performance metrics
        """
        inference_times = []
        frame_times = []
        
        try:
            with open(log_file, 'r') as f:
                for line in f:
                    if "AI_PERF" in line:
                        parts = line.strip().split()
                        if len(parts) >= 4:
                            # Extract the inference time (ms)
                            try:
                                inference_time = float(parts[2])
                                inference_times.append(inference_time)
                            except (ValueError, IndexError):
                                pass
                            
                            # Extract frame time if available
                            try:
                                frame_time = float(parts[3])
                                frame_times.append(frame_time)
                            except (ValueError, IndexError):
                                pass
            
            if inference_times:
                return {
                    "avg_inference_time_ms": np.mean(inference_times),
                    "max_inference_time_ms": np.max(inference_times),
                    "min_inference_time_ms": np.min(inference_times),
                    "std_inference_time_ms": np.std(inference_times),
                    "avg_frame_time_ms": np.mean(frame_times) if frame_times else 0,
                    "inference_times": inference_times,
                    "frame_times": frame_times,
                    "sample_count": len(inference_times)
                }
            else:
                print(f"Warning: No inference time data found in {log_file}")
                
        except Exception as e:
            print(f"Error analyzing log file: {e}")
            
        return None
    
    def _generate_performance_plots(self, metrics):
        """
        Generate performance visualization plots.
        
        Args:
            metrics: Dictionary of performance metrics
        """
        rom_name = metrics["rom_name"]
        timestamp = metrics["timestamp"]
        
        # Create plot directory
        plot_dir = self.output_dir / "plots"
        plot_dir.mkdir(exist_ok=True)
        
        # Plot inference time distribution
        plt.figure(figsize=(12, 6))
        
        plt.subplot(1, 2, 1)
        plt.hist(metrics["inference_times"], bins=30, alpha=0.7)
        plt.axvline(metrics["avg_inference_time_ms"], color='r', linestyle='dashed', 
                   linewidth=1, label=f'Mean: {metrics["avg_inference_time_ms"]:.2f} ms')
        plt.title(f"Inference Time Distribution - {rom_name}")
        plt.xlabel("Inference Time (ms)")
        plt.ylabel("Frequency")
        plt.legend()
        
        # Plot inference time over frames
        plt.subplot(1, 2, 2)
        plt.plot(metrics["inference_times"], alpha=0.7)
        plt.axhline(metrics["avg_inference_time_ms"], color='r', linestyle='dashed', 
                   linewidth=1, label=f'Mean: {metrics["avg_inference_time_ms"]:.2f} ms')
        plt.title(f"Inference Time Over Frames - {rom_name}")
        plt.xlabel("Frame Number")
        plt.ylabel("Inference Time (ms)")
        plt.legend()
        
        plt.tight_layout()
        plt.savefig(plot_dir / f"{rom_name}_{timestamp}_inference.png")
        
        # Plot frame times if available
        if metrics["frame_times"] and len(metrics["frame_times"]) > 0:
            plt.figure(figsize=(12, 6))
            
            plt.subplot(1, 2, 1)
            plt.hist(metrics["frame_times"], bins=30, alpha=0.7)
            plt.axvline(metrics["avg_frame_time_ms"], color='r', linestyle='dashed', 
                       linewidth=1, label=f'Mean: {metrics["avg_frame_time_ms"]:.2f} ms')
            plt.title(f"Frame Time Distribution - {rom_name}")
            plt.xlabel("Frame Time (ms)")
            plt.ylabel("Frequency")
            plt.legend()
            
            # Plot Frame time over time
            plt.subplot(1, 2, 2)
            plt.plot(metrics["frame_times"], alpha=0.7)
            target_frame_time = 16.67  # 60 FPS target
            plt.axhline(target_frame_time, color='g', linestyle='dashed', 
                       linewidth=1, label=f'Target: {target_frame_time:.2f} ms (60 FPS)')
            plt.axhline(metrics["avg_frame_time_ms"], color='r', linestyle='dashed', 
                       linewidth=1, label=f'Mean: {metrics["avg_frame_time_ms"]:.2f} ms')
            plt.title(f"Frame Time Over Time - {rom_name}")
            plt.xlabel("Frame Number")
            plt.ylabel("Frame Time (ms)")
            plt.legend()
            
            plt.tight_layout()
            plt.savefig(plot_dir / f"{rom_name}_{timestamp}_frame_time.png")
        
        plt.close('all')
    
    def compare_models(self, model_paths, rom_name, duration_seconds=30, headless=True):
        """
        Compare multiple models' performance on the same ROM.
        
        Args:
            model_paths: List of paths to model files
            rom_name: ROM to test with
            duration_seconds: Test duration per model
            headless: Whether to run in headless mode
            
        Returns:
            Comparison results
        """
        comparison = {}
        
        for model_path in model_paths:
            # Store current model path
            original_model_path = self.model_path
            self.model_path = Path(model_path)
            
            # Run test
            model_name = self.model_path.stem
            print(f"Testing model: {model_name}")
            metrics = self.test_inference_performance(
                rom_name, 
                duration_seconds=duration_seconds,
                headless=headless
            )
            
            if metrics:
                comparison[model_name] = {
                    "avg_inference_time_ms": metrics["avg_inference_time_ms"],
                    "max_inference_time_ms": metrics["max_inference_time_ms"],
                    "avg_frame_time_ms": metrics["avg_frame_time_ms"],
                }
            
            # Restore original model path
            self.model_path = original_model_path
        
        # Generate comparison plot
        self._generate_comparison_plot(comparison, rom_name)
        
        return comparison
    
    def _generate_comparison_plot(self, comparison, rom_name):
        """
        Generate a bar chart comparing model performance.
        
        Args:
            comparison: Dictionary of model comparison results
            rom_name: ROM that was tested
        """
        if not comparison:
            return
            
        # Create plot directory
        plot_dir = self.output_dir / "plots"
        plot_dir.mkdir(exist_ok=True)
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Prepare data
        models = list(comparison.keys())
        avg_times = [comparison[m]["avg_inference_time_ms"] for m in models]
        max_times = [comparison[m]["max_inference_time_ms"] for m in models]
        
        # Create plot
        plt.figure(figsize=(12, 6))
        x = np.arange(len(models))
        width = 0.35
        
        plt.bar(x - width/2, avg_times, width, label='Avg Inference Time (ms)')
        plt.bar(x + width/2, max_times, width, label='Max Inference Time (ms)')
        
        plt.xlabel('Model')
        plt.ylabel('Time (ms)')
        plt.title(f'Model Performance Comparison - {rom_name}')
        plt.xticks(x, models, rotation=45, ha='right')
        plt.legend()
        
        plt.tight_layout()
        plt.savefig(plot_dir / f"model_comparison_{rom_name}_{timestamp}.png")
        plt.close()
    
    def save_results(self, filename=None):
        """
        Save test results to a JSON file.
        
        Args:
            filename: Optional filename for results
            
        Returns:
            Path to the saved file
        """
        if not self.results:
            print("No results to save.")
            return None
            
        if filename is None:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"model_performance_results_{timestamp}.json"
        
        # Create a copy of results without large arrays for JSON serialization
        serializable_results = {}
        for key, result in self.results.items():
            serializable_results[key] = {k: v for k, v in result.items() if k not in ["inference_times", "frame_times"]}
            
            # Include summary statistics of arrays instead
            if "inference_times" in result:
                serializable_results[key]["inference_times_count"] = len(result["inference_times"])
                serializable_results[key]["inference_times_samples"] = result["inference_times"][:10]  # First 10 samples
                
            if "frame_times" in result:
                serializable_results[key]["frame_times_count"] = len(result["frame_times"])
                serializable_results[key]["frame_times_samples"] = result["frame_times"][:10]  # First 10 samples
            
        # Save to file
        file_path = self.output_dir / filename
        with open(file_path, 'w') as f:
            json.dump(serializable_results, f, indent=2)
            
        print(f"Results saved to {file_path}")
        return file_path


if __name__ == "__main__":
    # Example usage
    tester = ModelPerformanceTester(
        fbneo_path="/path/to/fbneo",
        model_path="/path/to/model.pt",
        output_dir="./test_results"
    )
    
    # Test single model
    metrics = tester.test_inference_performance(
        rom_name="sfiii3n",
        duration_seconds=30,
        ai_enabled=True,
        headless=True
    )
    
    # Compare multiple models
    models = [
        "/path/to/model_v1.pt",
        "/path/to/model_v2.pt",
        "/path/to/model_v3.mlmodel"
    ]
    
    comparison = tester.compare_models(
        model_paths=models,
        rom_name="sfiii3n",
        duration_seconds=30
    )
    
    # Save results
    tester.save_results() 