#!/usr/bin/env python3
# FBNeo AI CLI Command Coverage Test
# Tests all AI-related command line arguments to ensure they function correctly

import subprocess
import os
import sys
import json
import time
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Any

class FBNeoCliTester:
    """Test runner for FBNeo AI CLI commands"""
    
    def __init__(self, fbneo_path: str, test_rom: str, output_dir: str = "test_output"):
        self.fbneo_path = Path(fbneo_path).absolute()
        self.test_rom = test_rom
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True, parents=True)
        
        # Create subdirectories
        self.datasets_dir = self.output_dir / "datasets"
        self.models_dir = self.output_dir / "models"
        self.exports_dir = self.output_dir / "exports"
        self.mappings_dir = self.output_dir / "mappings"
        
        for directory in [self.datasets_dir, self.models_dir, self.exports_dir, self.mappings_dir]:
            directory.mkdir(exist_ok=True, parents=True)
        
        self.results = {
            "fbneo_path": str(self.fbneo_path),
            "test_rom": test_rom,
            "commands": {},
            "summary": {
                "total": 0,
                "successful": 0,
                "failed": 0,
                "test_date": time.strftime("%Y-%m-%d %H:%M:%S")
            }
        }
    
    def run_command(self, name: str, args: List[str], timeout: int = 30) -> Dict[str, Any]:
        """Run a single command and record the results"""
        full_command = [str(self.fbneo_path)] + args
        result = {
            "command": " ".join(full_command),
            "args": args,
            "success": False,
            "return_code": None,
            "duration": 0,
            "output_size": 0,
            "error": None
        }
        
        start_time = time.time()
        try:
            # Run the command and capture output
            process = subprocess.run(
                full_command,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=timeout
            )
            
            result["return_code"] = process.returncode
            result["success"] = process.returncode == 0
            result["stdout"] = process.stdout[:500] + "..." if len(process.stdout) > 500 else process.stdout
            result["stderr"] = process.stderr[:500] + "..." if len(process.stderr) > 500 else process.stderr
            
            # Check for expected output files based on command
            if "--collect" in args:
                dataset_path = self.datasets_dir / f"{self.test_rom}.jsonl"
                if dataset_path.exists():
                    result["output_size"] = dataset_path.stat().st_size
            
            elif "--train" in args:
                model_path = self.models_dir / f"{self.test_rom}.pt"
                if model_path.exists():
                    result["output_size"] = model_path.stat().st_size
            
            elif "--export" in args:
                export_path = self.exports_dir / f"{self.test_rom}.mlmodel"
                if export_path.exists():
                    result["output_size"] = export_path.stat().st_size
            
            elif "--extract-memory" in args:
                mapping_path = self.mappings_dir / f"{self.test_rom}.json"
                if mapping_path.exists():
                    result["output_size"] = mapping_path.stat().st_size
            
        except subprocess.TimeoutExpired:
            result["error"] = f"Command timed out after {timeout} seconds"
        except Exception as e:
            result["error"] = str(e)
        
        result["duration"] = round(time.time() - start_time, 2)
        self.results["commands"][name] = result
        
        # Update summary
        self.results["summary"]["total"] += 1
        if result["success"]:
            self.results["summary"]["successful"] += 1
        else:
            self.results["summary"]["failed"] += 1
        
        print(f"Test {name}: {'SUCCESS' if result['success'] else 'FAILED'} ({result['duration']}s)")
        if result["error"]:
            print(f"  Error: {result['error']}")
        
        return result
    
    def run_all_tests(self):
        """Run all CLI command tests"""
        # Memory Extraction Commands
        self.run_command(
            "extract_memory", 
            ["--extract-memory", self.test_rom, "--frames", "60"]
        )
        
        # Dataset Commands
        self.run_command(
            "collect_dataset", 
            ["--collect", self.test_rom, "--frames", "60", "--output", str(self.datasets_dir)]
        )
        
        self.run_command(
            "collect_with_random_ai", 
            ["--collect", self.test_rom, "--frames", "60", "--ai-opponent", "random", 
             "--output", str(self.datasets_dir)]
        )
        
        # Model Training Commands
        self.run_command(
            "train_model", 
            ["--train", self.test_rom, "--dataset", str(self.datasets_dir / f"{self.test_rom}.jsonl"),
             "--epochs", "1", "--output", str(self.models_dir)]
        )
        
        # Model Export Commands
        self.run_command(
            "export_model", 
            ["--export", str(self.models_dir / f"{self.test_rom}.pt"), 
             "--output", str(self.exports_dir)]
        )
        
        # AI Gameplay Commands
        self.run_command(
            "play_with_random_ai", 
            ["--play", self.test_rom, "--ai-opponent", "random", "--no-audio"],
            timeout=5  # Short timeout as this launches the UI
        )
        
        self.run_command(
            "play_with_model", 
            ["--play", self.test_rom, "--ai-opponent", "model", 
             "--model", str(self.models_dir / f"{self.test_rom}.pt"), "--no-audio"],
            timeout=5  # Short timeout as this launches the UI
        )
        
        # Headless Mode Commands
        self.run_command(
            "headless_random", 
            ["--headless", self.test_rom, "--ai-opponent", "random", "--frames", "60"]
        )
        
        self.run_command(
            "headless_model", 
            ["--headless", self.test_rom, "--ai-opponent", "model", 
             "--model", str(self.models_dir / f"{self.test_rom}.pt"), "--frames", "60"]
        )
        
        # Save results
        self.save_results()
    
    def save_results(self):
        """Save test results to a JSON file"""
        output_path = self.output_dir / "cli_test_results.json"
        with open(output_path, 'w') as f:
            json.dump(self.results, f, indent=2)
            
        # Print summary
        print("\nTest Summary:")
        print(f"Total tests: {self.results['summary']['total']}")
        print(f"Successful: {self.results['summary']['successful']}")
        print(f"Failed: {self.results['summary']['failed']}")
        print(f"Results saved to: {output_path}")

def main():
    parser = argparse.ArgumentParser(description="Test FBNeo AI CLI commands")
    parser.add_argument("--fbneo", required=True, help="Path to FBNeo executable")
    parser.add_argument("--rom", required=True, help="ROM name to use for testing")
    parser.add_argument("--output", default="test_output", help="Output directory for test results")
    
    args = parser.parse_args()
    
    tester = FBNeoCliTester(args.fbneo, args.rom, args.output)
    tester.run_all_tests()

if __name__ == "__main__":
    main() 