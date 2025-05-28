#!/usr/bin/env python3
import argparse
import json
import os
import time
import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict
from datetime import datetime
import subprocess
import tempfile
import struct
from pathlib import Path
import re
import signal
import sys


class MemorySnapshot:
    """Represents a snapshot of memory at a specific point in time."""
    def __init__(self, memory_data, timestamp=None, label=None):
        self.memory_data = memory_data
        self.timestamp = timestamp or time.time()
        self.label = label
    
    @property
    def size(self):
        return len(self.memory_data)


class MemoryChangeAnalyzer:
    """Analyzes changes in memory between snapshots."""
    def __init__(self):
        self.snapshots = []
        self.labels = set()
        self.changes_by_address = defaultdict(list)
        self.correlated_addresses = defaultdict(set)
    
    def add_snapshot(self, snapshot):
        """Add a new memory snapshot."""
        self.snapshots.append(snapshot)
        if snapshot.label:
            self.labels.add(snapshot.label)
    
    def find_differences(self, snapshot1, snapshot2):
        """Find bytes that differ between two snapshots."""
        min_size = min(snapshot1.size, snapshot2.size)
        differences = []
        
        for i in range(min_size):
            if snapshot1.memory_data[i] != snapshot2.memory_data[i]:
                differences.append((i, snapshot1.memory_data[i], snapshot2.memory_data[i]))
        
        return differences
    
    def analyze_all_snapshots(self):
        """Analyze all collected snapshots to find changing memory locations."""
        if len(self.snapshots) < 2:
            print("Need at least 2 snapshots to analyze")
            return []
        
        # Clear previous analysis
        self.changes_by_address.clear()
        self.correlated_addresses.clear()
        
        # Find differences between each consecutive pair of snapshots
        for i in range(1, len(self.snapshots)):
            prev = self.snapshots[i-1]
            curr = self.snapshots[i]
            
            differences = self.find_differences(prev, curr)
            
            for addr, old_val, new_val in differences:
                self.changes_by_address[addr].append((prev.timestamp, curr.timestamp, 
                                                     old_val, new_val, 
                                                     prev.label, curr.label))
        
        # Find addresses that change together
        addresses = list(self.changes_by_address.keys())
        for i in range(len(addresses)):
            addr1 = addresses[i]
            changes1 = self.changes_by_address[addr1]
            timestamps1 = [(start, end) for start, end, _, _, _, _ in changes1]
            
            for j in range(i+1, len(addresses)):
                addr2 = addresses[j]
                changes2 = self.changes_by_address[addr2]
                timestamps2 = [(start, end) for start, end, _, _, _, _ in changes2]
                
                # Check if changes happen at the same time
                matching_times = sum(1 for t1 in timestamps1 if t1 in timestamps2)
                if matching_times > len(timestamps1) * 0.7:  # 70% correlation threshold
                    self.correlated_addresses[addr1].add(addr2)
                    self.correlated_addresses[addr2].add(addr1)
        
        return list(self.changes_by_address.keys())
    
    def get_address_statistics(self, address):
        """Get statistics about an address that changed."""
        if address not in self.changes_by_address:
            return None
        
        changes = self.changes_by_address[address]
        values = [new_val for _, _, _, new_val, _, _ in changes]
        
        return {
            "address": address,
            "hex_address": f"0x{address:X}",
            "change_count": len(changes),
            "min_value": min(values),
            "max_value": max(values),
            "unique_values": len(set(values)),
            "correlated_with": len(self.correlated_addresses.get(address, set())),
            "changes": changes
        }
    
    def get_top_changing_addresses(self, limit=50):
        """Get addresses that changed the most frequently."""
        addresses = list(self.changes_by_address.keys())
        addresses.sort(key=lambda addr: len(self.changes_by_address[addr]), reverse=True)
        return addresses[:limit]
    
    def guess_data_types(self, address, check_multi_byte=True):
        """Attempt to guess the data type based on observed values."""
        if address not in self.changes_by_address:
            return []
        
        # Get all the values this address changed to
        values = [new_val for _, _, _, new_val, _, _ in self.changes_by_address[address]]
        
        guesses = []
        
        # Check if values are within byte range
        if all(0 <= v <= 255 for v in values):
            guesses.append(("byte", 1))
        
        # Check for multi-byte values if requested
        if check_multi_byte and address in self.correlated_addresses:
            # Look for consecutive addresses
            for size, type_name in [(2, "word"), (4, "dword")]:
                consecutive = True
                for i in range(1, size):
                    if address + i not in self.correlated_addresses[address]:
                        consecutive = False
                        break
                
                if consecutive:
                    guesses.append((type_name, size))
        
        # Check if values look like positions (usually change gradually)
        differences = [abs(values[i] - values[i-1]) for i in range(1, len(values))]
        if differences and sum(differences) / len(differences) < 10:
            guesses.append(("position", 1))
        
        # Check if values look like health (usually decrease)
        decreases = sum(1 for i in range(1, len(values)) if values[i] < values[i-1])
        if values and decreases > len(values) * 0.7:
            guesses.append(("health", 1))
        
        # Check if values look like binary flags
        if set(values) == {0, 1} or set(values) <= {0, 1, 2, 4, 8, 16, 32, 64, 128}:
            guesses.append(("flag", 1))
        
        return guesses
    
    def generate_mapping_entry(self, address, label=None):
        """Generate a memory mapping entry for the given address."""
        if address not in self.changes_by_address:
            return None
        
        stats = self.get_address_statistics(address)
        guesses = self.guess_data_types(address)
        
        if not guesses:
            return None
        
        # Use the most likely guess for this entry
        data_type, size = guesses[0]
        
        # Create a mapping name based on the label or address
        if label:
            name = f"{label.lower().replace(' ', '_')}"
        else:
            types = {"position": "pos", "health": "health", "flag": "flag"}
            type_str = types.get(data_type, "value")
            name = f"auto_{type_str}_{address:x}"
        
        entry = {
            "name": name,
            "address": f"0x{address:X}",
            "type": "byte" if size == 1 else ("word" if size == 2 else "dword"),
            "description": f"Auto-detected {data_type} value"
        }
        
        # Add additional fields based on the data type
        if data_type == "health":
            entry["min"] = stats["min_value"]
            entry["max"] = stats["max_value"]
        
        return entry


