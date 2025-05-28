#!/usr/bin/env python3
# FBNeo AI Memory Mapper Test
# Validates memory extraction and normalization for different game architectures

import subprocess
import os
import json
import argparse
import time
from pathlib import Path
import matplotlib.pyplot as plt
import numpy as np
from typing import Dict, List, Any, Optional, Tuple

class MemoryMapperTester:
    """Tests memory mapping and extraction functionality"""
    
    def __init__(self, fbneo_path: str, output_dir: str = "test_output"):
        self.fbneo_path = Path(fbneo_path).absolute()
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True, parents=True)
        
        # Create subdirectories
        self.mappings_dir = self.output_dir / "mappings"
        self.data_dir = self.output_dir / "extracted_data"
        self.plots_dir = self.output_dir / "plots"
        
        for directory in [self.mappings_dir, self.data_dir, self.plots_dir]:
            directory.mkdir(exist_ok=True, parents=True)
        
        self.results = {
            "fbneo_path": str(self.fbneo_path),
            "tests": {},
            "summary": {
                "total_games": 0,
                "successful": 0,
                "failed": 0,
                "test_date": time.strftime("%Y-%m-%d %H:%M:%S")
            }
        }
    
    def extract_memory_mapping(self, rom_name: str, frames: int = 60) -> Dict[str, Any]:
        """Extracts memory mapping for a specific ROM"""
        mapping_file = self.mappings_dir / f"{rom_name}.json"
        
        # Run memory extraction
        cmd = [
            str(self.fbneo_path),
            "--extract-memory",
            rom_name,
            "--frames",
            str(frames),
            "--output",
            str(mapping_file)
        ]
        
        result = {
            "rom": rom_name,
            "command": " ".join(cmd),
            "success": False,
            "mapping_file": str(mapping_file),
            "error": None
        }
        
        try:
            print(f"Extracting memory mapping for {rom_name}...")
            process = subprocess.run(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=120
            )
            
            result["return_code"] = process.returncode
            result["success"] = process.returncode == 0 and mapping_file.exists()
            
            # If successful, load and validate the mapping file
            if result["success"]:
                with open(mapping_file, 'r') as f:
                    mapping = json.load(f)
                
                result["architecture"] = mapping.get("architecture", "unknown")
                result["variables"] = len(mapping.get("variables", {}))
                result["valid_structure"] = self._validate_mapping_structure(mapping)
                result["success"] = result["valid_structure"]
            else:
                result["error"] = "Failed to create mapping file"
                if process.stderr:
                    result["error_details"] = process.stderr
        
        except subprocess.TimeoutExpired:
            result["error"] = f"Command timed out after 120 seconds"
        except Exception as e:
            result["error"] = str(e)
        
        self.results["tests"][rom_name] = result
        
        # Update summary
        self.results["summary"]["total_games"] += 1
        if result["success"]:
            self.results["summary"]["successful"] += 1
        else:
            self.results["summary"]["failed"] += 1
        
        return result
    
    def _validate_mapping_structure(self, mapping: Dict[str, Any]) -> bool:
        """Validates the structure of a memory mapping file"""
        required_fields = ["game_name", "architecture", "variables"]
        
        # Check for required fields
        for field in required_fields:
            if field not in mapping:
                print(f"Missing required field: {field}")
                return False
        
        # Check variables structure
        variables = mapping.get("variables", {})
        if not variables:
            print("No variables defined in mapping")
            return False
        
        for var_name, var_info in variables.items():
            if "address" not in var_info:
                print(f"Variable {var_name} missing address")
                return False
            if "data_type" not in var_info:
                print(f"Variable {var_name} missing data_type")
                return False
        
        return True
    
    def extract_sample_data(self, rom_name: str, frames: int = 300) -> Dict[str, Any]:
        """Extracts sample data using a memory mapping file"""
        mapping_file = self.mappings_dir / f"{rom_name}.json"
        data_file = self.data_dir / f"{rom_name}_data.jsonl"
        
        if not mapping_file.exists():
            return {
                "rom": rom_name,
                "success": False,
                "error": "Mapping file does not exist"
            }
        
        # Run data extraction
        cmd = [
            str(self.fbneo_path),
            "--collect",
            rom_name,
            "--frames",
            str(frames),
            "--mapping",
            str(mapping_file),
            "--output",
            str(data_file)
        ]
        
        result = {
            "rom": rom_name,
            "command": " ".join(cmd),
            "success": False,
            "data_file": str(data_file),
            "error": None
        }
        
        try:
            print(f"Extracting sample data for {rom_name}...")
            process = subprocess.run(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=180
            )
            
            result["return_code"] = process.returncode
            result["success"] = process.returncode == 0 and data_file.exists()
            
            # If successful, analyze the data file
            if result["success"]:
                result["data_size"] = data_file.stat().st_size
                result["frame_count"] = self._count_frames(data_file)
                result["variable_stats"] = self._analyze_data_variables(data_file)
                self._generate_variable_plots(rom_name, result["variable_stats"])
            else:
                result["error"] = "Failed to create data file"
                if process.stderr:
                    result["error_details"] = process.stderr
        
        except subprocess.TimeoutExpired:
            result["error"] = f"Command timed out after 180 seconds"
        except Exception as e:
            result["error"] = str(e)
        
        if rom_name in self.results["tests"]:
            self.results["tests"][rom_name]["data_extraction"] = result
        else:
            self.results["tests"][rom_name] = {"data_extraction": result}
        
        return result
    
    def _count_frames(self, data_file: Path) -> int:
        """Counts the number of frames in a data file"""
        count = 0
        with open(data_file, 'r') as f:
            for line in f:
                count += 1
        return count
    
    def _analyze_data_variables(self, data_file: Path) -> Dict[str, Dict[str, Any]]:
        """Analyzes variables in the extracted data"""
        variables = {}
        frame_data = []
        
        # Read all frames
        with open(data_file, 'r') as f:
            for line in f:
                try:
                    frame = json.loads(line)
                    frame_data.append(frame)
                except json.JSONDecodeError:
                    continue
        
        if not frame_data:
            return variables
        
        # Get all variable names from the first frame
        first_frame = frame_data[0]["input_frame"]
        
        # Extract player1 variables
        if "player1" in first_frame:
            for var_name in first_frame["player1"]:
                key = f"player1.{var_name}"
                variables[key] = {"values": [], "min": None, "max": None, "mean": None}
        
        # Extract player2 variables
        if "player2" in first_frame:
            for var_name in first_frame["player2"]:
                key = f"player2.{var_name}"
                variables[key] = {"values": [], "min": None, "max": None, "mean": None}
        
        # Extract global variables
        for var_name, value in first_frame.items():
            if var_name not in ["player1", "player2"]:
                variables[var_name] = {"values": [], "min": None, "max": None, "mean": None}
        
        # Collect values for each variable across all frames
        for frame in frame_data:
            frame_data = frame["input_frame"]
            
            # Extract player1 variables
            if "player1" in frame_data:
                for var_name, value in frame_data["player1"].items():
                    key = f"player1.{var_name}"
                    if key in variables:
                        variables[key]["values"].append(value)
            
            # Extract player2 variables
            if "player2" in frame_data:
                for var_name, value in frame_data["player2"].items():
                    key = f"player2.{var_name}"
                    if key in variables:
                        variables[key]["values"].append(value)
            
            # Extract global variables
            for var_name, value in frame_data.items():
                if var_name not in ["player1", "player2"] and var_name in variables:
                    variables[var_name]["values"].append(value)
        
        # Calculate statistics for each variable
        for var_name, data in variables.items():
            if data["values"]:
                # Only include numerical values for statistics
                numeric_values = [v for v in data["values"] if isinstance(v, (int, float))]
                if numeric_values:
                    data["min"] = min(numeric_values)
                    data["max"] = max(numeric_values)
                    data["mean"] = sum(numeric_values) / len(numeric_values)
                    data["range"] = data["max"] - data["min"]
                    data["unique_values"] = len(set(numeric_values))
                data["type"] = "numeric" if numeric_values else "non-numeric"
                data["sample"] = data["values"][:5]  # First few values as sample
        
        return variables
    
    def _generate_variable_plots(self, rom_name: str, variable_stats: Dict[str, Dict[str, Any]]) -> None:
        """Generates plots for selected variables over time"""
        key_vars = []
        
        # Select variables that likely show interesting patterns
        for var_name, stats in variable_stats.items():
            if stats.get("type") == "numeric" and stats.get("range", 0) > 0:
                if "health" in var_name.lower() or "position" in var_name.lower() or \
                   "energy" in var_name.lower() or "state" in var_name.lower():
                    key_vars.append(var_name)
        
        # Limit to most interesting variables
        key_vars = key_vars[:6]  # Plot at most 6 variables
        
        if not key_vars:
            return
        
        # Create figure with subplots
        fig, axes = plt.subplots(len(key_vars), 1, figsize=(10, 2 * len(key_vars)))
        if len(key_vars) == 1:
            axes = [axes]
        
        for i, var_name in enumerate(key_vars):
            values = variable_stats[var_name]["values"]
            numeric_values = [v for v in values if isinstance(v, (int, float))]
            
            if not numeric_values:
                continue
                
            axes[i].plot(numeric_values)
            axes[i].set_title(f"{var_name}")
            axes[i].set_xlabel("Frame")
            axes[i].set_ylabel("Value")
            
            # Add min/max lines
            min_val = variable_stats[var_name]["min"]
            max_val = variable_stats[var_name]["max"]
            axes[i].axhline(y=min_val, color='r', linestyle='--', alpha=0.3)
            axes[i].axhline(y=max_val, color='r', linestyle='--', alpha=0.3)
        
        plt.tight_layout()
        plot_file = self.plots_dir / f"{rom_name}_variables.png"
        plt.savefig(plot_file)
        plt.close(fig)
    
    def test_game(self, rom_name: str, frames: int = 300) -> Dict[str, Any]:
        """Test memory mapping and data extraction for a single game"""
        print(f"\n=== Testing {rom_name} ===")
        
        # Step 1: Extract memory mapping
        mapping_result = self.extract_memory_mapping(rom_name)
        
        # Step 2: If mapping successful, extract sample data
        if mapping_result["success"]:
            data_result = self.extract_sample_data(rom_name, frames)
            success = data_result["success"]
        else:
            success = False
        
        print(f"Testing {rom_name}: {'SUCCESS' if success else 'FAILED'}")
        return self.results["tests"][rom_name]
    
    def test_multiple_games(self, rom_names: List[str], frames: int = 300) -> None:
        """Test memory mapping and data extraction for multiple games"""
        for rom_name in rom_names:
            self.test_game(rom_name, frames)
        
        self.save_results()
    
    def save_results(self) -> None:
        """Save test results to a JSON file"""
        output_path = self.output_dir / "memory_mapper_test_results.json"
        with open(output_path, 'w') as f:
            json.dump(self.results, f, indent=2)
            
        # Print summary
        print("\nTest Summary:")
        print(f"Total games tested: {self.results['summary']['total_games']}")
        print(f"Successful: {self.results['summary']['successful']}")
        print(f"Failed: {self.results['summary']['failed']}")
        print(f"Results saved to: {output_path}")

def main():
    parser = argparse.ArgumentParser(description="Test FBNeo AI memory mapping functionality")
    parser.add_argument("--fbneo", required=True, help="Path to FBNeo executable")
    parser.add_argument("--roms", required=True, nargs="+", help="ROM names to test")
    parser.add_argument("--frames", type=int, default=300, help="Number of frames to capture")
    parser.add_argument("--output", default="test_output", help="Output directory for test results")
    
    args = parser.parse_args()
    
    tester = MemoryMapperTester(args.fbneo, args.output)
    tester.test_multiple_games(args.roms, args.frames)

if __name__ == "__main__":
    main() 