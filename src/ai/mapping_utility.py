#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import sys
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import re
import platform

class MappingUtility:
    def __init__(self, game_name, architecture=None, mapping_file=None):
        self.game_name = game_name
        self.architecture = architecture
        self.fbneo_executable = self.find_fbneo_executable()
        
        self.mappings = []
        if mapping_file:
            self.load_mapping(mapping_file)
        elif game_name:
            # Try to load existing mapping for this game
            self.load_mapping(f"mappings/{game_name}.json")
        
        self.samples = []
    
    def find_fbneo_executable(self):
        """Find the FBNeo executable in common paths."""
        common_paths = [
            ".",  # Current directory
            "./fbneo",  # Common subdirectory
            os.path.expanduser("~/.fbneo"),  # User directory
            os.path.expanduser("~/Applications/fbneo"),  # Mac Applications
            "C:/Program Files/fbneo",  # Windows Program Files
            "C:/fbneo",  # Windows root
        ]
        
        executable_names = ["fbneo"]
        if platform.system() == "Windows":
            executable_names = ["fbneo.exe"]
        elif platform.system() == "Darwin":
            executable_names.append("FinalBurn Neo")
        
        for path in common_paths:
            for name in executable_names:
                exe_path = os.path.join(path, name)
                if os.path.isfile(exe_path) and os.access(exe_path, os.X_OK):
                    return exe_path
        
        # If not found, return a default and let the user specify it later
        return "fbneo"
    
    def load_mapping(self, mapping_file):
        """Load a memory mapping from a JSON file."""
        try:
            # Try direct path
            if os.path.isfile(mapping_file):
                with open(mapping_file, 'r') as f:
                    data = json.load(f)
            # Try in mappings directory
            elif os.path.isfile(f"mappings/{mapping_file}"):
                with open(f"mappings/{mapping_file}", 'r') as f:
                    data = json.load(f)
            # Try with .json extension
            elif os.path.isfile(f"{mapping_file}.json"):
                with open(f"{mapping_file}.json", 'r') as f:
                    data = json.load(f)
            # Try in mappings directory with .json extension
            elif os.path.isfile(f"mappings/{mapping_file}.json"):
                with open(f"mappings/{mapping_file}.json", 'r') as f:
                    data = json.load(f)
            else:
                print(f"Warning: Mapping file {mapping_file} not found.")
                return False
                
            self.game_name = data.get("game", self.game_name)
            self.architecture = data.get("architecture", self.architecture)
            self.mappings = data.get("mappings", [])
            return True
            
        except Exception as e:
            print(f"Error loading mapping file: {e}")
            return False
    
    def save_mapping(self, output_file=None):
        """Save the current mappings to a JSON file."""
        if not output_file:
            output_file = f"mappings/{self.game_name}.json"
        
        # Create directory if it doesn't exist
        os.makedirs(os.path.dirname(output_file), exist_ok=True)
        
        data = {
            "game": self.game_name,
            "architecture": self.architecture,
            "mappings": self.mappings
        }
        
        with open(output_file, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"Mapping saved to {output_file}")
        return True
    
    def collect_samples(self, num_samples=100, interval=0.5):
        """Launch FBNeo and collect memory dumps."""
        try:
            # Clear previous samples
            self.samples = []
            
            # Command to launch FBNeo with the specified game
            cmd = [
                self.fbneo_executable,
                f"--game={self.game_name}",
                "--memory-dump"  # Assuming FBNeo has this feature
            ]
            
            print(f"Launching {self.game_name} in FBNeo...")
            print("Press ESC to stop collection, SPACE to capture a sample.")
            
            # This is a simulated approach - actual implementation would depend on FBNeo's capabilities
            proc = subprocess.Popen(cmd)
            
            samples_collected = 0
            while samples_collected < num_samples:
                # Wait for user to press key to capture sample
                # This would be replaced with actual implementation
                time.sleep(interval)
                
                # Simulate capturing a memory dump
                # In reality, this would read from a memory dump file or API
                sample = self._simulate_memory_dump()
                self.samples.append(sample)
                
                samples_collected += 1
                print(f"Sample {samples_collected}/{num_samples} collected")
            
            proc.terminate()
            print(f"Collected {len(self.samples)} memory samples")
            return True
            
        except Exception as e:
            print(f"Error collecting samples: {e}")
            return False
    
    def _simulate_memory_dump(self):
        """Simulate a memory dump for testing purposes."""
        # In a real implementation, this would read actual memory values
        # For now, we'll generate random data
        return {
            "timestamp": time.time(),
            "memory": {
                # Simulate some memory addresses with changing values
                "0xFF83C6": np.random.randint(0, 100),  # Health
                "0xFF83E2": np.random.randint(0, 400),  # X position
                "0xFF83E6": np.random.randint(0, 200),  # Y position
            }
        }
    
    def add_mapping(self, name, address, data_type, description, 
                   scale=1.0, offset=0, min_value=None, max_value=None):
        """Add a new memory mapping."""
        # Convert address to standardized format
        if isinstance(address, int):
            address = f"0x{address:X}"
        elif isinstance(address, str) and not address.startswith("0x"):
            try:
                address_int = int(address, 16 if "0x" in address.lower() else 10)
                address = f"0x{address_int:X}"
            except ValueError:
                print(f"Invalid address format: {address}")
                return False
        
        # Validate data_type
        valid_types = ["byte", "word", "dword", "float"]
        if data_type.lower() not in valid_types:
            print(f"Invalid data type. Must be one of: {', '.join(valid_types)}")
            return False
        
        # Check for duplicate name
        for mapping in self.mappings:
            if mapping["name"] == name:
                print(f"Warning: Mapping with name '{name}' already exists. Updating.")
                mapping.update({
                    "address": address,
                    "type": data_type.lower(),
                    "description": description,
                    "scale": float(scale),
                    "offset": float(offset)
                })
                if min_value is not None:
                    mapping["min"] = float(min_value)
                if max_value is not None:
                    mapping["max"] = float(max_value)
                return True
        
        # Create new mapping
        new_mapping = {
            "name": name,
            "address": address,
            "type": data_type.lower(),
            "description": description,
            "scale": float(scale),
            "offset": float(offset)
        }
        
        if min_value is not None:
            new_mapping["min"] = float(min_value)
        if max_value is not None:
            new_mapping["max"] = float(max_value)
        
        self.mappings.append(new_mapping)
        print(f"Added mapping: {name} -> {address}")
        return True
    
    def remove_mapping(self, name):
        """Remove a mapping by name."""
        for i, mapping in enumerate(self.mappings):
            if mapping["name"] == name:
                del self.mappings[i]
                print(f"Removed mapping: {name}")
                return True
        
        print(f"Mapping '{name}' not found.")
        return False
    
    def edit_mapping(self, name, **kwargs):
        """Edit an existing mapping."""
        for mapping in self.mappings:
            if mapping["name"] == name:
                # Update specified fields
                for key, value in kwargs.items():
                    if key in ["scale", "offset", "min", "max"] and value is not None:
                        mapping[key] = float(value)
                    elif key == "address" and value is not None:
                        # Convert address to standardized format
                        if isinstance(value, int):
                            mapping[key] = f"0x{value:X}"
                        elif isinstance(value, str) and not value.startswith("0x"):
                            try:
                                addr_int = int(value, 16 if "0x" in value.lower() else 10)
                                mapping[key] = f"0x{addr_int:X}"
                            except ValueError:
                                print(f"Invalid address format: {value}")
                                return False
                        else:
                            mapping[key] = value
                    elif value is not None:
                        mapping[key] = value
                
                print(f"Updated mapping: {name}")
                return True
        
        print(f"Mapping '{name}' not found.")
        return False
    
    def analyze_samples(self):
        """Analyze collected samples for patterns."""
        if not self.samples:
            print("No samples to analyze.")
            return
        
        # Extract all memory addresses from samples
        addresses = set()
        for sample in self.samples:
            addresses.update(sample["memory"].keys())
        
        # Analyze each address for variation
        address_stats = {}
        for addr in addresses:
            values = [sample["memory"].get(addr, 0) for sample in self.samples]
            address_stats[addr] = {
                "min": min(values),
                "max": max(values),
                "mean": np.mean(values),
                "std": np.std(values),
                "unique": len(set(values)),
                "values": values
            }
        
        # Sort addresses by variation (standard deviation)
        sorted_addresses = sorted(
            addresses, 
            key=lambda x: address_stats[x]["std"], 
            reverse=True
        )
        
        # Display the most variable addresses
        print("\nMost variable memory addresses:")
        for i, addr in enumerate(sorted_addresses[:10]):
            stats = address_stats[addr]
            print(f"{addr}: min={stats['min']}, max={stats['max']}, "
                  f"mean={stats['mean']:.2f}, std={stats['std']:.2f}, "
                  f"unique values={stats['unique']}")
        
        # Plot the top 5 most variable addresses
        plt.figure(figsize=(12, 8))
        for i, addr in enumerate(sorted_addresses[:5]):
            values = address_stats[addr]["values"]
            plt.subplot(5, 1, i+1)
            plt.plot(values, label=addr)
            plt.title(f"Address {addr}")
            plt.ylabel("Value")
            plt.grid(True)
        
        plt.tight_layout()
        plt.savefig(f"mappings/{self.game_name}_analysis.png")
        plt.show()
        
        return address_stats
    
    def interactive_editor(self):
        """Launch an interactive GUI for editing mappings."""
        root = tk.Tk()
        root.title(f"Memory Mapping Editor - {self.game_name}")
        root.geometry("800x600")
        
        # Frame for game info
        frame_info = ttk.Frame(root, padding=10)
        frame_info.pack(fill=tk.X)
        
        ttk.Label(frame_info, text="Game:").grid(row=0, column=0, sticky=tk.W)
        game_var = tk.StringVar(value=self.game_name)
        ttk.Entry(frame_info, textvariable=game_var).grid(row=0, column=1, sticky=tk.W+tk.E)
        
        ttk.Label(frame_info, text="Architecture:").grid(row=1, column=0, sticky=tk.W)
        arch_var = tk.StringVar(value=self.architecture)
        ttk.Entry(frame_info, textvariable=arch_var).grid(row=1, column=1, sticky=tk.W+tk.E)
        
        # Button to update game info
        def update_game_info():
            self.game_name = game_var.get()
            self.architecture = arch_var.get()
            root.title(f"Memory Mapping Editor - {self.game_name}")
            messagebox.showinfo("Info", "Game info updated")
        
        ttk.Button(frame_info, text="Update", command=update_game_info).grid(row=0, column=2, rowspan=2)
        
        # Frame for mapping list
        frame_mappings = ttk.Frame(root, padding=10)
        frame_mappings.pack(fill=tk.BOTH, expand=True)
        
        # Treeview for mappings
        columns = ("name", "address", "type", "description", "scale", "offset", "min", "max")
        tree = ttk.Treeview(frame_mappings, columns=columns, show="headings")
        
        tree.heading("name", text="Name")
        tree.heading("address", text="Address")
        tree.heading("type", text="Type")
        tree.heading("description", text="Description")
        tree.heading("scale", text="Scale")
        tree.heading("offset", text="Offset")
        tree.heading("min", text="Min")
        tree.heading("max", text="Max")
        
        tree.column("name", width=100)
        tree.column("address", width=80)
        tree.column("type", width=60)
        tree.column("description", width=150)
        tree.column("scale", width=60)
        tree.column("offset", width=60)
        tree.column("min", width=60)
        tree.column("max", width=60)
        
        scrollbar = ttk.Scrollbar(frame_mappings, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscroll=scrollbar.set)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        # Function to refresh the mapping list
        def refresh_mappings():
            for item in tree.get_children():
                tree.delete(item)
            
            for mapping in self.mappings:
                values = [
                    mapping["name"],
                    mapping["address"],
                    mapping["type"],
                    mapping.get("description", ""),
                    mapping.get("scale", 1.0),
                    mapping.get("offset", 0),
                    mapping.get("min", ""),
                    mapping.get("max", "")
                ]
                tree.insert("", tk.END, values=values)
        
        refresh_mappings()
        
        # Frame for buttons
        frame_buttons = ttk.Frame(root, padding=10)
        frame_buttons.pack(fill=tk.X)
        
        # Add mapping dialog
        def add_mapping_dialog():
            dialog = tk.Toplevel(root)
            dialog.title("Add Mapping")
            dialog.geometry("400x350")
            dialog.grab_set()
            
            ttk.Label(dialog, text="Name:").grid(row=0, column=0, sticky=tk.W, padx=5, pady=5)
            name_var = tk.StringVar()
            ttk.Entry(dialog, textvariable=name_var).grid(row=0, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Address:").grid(row=1, column=0, sticky=tk.W, padx=5, pady=5)
            addr_var = tk.StringVar()
            ttk.Entry(dialog, textvariable=addr_var).grid(row=1, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Type:").grid(row=2, column=0, sticky=tk.W, padx=5, pady=5)
            type_var = tk.StringVar(value="byte")
            type_combo = ttk.Combobox(dialog, textvariable=type_var, values=["byte", "word", "dword", "float"])
            type_combo.grid(row=2, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Description:").grid(row=3, column=0, sticky=tk.W, padx=5, pady=5)
            desc_var = tk.StringVar()
            ttk.Entry(dialog, textvariable=desc_var).grid(row=3, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Scale:").grid(row=4, column=0, sticky=tk.W, padx=5, pady=5)
            scale_var = tk.StringVar(value="1.0")
            ttk.Entry(dialog, textvariable=scale_var).grid(row=4, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Offset:").grid(row=5, column=0, sticky=tk.W, padx=5, pady=5)
            offset_var = tk.StringVar(value="0")
            ttk.Entry(dialog, textvariable=offset_var).grid(row=5, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Min Value:").grid(row=6, column=0, sticky=tk.W, padx=5, pady=5)
            min_var = tk.StringVar()
            ttk.Entry(dialog, textvariable=min_var).grid(row=6, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Max Value:").grid(row=7, column=0, sticky=tk.W, padx=5, pady=5)
            max_var = tk.StringVar()
            ttk.Entry(dialog, textvariable=max_var).grid(row=7, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            def save_mapping():
                name = name_var.get()
                address = addr_var.get()
                data_type = type_var.get()
                description = desc_var.get()
                scale = scale_var.get()
                offset = offset_var.get()
                min_value = min_var.get() or None
                max_value = max_var.get() or None
                
                if not name or not address:
                    messagebox.showerror("Error", "Name and Address are required")
                    return
                
                if self.add_mapping(name, address, data_type, description, 
                                   scale, offset, min_value, max_value):
                    refresh_mappings()
                    dialog.destroy()
            
            ttk.Button(dialog, text="Save", command=save_mapping).grid(row=8, column=0, padx=5, pady=10)
            ttk.Button(dialog, text="Cancel", command=dialog.destroy).grid(row=8, column=1, padx=5, pady=10)
        
        # Edit mapping dialog
        def edit_mapping_dialog():
            selected = tree.selection()
            if not selected:
                messagebox.showinfo("Info", "Please select a mapping to edit")
                return
                
            item = tree.item(selected[0])
            values = item["values"]
            name = values[0]
            
            # Find the mapping
            mapping = None
            for m in self.mappings:
                if m["name"] == name:
                    mapping = m
                    break
            
            if not mapping:
                messagebox.showerror("Error", f"Mapping '{name}' not found")
                return
            
            dialog = tk.Toplevel(root)
            dialog.title(f"Edit Mapping - {name}")
            dialog.geometry("400x350")
            dialog.grab_set()
            
            ttk.Label(dialog, text="Name:").grid(row=0, column=0, sticky=tk.W, padx=5, pady=5)
            name_var = tk.StringVar(value=mapping["name"])
            ttk.Entry(dialog, textvariable=name_var, state="readonly").grid(row=0, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Address:").grid(row=1, column=0, sticky=tk.W, padx=5, pady=5)
            addr_var = tk.StringVar(value=mapping["address"])
            ttk.Entry(dialog, textvariable=addr_var).grid(row=1, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Type:").grid(row=2, column=0, sticky=tk.W, padx=5, pady=5)
            type_var = tk.StringVar(value=mapping["type"])
            type_combo = ttk.Combobox(dialog, textvariable=type_var, values=["byte", "word", "dword", "float"])
            type_combo.grid(row=2, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Description:").grid(row=3, column=0, sticky=tk.W, padx=5, pady=5)
            desc_var = tk.StringVar(value=mapping.get("description", ""))
            ttk.Entry(dialog, textvariable=desc_var).grid(row=3, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Scale:").grid(row=4, column=0, sticky=tk.W, padx=5, pady=5)
            scale_var = tk.StringVar(value=str(mapping.get("scale", 1.0)))
            ttk.Entry(dialog, textvariable=scale_var).grid(row=4, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Offset:").grid(row=5, column=0, sticky=tk.W, padx=5, pady=5)
            offset_var = tk.StringVar(value=str(mapping.get("offset", 0)))
            ttk.Entry(dialog, textvariable=offset_var).grid(row=5, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Min Value:").grid(row=6, column=0, sticky=tk.W, padx=5, pady=5)
            min_var = tk.StringVar(value=str(mapping.get("min", "")))
            ttk.Entry(dialog, textvariable=min_var).grid(row=6, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            ttk.Label(dialog, text="Max Value:").grid(row=7, column=0, sticky=tk.W, padx=5, pady=5)
            max_var = tk.StringVar(value=str(mapping.get("max", "")))
            ttk.Entry(dialog, textvariable=max_var).grid(row=7, column=1, sticky=tk.W+tk.E, padx=5, pady=5)
            
            def save_mapping():
                address = addr_var.get()
                data_type = type_var.get()
                description = desc_var.get()
                scale = scale_var.get()
                offset = offset_var.get()
                min_value = min_var.get() or None
                max_value = max_var.get() or None
                
                if not address:
                    messagebox.showerror("Error", "Address is required")
                    return
                
                if self.edit_mapping(name, address=address, type=data_type, 
                                    description=description, scale=scale, 
                                    offset=offset, min=min_value, max=max_value):
                    refresh_mappings()
                    dialog.destroy()
            
            ttk.Button(dialog, text="Save", command=save_mapping).grid(row=8, column=0, padx=5, pady=10)
            ttk.Button(dialog, text="Cancel", command=dialog.destroy).grid(row=8, column=1, padx=5, pady=10)
        
        # Remove mapping
        def remove_mapping():
            selected = tree.selection()
            if not selected:
                messagebox.showinfo("Info", "Please select a mapping to remove")
                return
                
            item = tree.item(selected[0])
            name = item["values"][0]
            
            if messagebox.askyesno("Confirm", f"Remove mapping '{name}'?"):
                if self.remove_mapping(name):
                    refresh_mappings()
        
        # Load mapping file
        def load_mapping():
            file_path = filedialog.askopenfilename(
                title="Load Mapping File",
                filetypes=[("JSON Files", "*.json"), ("All Files", "*.*")]
            )
            if file_path:
                if self.load_mapping(file_path):
                    game_var.set(self.game_name)
                    arch_var.set(self.architecture)
                    refresh_mappings()
                    messagebox.showinfo("Info", f"Loaded {len(self.mappings)} mappings")
                else:
                    messagebox.showerror("Error", "Failed to load mapping file")
        
        # Save mapping file
        def save_mapping():
            file_path = filedialog.asksaveasfilename(
                title="Save Mapping File",
                defaultextension=".json",
                filetypes=[("JSON Files", "*.json"), ("All Files", "*.*")],
                initialfile=f"{self.game_name}.json"
            )
            if file_path:
                # Update game info before saving
                self.game_name = game_var.get()
                self.architecture = arch_var.get()
                
                if self.save_mapping(file_path):
                    messagebox.showinfo("Info", f"Saved {len(self.mappings)} mappings to {file_path}")
                else:
                    messagebox.showerror("Error", "Failed to save mapping file")
        
        # Add buttons
        ttk.Button(frame_buttons, text="Add", command=add_mapping_dialog).pack(side=tk.LEFT, padx=5)
        ttk.Button(frame_buttons, text="Edit", command=edit_mapping_dialog).pack(side=tk.LEFT, padx=5)
        ttk.Button(frame_buttons, text="Remove", command=remove_mapping).pack(side=tk.LEFT, padx=5)
        ttk.Button(frame_buttons, text="Load", command=load_mapping).pack(side=tk.LEFT, padx=5)
        ttk.Button(frame_buttons, text="Save", command=save_mapping).pack(side=tk.LEFT, padx=5)
        ttk.Button(frame_buttons, text="Exit", command=root.destroy).pack(side=tk.RIGHT, padx=5)
        
        # Start the GUI
        root.mainloop()


def main():
    """Main entry point for the memory mapping utility."""
    parser = argparse.ArgumentParser(description="FBNeo Memory Mapping Utility")
    parser.add_argument("game_name", nargs="?", help="Name of the game to create/edit mappings for")
    parser.add_argument("--architecture", help="Game architecture (CPS1, CPS2, etc.)")
    parser.add_argument("--mapping-file", help="Path to existing mapping file to load")
    parser.add_argument("--output", help="Path to save the mapping file")
    parser.add_argument("--interactive", action="store_true", help="Launch interactive editor")
    args = parser.parse_args()
    
    # If no game name provided, show help and exit
    if not args.game_name and not args.mapping_file:
        parser.print_help()
        return
    
    # Create the mapping utility
    game_name = args.game_name
    util = MappingUtility(game_name, args.architecture, args.mapping_file)
    
    # Launch interactive editor if requested
    if args.interactive:
        util.interactive_editor()
    
    # Save to output file if specified
    if args.output:
        util.save_mapping(args.output)


if __name__ == "__main__":
    main() 