class MemoryDiscoverer:
    """Tool for discovering memory mappings by monitoring a running game."""
    def __init__(self, game_name, fbneo_path=None, dump_dir=None):
        self.game_name = game_name
        self.fbneo_path = fbneo_path or self._find_fbneo_executable()
        self.dump_dir = dump_dir or tempfile.mkdtemp()
        self.analyzer = MemoryChangeAnalyzer()
        self.snapshots = []
        self.process = None
        
    def _find_fbneo_executable(self):
        """Find the FBNeo executable in common locations."""
        common_paths = [
            "fbneo",
            "./fbneo",
            "../fbneo",
            os.path.expanduser("~/fbneo"),
            "/usr/local/bin/fbneo",
            "C:/Program Files/fbneo/fbneo.exe",
        ]
        
        for path in common_paths:
            if os.path.isfile(path) and os.access(path, os.X_OK):
                return path
        
        return "fbneo"  # Default to just the name and hope it's in PATH
    
    def start_game(self):
        """Start the game with memory dump capabilities."""
        try:
            # Create command for FBNeo, using a hypothetical memory dump flag
            cmd = [
                self.fbneo_path,
                f"--game={self.game_name}",
                "--dump-memory"  # Hypothetical flag, actual implementation would depend on FBNeo
            ]
            
            print(f"Starting game: {self.game_name}")
            self.process = subprocess.Popen(cmd)
            
            # Wait a bit for the game to start
            time.sleep(3)
            
            return True
        except Exception as e:
            print(f"Error starting game: {e}")
            return False
    
    def stop_game(self):
        """Stop the running game."""
        if self.process:
            self.process.terminate()
            self.process = None
    
    def take_snapshot(self, label=None):
        """Take a snapshot of the current memory state."""
        try:
            # This is a placeholder for the actual implementation
            # In a real scenario, we would read memory from the running game
            
            # For demonstration purposes, we'll simulate reading memory
            # from a hypothetical dump file that FBNeo would create
            
            # Simulate a dump file path
            dump_file = os.path.join(self.dump_dir, f"memory_{int(time.time())}.bin")
            
            # In reality, we would need to trigger FBNeo to create a dump
            # For now, just create a simulated memory dump
            with open(dump_file, 'wb') as f:
                # Create a simulated memory dump with random data
                mem_size = 1024 * 1024  # 1MB of memory
                memory_data = bytearray(os.urandom(mem_size))
                f.write(memory_data)
            
            snapshot = MemorySnapshot(memory_data, time.time(), label)
            self.snapshots.append(snapshot)
            self.analyzer.add_snapshot(snapshot)
            
            print(f"Snapshot taken{': ' + label if label else ''}")
            return True
        except Exception as e:
            print(f"Error taking snapshot: {e}")
            return False
    
    def load_snapshot_from_file(self, file_path, label=None):
        """Load a memory snapshot from a file."""
        try:
            with open(file_path, 'rb') as f:
                memory_data = f.read()
            
            snapshot = MemorySnapshot(memory_data, time.time(), label)
            self.snapshots.append(snapshot)
            self.analyzer.add_snapshot(snapshot)
            
            print(f"Snapshot loaded from {file_path}{': ' + label if label else ''}")
            return True
        except Exception as e:
            print(f"Error loading snapshot: {e}")
            return False
    
    def analyze_snapshots(self):
        """Analyze the collected snapshots to find memory mappings."""
        if len(self.snapshots) < 2:
            print("Need at least 2 snapshots to analyze. Take more snapshots.")
            return []
        
        print(f"Analyzing {len(self.snapshots)} snapshots...")
        
        # Find changing addresses
        changing_addresses = self.analyzer.analyze_all_snapshots()
        print(f"Found {len(changing_addresses)} changing memory locations")
        
        # Get the top changing addresses
        top_addresses = self.analyzer.get_top_changing_addresses(100)
        
        # Generate mapping entries for the top addresses
        mapping_entries = []
        for addr in top_addresses:
            entry = self.analyzer.generate_mapping_entry(addr)
            if entry:
                mapping_entries.append(entry)
        
        print(f"Generated {len(mapping_entries)} potential mapping entries")
        return mapping_entries
    
    def generate_mapping_file(self, output_file=None):
        """Generate a mapping file from the analysis results."""
        if not output_file:
            output_file = f"mappings/{self.game_name}_auto.json"
        
        # Make sure the output directory exists
        os.makedirs(os.path.dirname(output_file), exist_ok=True)
        
        # Generate mapping entries
        mapping_entries = self.analyze_snapshots()
        
        if not mapping_entries:
            print("No mappings to save. Take more snapshots with different game states.")
            return False
        
        # Create mapping file structure
        mapping_data = {
            "game": self.game_name,
            "architecture": "Unknown",  # Would need to be specified by user
            "version": "1.0.0",
            "description": f"Auto-generated memory mapping for {self.game_name}",
            "author": "Memory Discoverer Tool",
            "created": datetime.now().isoformat(),
            "mappings": mapping_entries
        }
        
        # Save the mapping file
        with open(output_file, 'w') as f:
            json.dump(mapping_data, f, indent=2)
        
        print(f"Mapping file saved to {output_file}")
        return True
    
    def plot_value_changes(self, addresses, output_file=None):
        """Plot how values at different addresses changed over time."""
        if not addresses:
            print("No addresses specified for plotting")
            return
        
        # Convert addresses to integers if they're hex strings
        addrs = []
        for addr in addresses:
            if isinstance(addr, str) and addr.startswith("0x"):
                addrs.append(int(addr, 16))
            else:
                addrs.append(addr)
        
        plt.figure(figsize=(12, 8))
        
        for i, addr in enumerate(addrs):
            if addr not in self.analyzer.changes_by_address:
                print(f"Address {addr} not found in changes")
                continue
            
            changes = self.analyzer.changes_by_address[addr]
            times = [end for _, end, _, _, _, _ in changes]
            values = [new_val for _, _, _, new_val, _, _ in changes]
            
            # Normalize times to start from 0
            if times:
                start_time = min(times)
                times = [t - start_time for t in times]
            
            plt.subplot(len(addrs), 1, i+1)
            plt.plot(times, values, 'o-')
            plt.title(f"Address 0x{addr:X}")
            plt.ylabel("Value")
            plt.grid(True)
        
        plt.tight_layout()
        
        if output_file:
            plt.savefig(output_file)
            print(f"Plot saved to {output_file}")
        else:
            plt.show()
    
    def interactive_session(self):
        """Run an interactive session for discovering memory mappings."""
        print(f"Starting interactive memory discovery for {self.game_name}")
        print("Commands:")
        print("  start      - Start the game")
        print("  snapshot   - Take a memory snapshot")
        print("  label NAME - Take a labeled snapshot")
        print("  analyze    - Analyze snapshots")
        print("  save [FILE] - Save mapping file")
        print("  plot ADDR1 [ADDR2 ...] - Plot values for addresses")
        print("  quit       - Exit")
        
        try:
            while True:
                cmd = input("> ").strip().split()
                if not cmd:
                    continue
                
                if cmd[0] == "start":
                    self.start_game()
                
                elif cmd[0] == "snapshot":
                    self.take_snapshot()
                
                elif cmd[0] == "label" and len(cmd) > 1:
                    self.take_snapshot(cmd[1])
                
                elif cmd[0] == "analyze":
                    self.analyze_snapshots()
                
                elif cmd[0] == "save":
                    output_file = cmd[1] if len(cmd) > 1 else None
                    self.generate_mapping_file(output_file)
                
                elif cmd[0] == "plot" and len(cmd) > 1:
                    addresses = []
                    for addr_str in cmd[1:]:
                        if addr_str.startswith("0x"):
                            addresses.append(int(addr_str, 16))
                        else:
                            try:
                                addresses.append(int(addr_str))
                            except ValueError:
                                print(f"Invalid address: {addr_str}")
                    
                    if addresses:
                        self.plot_value_changes(addresses)
                
                elif cmd[0] == "quit" or cmd[0] == "exit":
                    break
                
                else:
                    print(f"Unknown command: {cmd[0]}")
        
        except KeyboardInterrupt:
            print("\nExiting...")
        
        finally:
            self.stop_game()
    
    def batch_mode(self, snapshot_labels, output_file=None):
        """Run in batch mode with predefined snapshot labels."""
        success = self.start_game()
        if not success:
            return False
        
        try:
            for label in snapshot_labels:
                print(f"Take a snapshot for: {label}")
                print("Press Enter when ready...")
                input()
                self.take_snapshot(label)
            
            self.generate_mapping_file(output_file)
            return True
            
        except KeyboardInterrupt:
            print("\nBatch mode interrupted")
            return False
            
        finally:
            self.stop_game()


def main():
    parser = argparse.ArgumentParser(description="FBNeo Memory Mapping Discovery Tool")
    parser.add_argument("game", help="ROM name of the game to analyze")
    parser.add_argument("--fbneo", help="Path to FBNeo executable")
    parser.add_argument("--output", help="Output mapping file path")
    parser.add_argument("--batch", nargs="+", help="Run in batch mode with these snapshot labels")
    parser.add_argument("--dump-dir", help="Directory for memory dumps")
    
    args = parser.parse_args()
    
    discoverer = MemoryDiscoverer(args.game, args.fbneo, args.dump_dir)
    
    if args.batch:
        discoverer.batch_mode(args.batch, args.output)
    else:
        discoverer.interactive_session()


if __name__ == "__main__":
    main() 