#!/usr/bin/env python3
"""
Memory Test Suite for FBNeo Games

This script provides a framework for testing memory mappings against actual game states
to verify mapping accuracy and detect regressions.
"""

import argparse
import json
import os
import sys
import time
import subprocess
import tempfile
import glob
from pathlib import Path
import numpy as np
from collections import defaultdict, Counter

class MemoryTestSuite:
    def __init__(self, game_name, architecture=None, mapping_file=None):
        self.game_name = game_name
        self.architecture = architecture
        self.mapping_file = mapping_file
        self.mapping_data = None
        self.test_cases = []
        self.fbneo_path = self.find_fbneo_executable()
        self.results = {
            "game": game_name,
            "passed": 0,
            "failed": 0,
            "skipped": 0,
            "test_results": []
        }
        
    def find_fbneo_executable(self):
        """Find the FBNeo executable path based on the platform."""
        possible_paths = []
        
        if os.name == 'nt':  # Windows
            possible_paths = [
                "FBNeo.exe",
                os.path.join(".", "FBNeo.exe"),
                os.path.join("..", "FBNeo.exe"),
                r"C:\Program Files\FBNeo\FBNeo.exe",
                r"C:\Program Files (x86)\FBNeo\FBNeo.exe"
            ]
        elif sys.platform == 'darwin':  # macOS
            possible_paths = [
                "FBNeo",
                os.path.join(".", "FBNeo"),
                os.path.join("..", "FBNeo"),
                os.path.join(os.path.expanduser("~"), "Applications", "FBNeo.app", "Contents", "MacOS", "FBNeo"),
                os.path.join("/Applications", "FBNeo.app", "Contents", "MacOS", "FBNeo")
            ]
        else:  # Linux and others
            possible_paths = [
                "fbneo",
                "./fbneo",
                "../fbneo",
                os.path.join(os.path.expanduser("~"), ".local", "bin", "fbneo"),
                "/usr/local/bin/fbneo",
                "/usr/bin/fbneo"
            ]
            
        # Check if any of the paths exist
        for path in possible_paths:
            if os.path.isfile(path):
                return path
                
        # If no path found, return a default path and warn user
        print("Warning: Could not find FBNeo executable. Please specify the path manually.")
        return "FBNeo"
        
    def load_mapping(self):
        """Load the memory mapping from the file."""
        if not self.mapping_file:
            # Try to find mapping in common locations
            mapping_paths = [
                os.path.join("src", "ai", "mappings", f"{self.game_name}.json"),
                os.path.join("mappings", f"{self.game_name}.json"),
                os.path.join("..", "mappings", f"{self.game_name}.json"),
                f"{self.game_name}.json"
            ]
            
            for path in mapping_paths:
                if os.path.isfile(path):
                    self.mapping_file = path
                    break
            
            if not self.mapping_file:
                print(f"Error: Could not find mapping file for {self.game_name}")
                return False
        
        try:
            with open(self.mapping_file, 'r') as f:
                self.mapping_data = json.load(f)
                
            if not self.architecture:
                self.architecture = self.mapping_data.get("architecture")
                
            print(f"Loaded mapping for {self.mapping_data.get('game', self.game_name)} ({self.architecture})")
            return True
                
        except Exception as e:
            print(f"Error loading mapping file: {e}")
            return False
            
    def load_test_cases(self, test_case_file):
        """Load test cases from a JSON file."""
        try:
            with open(test_case_file, 'r') as f:
                data = json.load(f)
                
            self.test_cases = data.get("test_cases", [])
            print(f"Loaded {len(self.test_cases)} test cases from {test_case_file}")
            return True
                
        except Exception as e:
            print(f"Error loading test cases: {e}")
            return False
            
    def create_test_case(self, name, description=None, setup_steps=None):
        """Create a new test case."""
        test_case = {
            "name": name,
            "description": description or f"Test case for {name}",
            "setup_steps": setup_steps or [],
            "expected_values": {},
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
        }
        
        self.test_cases.append(test_case)
        return test_case
        
    def record_expected_values(self, test_case_index):
        """
        Launch FBNeo and record expected values for a test case.
        This records the memory values for the current state to use for comparison.
        """
        if test_case_index >= len(self.test_cases):
            print(f"Error: Test case index {test_case_index} is out of range")
            return False
            
        test_case = self.test_cases[test_case_index]
        print(f"Recording expected values for test case: {test_case['name']}")
        print("Follow these steps to set up the game state:")
        
        for i, step in enumerate(test_case.get("setup_steps", [])):
            print(f"{i+1}. {step}")
            
        print("\nWhen the game state is ready, press 'R' to record the values.")
        print("Press 'Q' to quit without recording.")
        
        # Create a temp file for memory dump
        with tempfile.NamedTemporaryFile(delete=False) as tmp:
            temp_file = tmp.name
            
        # Launch FBNeo with memory dump capability
        cmd = [
            self.fbneo_path,
            f"--memory-dump-file={temp_file}",
            f"--gamename={self.game_name}"
        ]
        
        try:
            process = subprocess.Popen(cmd)
            
            # Wait for the process to complete
            process.wait()
            
            # Check if the memory dump file exists and is non-empty
            if os.path.exists(temp_file) and os.path.getsize(temp_file) > 0:
                # Parse the memory dump file
                memory_values = self._parse_memory_dump(temp_file)
                
                # Record expected values only for addresses in the mapping
                for mapping in self.mapping_data.get("mappings", []):
                    addr = mapping.get("address")
                    if addr in memory_values:
                        test_case["expected_values"][addr] = memory_values[addr]
                        
                print(f"Recorded {len(test_case['expected_values'])} memory values")
                return True
            else:
                print("Error: No memory dump was created")
                return False
                
        except Exception as e:
            print(f"Error recording expected values: {e}")
            return False
        finally:
            # Clean up temp file
            if os.path.exists(temp_file):
                os.remove(temp_file)
                
    def _parse_memory_dump(self, dump_file):
        """Parse a memory dump file into a dictionary of address:value pairs."""
        memory_values = {}
        
        try:
            with open(dump_file, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line and ":" in line:
                        parts = line.split(":", 1)
                        addr = parts[0].strip()
                        value = parts[1].strip()
                        
                        # Convert value to integer
                        try:
                            if value.lower().startswith("0x"):
                                value = int(value, 16)
                            else:
                                value = int(value)
                        except ValueError:
                            pass  # Keep as string if not convertible
                            
                        memory_values[addr] = value
                        
            return memory_values
                        
        except Exception as e:
            print(f"Error parsing memory dump: {e}")
            return {}
            
    def run_tests(self):
        """Run all test cases and collect results."""
        if not self.mapping_data:
            print("Error: No mapping data loaded")
            return False
            
        if not self.test_cases:
            print("Error: No test cases loaded")
            return False
            
        print(f"Running {len(self.test_cases)} test cases...")
        
        for i, test_case in enumerate(self.test_cases):
            print(f"\nTest Case {i+1}: {test_case['name']}")
            print(f"Description: {test_case['description']}")
            
            if not test_case.get("expected_values"):
                print("Skipping: No expected values defined")
                self.results["skipped"] += 1
                self.results["test_results"].append({
                    "name": test_case["name"],
                    "status": "skipped",
                    "reason": "No expected values defined"
                })
                continue
                
            # Display setup steps
            print("\nSetup Steps:")
            for j, step in enumerate(test_case.get("setup_steps", [])):
                print(f"{j+1}. {step}")
                
            # Manual testing mode - ask user to set up the game state
            input("\nPlease set up the game state according to the steps above, then press Enter to continue...")
            
            # Create a temp file for memory dump
            with tempfile.NamedTemporaryFile(delete=False) as tmp:
                temp_file = tmp.name
                
            # Launch FBNeo with memory dump capability
            cmd = [
                self.fbneo_path,
                f"--memory-dump-file={temp_file}",
                f"--gamename={self.game_name}"
            ]
            
            try:
                print("\nLaunching FBNeo. Press 'D' to dump memory when the game state is ready.")
                print("Press 'Q' to quit without testing.")
                
                process = subprocess.Popen(cmd)
                process.wait()
                
                # Check if the memory dump file exists and is non-empty
                if os.path.exists(temp_file) and os.path.getsize(temp_file) > 0:
                    # Parse the memory dump file
                    actual_values = self._parse_memory_dump(temp_file)
                    
                    # Compare with expected values
                    test_result = {
                        "name": test_case["name"],
                        "status": "passed",
                        "details": []
                    }
                    
                    for addr, expected in test_case["expected_values"].items():
                        actual = actual_values.get(addr)
                        
                        if actual is None:
                            test_result["details"].append({
                                "address": addr,
                                "expected": expected,
                                "actual": "Not found",
                                "passed": False
                            })
                            test_result["status"] = "failed"
                        elif actual != expected:
                            test_result["details"].append({
                                "address": addr,
                                "expected": expected,
                                "actual": actual,
                                "passed": False
                            })
                            test_result["status"] = "failed"
                        else:
                            test_result["details"].append({
                                "address": addr,
                                "expected": expected,
                                "actual": actual,
                                "passed": True
                            })
                            
                    # Update test result counts
                    if test_result["status"] == "passed":
                        self.results["passed"] += 1
                        print("Test PASSED")
                    else:
                        self.results["failed"] += 1
                        print("Test FAILED")
                        
                        # Print details of failures
                        print("\nFailures:")
                        for detail in test_result["details"]:
                            if not detail["passed"]:
                                addr = detail["address"]
                                expected = detail["expected"]
                                actual = detail["actual"]
                                print(f"  Address {addr}: Expected {expected}, got {actual}")
                                
                    self.results["test_results"].append(test_result)
                else:
                    print("Error: No memory dump was created")
                    self.results["skipped"] += 1
                    self.results["test_results"].append({
                        "name": test_case["name"],
                        "status": "skipped",
                        "reason": "Memory dump failed"
                    })
                    
            except Exception as e:
                print(f"Error running test: {e}")
                self.results["skipped"] += 1
                self.results["test_results"].append({
                    "name": test_case["name"],
                    "status": "skipped",
                    "reason": str(e)
                })
            finally:
                # Clean up temp file
                if os.path.exists(temp_file):
                    os.remove(temp_file)
                    
        # Print summary
        print("\nTest Summary:")
        print(f"Total: {len(self.test_cases)}")
        print(f"Passed: {self.results['passed']}")
        print(f"Failed: {self.results['failed']}")
        print(f"Skipped: {self.results['skipped']}")
        
        return self.results
        
    def save_test_cases(self, output_file):
        """Save test cases to a JSON file."""
        if not self.test_cases:
            print("No test cases to save")
            return False
            
        test_suite = {
            "game": self.game_name,
            "architecture": self.architecture,
            "mapping_file": self.mapping_file,
            "test_cases": self.test_cases
        }
        
        try:
            output_path = Path(output_file)
            output_path.parent.mkdir(exist_ok=True, parents=True)
            
            with open(output_path, 'w') as f:
                json.dump(test_suite, f, indent=2)
                
            print(f"Test cases saved to {output_file}")
            return True
        except Exception as e:
            print(f"Error saving test cases: {e}")
            return False
            
    def save_results(self, output_file):
        """Save test results to a JSON file."""
        if not self.results or not self.results.get("test_results"):
            print("No test results to save")
            return False
            
        try:
            output_path = Path(output_file)
            output_path.parent.mkdir(exist_ok=True, parents=True)
            
            with open(output_path, 'w') as f:
                json.dump(self.results, f, indent=2)
                
            print(f"Test results saved to {output_file}")
            return True
        except Exception as e:
            print(f"Error saving test results: {e}")
            return False
            
    def create_standard_test_suite(self):
        """Create a standard set of test cases for typical fighting game mappings."""
        if not self.mapping_data:
            print("Error: No mapping data loaded")
            return False
            
        # Clear existing test cases
        self.test_cases = []
        
        # Create test case for idle state
        idle_test = self.create_test_case(
            "idle_state",
            "Test mapping values in the idle state (character select screen)",
            [
                "Start the game",
                "Navigate to the character select screen",
                "Do not select any character yet"
            ]
        )
        
        # Create test case for character select
        char_select_test = self.create_test_case(
            "character_select",
            "Test mapping values after selecting characters",
            [
                "Start the game",
                "Navigate to the character select screen",
                "Select a character for both players",
                "Wait for the match to be about to start, but before actual gameplay"
            ]
        )
        
        # Create test case for round start
        round_start_test = self.create_test_case(
            "round_start",
            "Test mapping values at the start of a round",
            [
                "Start a match",
                "Wait for the round to begin",
                "Don't make any moves yet"
            ]
        )
        
        # Create test case for mid-round
        mid_round_test = self.create_test_case(
            "mid_round",
            "Test mapping values during active gameplay",
            [
                "Start a match",
                "Play for a few seconds",
                "Make sure both players have taken some damage",
                "Pause at an interesting point in the match"
            ]
        )
        
        # Create test case for round end
        round_end_test = self.create_test_case(
            "round_end",
            "Test mapping values at the end of a round",
            [
                "Play a match until one player wins a round",
                "Capture the state right after the round victory announcement"
            ]
        )
        
        print(f"Created {len(self.test_cases)} standard test cases")
        return True

def main():
    parser = argparse.ArgumentParser(description="Memory Test Suite for FBNeo Games")
    parser.add_argument("--game", required=True, help="Game name (ROM name)")
    parser.add_argument("--architecture", help="Game architecture (e.g., CPS2, NEOGEO)")
    parser.add_argument("--mapping-file", help="Path to the mapping JSON file")
    parser.add_argument("--test-case-file", help="Path to load test cases from")
    parser.add_argument("--save-test-cases", help="Path to save test cases to")
    parser.add_argument("--save-results", help="Path to save test results to")
    parser.add_argument("--record", type=int, help="Record expected values for test case at given index")
    parser.add_argument("--create-standard", action="store_true", help="Create standard test cases")
    parser.add_argument("--run", action="store_true", help="Run all test cases")
    
    args = parser.parse_args()
    
    test_suite = MemoryTestSuite(args.game, args.architecture, args.mapping_file)
    
    if not test_suite.load_mapping():
        sys.exit(1)
        
    if args.test_case_file:
        if not test_suite.load_test_cases(args.test_case_file):
            sys.exit(1)
    
    if args.create_standard:
        test_suite.create_standard_test_suite()
        
    if args.record is not None:
        if not test_suite.record_expected_values(args.record):
            sys.exit(1)
            
    if args.run:
        test_suite.run_tests()
        
    if args.save_test_cases:
        test_suite.save_test_cases(args.save_test_cases)
        
    if args.save_results:
        test_suite.save_results(args.save_results)

if __name__ == "__main__":
    main() 