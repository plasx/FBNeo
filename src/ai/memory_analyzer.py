#!/usr/bin/env python3
"""
Memory Analyzer for FBNeo Games

This script analyzes memory samples collected from FBNeo games and suggests
potential memory mappings based on pattern analysis.
"""

import argparse
import json
import os
import sys
from pathlib import Path
import numpy as np
from collections import defaultdict, Counter
import matplotlib.pyplot as plt
import re

class MemoryAnalyzer:
    def __init__(self, sample_file):
        self.sample_file = sample_file
        self.samples = []
        self.game_name = ""
        self.architecture = ""
        self.memory_data = {}  # Address -> list of values
        self.sample_count = 0
        self.analysis_results = {}
        
    def load_samples(self):
        """Load memory samples from the JSON file."""
        try:
            with open(self.sample_file, 'r') as f:
                data = json.load(f)
                
            self.game_name = data.get("game", "unknown")
            self.architecture = data.get("architecture", "unknown")
            self.samples = data.get("samples", [])
            self.sample_count = len(self.samples)
            
            if self.sample_count == 0:
                print("Error: No samples found in the file.")
                return False
                
            print(f"Loaded {self.sample_count} samples for {self.game_name} ({self.architecture})")
            return True
            
        except Exception as e:
            print(f"Error loading samples: {e}")
            return False
    
    def preprocess_data(self):
        """Preprocess sample data into a structured format for analysis."""
        # Initialize memory data dictionary
        self.memory_data = defaultdict(list)
        
        # Process each sample
        for sample in self.samples:
            memory = sample.get("memory", {})
            
            # Add each memory value to its address list
            for addr, value in memory.items():
                # Convert hex values to integers for easier analysis
                try:
                    # Strip any '0x' prefix if present
                    if isinstance(value, str) and value.lower().startswith('0x'):
                        value = value[2:]
                    
                    # Convert to integer
                    int_value = int(value, 16) if isinstance(value, str) else int(value)
                    self.memory_data[addr].append(int_value)
                except ValueError:
                    # Skip values that can't be converted to integers
                    continue
        
        print(f"Preprocessed data for {len(self.memory_data)} memory addresses")
        return len(self.memory_data) > 0
    
    def analyze(self):
        """Perform various analyses on the memory data."""
        if not self.memory_data:
            print("No data to analyze. Run preprocess_data first.")
            return False
        
        # Initialize results
        self.analysis_results = {
            "game": self.game_name,
            "architecture": self.architecture,
            "sample_count": self.sample_count,
            "address_count": len(self.memory_data),
            "constant_values": {},
            "high_variance": {},
            "binary_values": {},
            "counter_values": {},
            "potential_health": [],
            "potential_position": [],
            "potential_timer": []
        }
        
        # Analyze each memory address
        for addr, values in self.memory_data.items():
            # Skip addresses with no values
            if not values:
                continue
                
            # Convert to numpy array for easier analysis
            values_array = np.array(values)
            
            # Check if values are constant
            if np.all(values_array == values_array[0]):
                self.analysis_results["constant_values"][addr] = {
                    "value": hex(values_array[0]),
                    "decimal": int(values_array[0])
                }
                continue
            
            # Calculate statistics
            min_val = np.min(values_array)
            max_val = np.max(values_array)
            mean_val = np.mean(values_array)
            variance = np.var(values_array)
            changes = np.sum(np.diff(values_array) != 0)
            unique_vals = len(np.unique(values_array))
            
            # Record high variance addresses
            if variance > 1000 and changes > self.sample_count / 10:
                self.analysis_results["high_variance"][addr] = {
                    "min": int(min_val),
                    "max": int(max_val),
                    "mean": float(mean_val),
                    "variance": float(variance),
                    "changes": int(changes),
                    "unique_values": int(unique_vals)
                }
            
            # Check for binary-like values (only 0 and 1, or only a few values)
            if unique_vals <= 2 and changes > 0:
                self.analysis_results["binary_values"][addr] = {
                    "values": [int(v) for v in np.unique(values_array)],
                    "changes": int(changes)
                }
                
            # Check for counter-like values (steadily increasing)
            diffs = np.diff(values_array)
            if np.all(diffs >= 0) and changes > self.sample_count / 3:
                self.analysis_results["counter_values"][addr] = {
                    "start": int(values_array[0]),
                    "end": int(values_array[-1]),
                    "changes": int(changes)
                }
                
            # Check for potential health values (decreasing, within typical range)
            if 0 <= min_val <= max_val <= 255 and np.mean(diffs) < 0:
                self.analysis_results["potential_health"].append({
                    "address": addr,
                    "min": int(min_val),
                    "max": int(max_val),
                    "changes": int(changes)
                })
                
            # Check for potential position values
            if variance > 100 and changes > self.sample_count / 5:
                # Look for x,y coordinate pairs (adjacent addresses)
                addr_num = int(addr, 16) if isinstance(addr, str) else int(addr)
                next_addr = hex(addr_num + 1) if isinstance(addr, str) else addr_num + 1
                
                if next_addr in self.memory_data and np.var(np.array(self.memory_data[next_addr])) > 100:
                    self.analysis_results["potential_position"].append({
                        "x_address": addr,
                        "y_address": str(next_addr) if isinstance(next_addr, int) else next_addr,
                        "x_variance": float(variance),
                        "y_variance": float(np.var(np.array(self.memory_data[next_addr])))
                    })
                    
            # Check for potential timer values (steadily decreasing)
            if np.all(diffs <= 0) and changes > self.sample_count / 3:
                self.analysis_results["potential_timer"].append({
                    "address": addr,
                    "start": int(values_array[0]),
                    "end": int(values_array[-1]),
                    "changes": int(changes)
                })
        
        print("Analysis completed successfully")
        return True
    
    def suggest_mappings(self):
        """Suggest potential memory mappings based on analysis results."""
        if not self.analysis_results:
            print("No analysis results available. Run analyze first.")
            return {}
        
        suggestions = {
            "game": self.game_name,
            "architecture": self.architecture,
            "version": "1.0.0",
            "description": f"Auto-generated mappings for {self.game_name}",
            "mappings": []
        }
        
        # Helper function to add a mapping
        def add_mapping(name, addr, type_name, description, category, min_val=None, max_val=None, player=None):
            mapping = {
                "name": name,
                "address": addr,
                "type": type_name,
                "description": description,
                "category": category
            }
            
            if min_val is not None:
                mapping["min_value"] = min_val
            if max_val is not None:
                mapping["max_value"] = max_val
            if player is not None:
                mapping["player_index"] = player
                
            suggestions["mappings"].append(mapping)
        
        # Suggest health mappings
        for i, health in enumerate(self.analysis_results["potential_health"][:2]):  # Limit to top 2
            player = i + 1  # Player 1, Player 2
            add_mapping(
                f"p{player}_health",
                health["address"],
                "int8",
                f"Player {player} health",
                "player_state",
                0,
                health["max"],
                player
            )
        
        # Suggest position mappings
        for i, pos in enumerate(self.analysis_results["potential_position"][:2]):  # Limit to top 2
            player = i + 1  # Player 1, Player 2
            add_mapping(
                f"p{player}_pos_x",
                pos["x_address"],
                "int16",
                f"Player {player} X position",
                "position",
                None,
                None,
                player
            )
            add_mapping(
                f"p{player}_pos_y",
                pos["y_address"],
                "int16",
                f"Player {player} Y position",
                "position",
                None,
                None,
                player
            )
        
        # Suggest timer mappings
        if self.analysis_results["potential_timer"]:
            timer = self.analysis_results["potential_timer"][0]  # Take the first one
            add_mapping(
                "timer",
                timer["address"],
                "int16",
                "Round timer",
                "game_state",
                0,
                timer["start"]
            )
        
        # Suggest binary values as flags
        for addr, data in list(self.analysis_results["binary_values"].items())[:5]:  # Limit to top 5
            add_mapping(
                f"flag_{addr[-4:]}",  # Use last 4 chars of address as name
                addr,
                "bool",
                f"State flag at {addr}",
                "game_state"
            )
        
        # Suggest counter values
        for addr, data in list(self.analysis_results["counter_values"].items())[:3]:  # Limit to top 3
            add_mapping(
                f"counter_{addr[-4:]}",  # Use last 4 chars of address as name
                addr,
                "int8",
                f"Counter at {addr}",
                "game_state"
            )
        
        print(f"Generated {len(suggestions['mappings'])} mapping suggestions")
        return suggestions
    
    def visualize(self, output_dir=None):
        """Generate visualizations of the analyzed data."""
        if not self.memory_data or not self.analysis_results:
            print("No data to visualize. Run analyze first.")
            return False
            
        if output_dir:
            output_path = Path(output_dir)
            output_path.mkdir(exist_ok=True, parents=True)
        else:
            output_path = Path("analysis", self.game_name)
            output_path.mkdir(exist_ok=True, parents=True)
            
        # Plot potential health values
        if self.analysis_results["potential_health"]:
            plt.figure(figsize=(12, 6))
            for h in self.analysis_results["potential_health"][:3]:  # Plot top 3
                addr = h["address"]
                values = self.memory_data[addr]
                plt.plot(values, label=f"Address {addr}")
            
            plt.title("Potential Health Values")
            plt.xlabel("Sample Index")
            plt.ylabel("Value")
            plt.legend()
            plt.grid(True)
            plt.savefig(output_path / "health_values.png")
            plt.close()
            
        # Plot potential timer values
        if self.analysis_results["potential_timer"]:
            plt.figure(figsize=(12, 6))
            for t in self.analysis_results["potential_timer"][:3]:  # Plot top 3
                addr = t["address"]
                values = self.memory_data[addr]
                plt.plot(values, label=f"Address {addr}")
            
            plt.title("Potential Timer Values")
            plt.xlabel("Sample Index")
            plt.ylabel("Value")
            plt.legend()
            plt.grid(True)
            plt.savefig(output_path / "timer_values.png")
            plt.close()
            
        # Plot potential position values (X and Y pairs)
        if self.analysis_results["potential_position"]:
            plt.figure(figsize=(12, 6))
            for p in self.analysis_results["potential_position"][:2]:  # Plot top 2
                x_addr = p["x_address"]
                y_addr = p["y_address"]
                
                x_values = self.memory_data[x_addr]
                y_values = self.memory_data[y_addr]
                
                # Plot X and Y values over time
                plt.subplot(1, 2, 1)
                plt.plot(x_values, label=f"X: {x_addr}")
                plt.plot(y_values, label=f"Y: {y_addr}")
                plt.title("Position Values Over Time")
                plt.xlabel("Sample Index")
                plt.ylabel("Value")
                plt.legend()
                plt.grid(True)
                
                # Plot X vs Y (2D position)
                plt.subplot(1, 2, 2)
                plt.scatter(x_values, y_values, alpha=0.7, label=f"{x_addr} vs {y_addr}")
                plt.title("X vs Y Position")
                plt.xlabel("X Position")
                plt.ylabel("Y Position")
                plt.legend()
                plt.grid(True)
                
            plt.tight_layout()
            plt.savefig(output_path / "position_values.png")
            plt.close()
        
        print(f"Visualizations saved to {output_path}")
        return True
        
    def save_results(self, output_file=None):
        """Save analysis results to a JSON file."""
        if not self.analysis_results:
            print("No analysis results to save.")
            return False
            
        if not output_file:
            output_dir = Path("analysis")
            output_dir.mkdir(exist_ok=True)
            output_file = output_dir / f"{self.game_name}_analysis.json"
        else:
            output_file = Path(output_file)
            output_file.parent.mkdir(exist_ok=True, parents=True)
            
        try:
            with open(output_file, 'w') as f:
                json.dump(self.analysis_results, f, indent=2)
                
            print(f"Analysis results saved to {output_file}")
            return True
        except Exception as e:
            print(f"Error saving analysis results: {e}")
            return False
            
    def save_suggestions(self, output_file=None):
        """Save mapping suggestions to a JSON file."""
        suggestions = self.suggest_mappings()
        
        if not suggestions or not suggestions.get("mappings"):
            print("No mapping suggestions to save.")
            return False
            
        if not output_file:
            output_dir = Path("mappings")
            output_dir.mkdir(exist_ok=True)
            output_file = output_dir / f"{self.game_name}_suggested.json"
        else:
            output_file = Path(output_file)
            output_file.parent.mkdir(exist_ok=True, parents=True)
            
        try:
            with open(output_file, 'w') as f:
                json.dump(suggestions, f, indent=2)
                
            print(f"Mapping suggestions saved to {output_file}")
            return True
        except Exception as e:
            print(f"Error saving mapping suggestions: {e}")
            return False

def main():
    parser = argparse.ArgumentParser(description="Analyze memory samples from FBNeo games")
    parser.add_argument("sample_file", help="Path to the sample JSON file")
    parser.add_argument("--output", help="Output file for analysis results")
    parser.add_argument("--visualize", action="store_true", help="Generate visualizations")
    parser.add_argument("--suggest", action="store_true", help="Generate mapping suggestions")
    parser.add_argument("--visualization-dir", help="Directory to save visualizations")
    parser.add_argument("--suggestions-output", help="Output file for mapping suggestions")
    
    args = parser.parse_args()
    
    analyzer = MemoryAnalyzer(args.sample_file)
    
    if not analyzer.load_samples():
        sys.exit(1)
        
    if not analyzer.preprocess_data():
        sys.exit(1)
        
    if not analyzer.analyze():
        sys.exit(1)
        
    analyzer.save_results(args.output)
    
    if args.visualize:
        analyzer.visualize(args.visualization_dir)
        
    if args.suggest:
        analyzer.save_suggestions(args.suggestions_output)

if __name__ == "__main__":
    main() 