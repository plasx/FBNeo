#!/usr/bin/env python3
"""
Memory Collection Script for FBNeo Games

This script automates the collection of memory samples from FBNeo games
without requiring the interactive GUI from the mapping utility.
"""

import argparse
import json
import os
import subprocess
import sys
import time
from datetime import datetime
import platform
from pathlib import Path

def find_fbneo_executable():
    """Find the FBNeo executable on different platforms."""
    common_paths = {
        "Windows": [
            "C:\\Program Files\\FBNeo\\fbneo.exe",
            "C:\\Program Files (x86)\\FBNeo\\fbneo.exe",
            ".\\fbneo.exe",
        ],
        "Darwin": [  # macOS
            "/Applications/FBNeo.app/Contents/MacOS/FBNeo",
            "./FBNeo.app/Contents/MacOS/FBNeo",
            "~/Applications/FBNeo.app/Contents/MacOS/FBNeo",
        ],
        "Linux": [
            "/usr/local/bin/fbneo",
            "/usr/bin/fbneo",
            "./fbneo",
            "~/.local/bin/fbneo",
        ]
    }

    system = platform.system()
    if system in common_paths:
        for path in common_paths[system]:
            expanded_path = os.path.expanduser(path)
            if os.path.exists(expanded_path):
                return expanded_path
    
    return None

def construct_fbneo_command(executable, game, rom_path=None, args=None):
    """Construct FBNeo command with appropriate arguments."""
    command = [executable]
    
    if game:
        command.append(f"--rom={game}")
    
    if rom_path:
        command.append(f"--rom-path={rom_path}")
    
    # Add common arguments for debug/memory access
    command.append("--debug-cpu")
    command.append("--create-memory-dump")
    
    # Add any additional arguments
    if args:
        command.extend(args.split())
    
    return command

def memory_dump_to_dict(dump_file):
    """Convert a memory dump file to a dictionary of address: value."""
    memory_dict = {}
    
    try:
        with open(dump_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    parts = line.split(':', 1)
                    if len(parts) == 2:
                        addr = parts[0].strip()
                        value = parts[1].strip()
                        memory_dict[addr] = value
    except Exception as e:
        print(f"Error reading memory dump: {e}")
    
    return memory_dict

def collect_samples(game, architecture, num_samples=50, interval=1.0, output_file=None, rom_path=None, fbneo_args=None):
    """Collect memory samples from a running FBNeo game."""
    # Find FBNeo executable
    fbneo_exe = find_fbneo_executable()
    if not fbneo_exe:
        print("Error: FBNeo executable not found. Please specify the path.")
        return False
    
    # Construct command
    command = construct_fbneo_command(fbneo_exe, game, rom_path, fbneo_args)
    
    # Create output directory if it doesn't exist
    samples_dir = Path("samples")
    samples_dir.mkdir(exist_ok=True)
    
    # Determine output file
    if not output_file:
        output_file = samples_dir / f"{game}_samples.json"
    else:
        output_file = Path(output_file)
    
    print(f"Starting FBNeo with game: {game}")
    print(f"Will collect {num_samples} samples every {interval} seconds")
    print(f"Output will be saved to: {output_file}")
    print("Press Ctrl+C to stop collection early")
    
    # Launch FBNeo
    try:
        process = subprocess.Popen(command)
        
        # Give FBNeo time to start up
        time.sleep(5)
        
        samples = []
        dump_file = Path("memory.dump")  # Default dump file
        
        print("\nBeginning sample collection...")
        print("Play the game to generate meaningful samples")
        
        for i in range(num_samples):
            try:
                # Wait for the interval
                time.sleep(interval)
                
                # FBNeo creates memory dumps automatically or we can trigger one
                # Here we assume it's already being created
                
                if dump_file.exists():
                    memory_data = memory_dump_to_dict(dump_file)
                    timestamp = datetime.now().isoformat()
                    
                    sample = {
                        "timestamp": timestamp,
                        "frame_number": i,
                        "memory": memory_data
                    }
                    
                    samples.append(sample)
                    print(f"Collected sample {i+1}/{num_samples} ({len(memory_data)} memory locations)")
                else:
                    print(f"Warning: Memory dump file not found at: {dump_file}")
            
            except KeyboardInterrupt:
                print("\nSample collection interrupted")
                break
        
        # Save collected samples
        sample_data = {
            "game": game,
            "architecture": architecture,
            "date_collected": datetime.now().isoformat(),
            "num_samples": len(samples),
            "samples": samples
        }
        
        with open(output_file, 'w') as f:
            json.dump(sample_data, f, indent=2)
        
        print(f"\nSaved {len(samples)} samples to {output_file}")
        
        # Ask user to close FBNeo
        input("\nPress Enter after closing FBNeo...")
        
        return True
    
    except Exception as e:
        print(f"Error during sample collection: {e}")
        return False
    finally:
        # Try to terminate FBNeo process if still running
        try:
            process.terminate()
        except:
            pass

def main():
    parser = argparse.ArgumentParser(description="Collect memory samples from FBNeo games")
    parser.add_argument("--game", required=True, help="ROM name of the game to run")
    parser.add_argument("--arch", required=True, help="Architecture of the game (CPS1, CPS2, CPS3, NEOGEO, etc.)")
    parser.add_argument("--samples", type=int, default=50, help="Number of samples to collect")
    parser.add_argument("--interval", type=float, default=1.0, help="Interval between samples in seconds")
    parser.add_argument("--output", help="Output JSON file path")
    parser.add_argument("--rom-path", help="Path to ROM directory")
    parser.add_argument("--fbneo-args", help="Additional arguments to pass to FBNeo")
    
    args = parser.parse_args()
    
    collect_samples(
        args.game, 
        args.arch,
        num_samples=args.samples,
        interval=args.interval,
        output_file=args.output,
        rom_path=args.rom_path,
        fbneo_args=args.fbneo_args
    )

if __name__ == "__main__":
    main() 