#!/usr/bin/env python3
import argparse
import os
import sys
import subprocess
import importlib.util
import json
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from pathlib import Path
import webbrowser


class MappingSuite:
    """Unified interface for FBNeo memory mapping tools."""
    
    def __init__(self, base_dir=None):
        self.base_dir = base_dir or os.path.dirname(os.path.abspath(__file__))
        self.mappings_dir = os.path.join(self.base_dir, "mappings")
        self.ensure_directories()
        
        # Try to import tools (but don't fail if they're not available)
        self.validator_module = self.import_module("mapping_validator")
        self.utility_module = self.import_module("mapping_utility")
        self.discoverer_module = self.import_module("mapping_discoverer")
        
        # Store available games with mappings
        self.available_games = self.find_available_games()
    
    def ensure_directories(self):
        """Ensure required directories exist."""
        os.makedirs(self.mappings_dir, exist_ok=True)
    
    def import_module(self, module_name):
        """Dynamically import a module from the current directory."""
        try:
            module_path = os.path.join(self.base_dir, f"{module_name}.py")
            if not os.path.exists(module_path):
                return None
                
            spec = importlib.util.spec_from_file_location(module_name, module_path)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            return module
        except Exception as e:
            print(f"Error importing {module_name}: {e}")
            return None
    
    def find_available_games(self):
        """Find games with existing mapping files."""
        games = {}
        
        if not os.path.exists(self.mappings_dir):
            return games
            
        for file_path in Path(self.mappings_dir).glob("*.json"):
            try:
                with open(file_path, 'r') as f:
                    data = json.load(f)
                    game_name = data.get("game", file_path.stem)
                    games[game_name] = {
                        "path": str(file_path),
                        "architecture": data.get("architecture", "Unknown"),
                        "mapping_count": len(data.get("mappings", [])),
                        "description": data.get("description", "")
                    }
            except Exception:
                # Skip files that can't be parsed
                continue
                
        return games
    
    def validate_mapping(self, mapping_file):
        """Validate a mapping file using the validator tool."""
        if not self.validator_module:
            print("Validator module not available")
            return False, ["Validator module not available"]
            
        validator = self.validator_module.MappingValidator()
        return validator.validate(mapping_file)
    
    def validate_all_mappings(self, output_report=None):
        """Validate all mapping files and generate a report."""
        if not self.validator_module:
            print("Validator module not available")
            return {}
            
        validator = self.validator_module.MappingValidator()
        results = validator.validate_directory(self.mappings_dir)
        
        if output_report:
            validator.generate_report(results, output_report)
            
        return results
    
    def launch_utility(self, game_name=None, mapping_file=None):
        """Launch the mapping utility for a specific game."""
        if not self.utility_module:
            print("Utility module not available")
            return False
            
        try:
            args = []
            
            if game_name:
                args.append(game_name)
                
            if mapping_file:
                args.extend(["--mapping-file", mapping_file])
                
            args.append("--interactive")
            
            # Use the current Python interpreter to run the utility
            cmd = [sys.executable, 
                   os.path.join(self.base_dir, "mapping_utility.py")] + args
                   
            print(f"Launching: {' '.join(cmd)}")
            subprocess.Popen(cmd)
            return True
            
        except Exception as e:
            print(f"Error launching utility: {e}")
            return False
    
    def launch_discoverer(self, game_name):
        """Launch the mapping discoverer for a specific game."""
        if not self.discoverer_module:
            print("Discoverer module not available")
            return False
            
        try:
            # Use the current Python interpreter to run the discoverer
            cmd = [sys.executable, 
                   os.path.join(self.base_dir, "mapping_discoverer.py"),
                   game_name]
                   
            print(f"Launching: {' '.join(cmd)}")
            subprocess.Popen(cmd)
            return True
            
        except Exception as e:
            print(f"Error launching discoverer: {e}")
            return False
    
    def launch_gui(self):
        """Launch a GUI for the mapping suite."""
        root = tk.Tk()
        root.title("FBNeo Memory Mapping Suite")
        root.geometry("900x600")
        
        # Define styles
        style = ttk.Style()
        style.configure("TButton", padding=6)
        style.configure("Header.TLabel", font=("Arial", 14, "bold"))
        
        # Create main notebook
        notebook = ttk.Notebook(root)
        notebook.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Games tab
        games_frame = ttk.Frame(notebook, padding=10)
        notebook.add(games_frame, text="Games")
        
        # Games tab layout
        ttk.Label(games_frame, text="Available Games with Mappings", 
                 style="Header.TLabel").pack(anchor=tk.W, pady=(0, 10))
        
        # Games table
        games_cols = ("game", "arch", "mappings", "description")
        games_tree = ttk.Treeview(games_frame, columns=games_cols, show="headings")
        games_tree.heading("game", text="Game")
        games_tree.heading("arch", text="Architecture")
        games_tree.heading("mappings", text="Mappings")
        games_tree.heading("description", text="Description")
        
        games_tree.column("game", width=120)
        games_tree.column("arch", width=100)
        games_tree.column("mappings", width=80)
        games_tree.column("description", width=400)
        
        games_tree.pack(fill=tk.BOTH, expand=True)
        
        # Populate games
        for game, info in self.available_games.items():
            games_tree.insert("", tk.END, values=(
                game,
                info.get("architecture", ""),
                info.get("mapping_count", 0),
                info.get("description", "")
            ))
        
        # Games actions frame
        games_actions = ttk.Frame(games_frame)
        games_actions.pack(fill=tk.X, pady=10)
        
        def on_edit_game():
            selected = games_tree.selection()
            if not selected:
                messagebox.showinfo("Info", "Please select a game to edit")
                return
            
            item = games_tree.item(selected[0])
            game = item["values"][0]
            
            if game in self.available_games:
                mapping_file = self.available_games[game]["path"]
                self.launch_utility(game, mapping_file)
        
        def on_new_game():
            new_game = simpledialog.askstring("New Game", "Enter ROM name:")
            if new_game:
                self.launch_utility(new_game)
        
        def on_discover_mappings():
            selected = games_tree.selection()
            if selected:
                item = games_tree.item(selected[0])
                game = item["values"][0]
            else:
                game = simpledialog.askstring("Discover Mappings", "Enter ROM name:")
                
            if game:
                self.launch_discoverer(game)
        
        ttk.Button(games_actions, text="Edit Selected", 
                  command=on_edit_game).pack(side=tk.LEFT, padx=5)
        ttk.Button(games_actions, text="New Game", 
                  command=on_new_game).pack(side=tk.LEFT, padx=5)
        ttk.Button(games_actions, text="Discover Mappings", 
                  command=on_discover_mappings).pack(side=tk.LEFT, padx=5)
        
        # Validation tab
        validation_frame = ttk.Frame(notebook, padding=10)
        notebook.add(validation_frame, text="Validation")
        
        ttk.Label(validation_frame, text="Mapping File Validation", 
                 style="Header.TLabel").pack(anchor=tk.W, pady=(0, 10))
        
        validation_options = ttk.Frame(validation_frame)
        validation_options.pack(fill=tk.X, pady=10)
        
        def on_validate_file():
            file_path = filedialog.askopenfilename(
                title="Select Mapping File",
                filetypes=[("JSON Files", "*.json"), ("All Files", "*.*")],
                initialdir=self.mappings_dir
            )
            
            if file_path:
                is_valid, errors = self.validate_mapping(file_path)
                
                if is_valid:
                    messagebox.showinfo("Validation", "Mapping file is valid!")
                else:
                    error_text = "\n".join([f"- {e}" for e in errors])
                    messagebox.showerror("Validation Errors", 
                                        f"Mapping file is invalid:\n{error_text}")
        
        def on_validate_all():
            report_path = filedialog.asksaveasfilename(
                title="Save Validation Report",
                defaultextension=".md",
                filetypes=[("Markdown", "*.md"), ("Text", "*.txt"), ("All Files", "*.*")],
                initialdir=self.base_dir
            )
            
            if report_path:
                results = self.validate_all_mappings(report_path)
                
                valid_count = sum(1 for is_valid, _ in results.values() if is_valid)
                total_count = len(results)
                
                messagebox.showinfo("Validation Complete", 
                                  f"Validated {total_count} files.\n"
                                  f"Valid: {valid_count}\n"
                                  f"Invalid: {total_count - valid_count}\n\n"
                                  f"Report saved to {report_path}")
                
                # Open the report in the default app
                try:
                    webbrowser.open(report_path)
                except:
                    pass
        
        ttk.Button(validation_options, text="Validate File", 
                  command=on_validate_file).pack(side=tk.LEFT, padx=5)
        ttk.Button(validation_options, text="Validate All & Generate Report", 
                  command=on_validate_all).pack(side=tk.LEFT, padx=5)
        
        # Tools tab
        tools_frame = ttk.Frame(notebook, padding=10)
        notebook.add(tools_frame, text="Tools")
        
        ttk.Label(tools_frame, text="Memory Mapping Tools", 
                 style="Header.TLabel").pack(anchor=tk.W, pady=(0, 10))
        
        tools_list = ttk.Frame(tools_frame)
        tools_list.pack(fill=tk.BOTH, expand=True)
        
        # Tool descriptions
        tools = [
            {
                "name": "Mapping Utility",
                "description": "Create and edit memory mappings with an interactive GUI.",
                "module": "mapping_utility",
                "action": lambda: self.launch_utility()
            },
            {
                "name": "Mapping Discoverer",
                "description": "Automatically discover memory mappings by monitoring gameplay.",
                "module": "mapping_discoverer",
                "action": lambda: self.launch_discoverer(
                    simpledialog.askstring("Game", "Enter ROM name:")
                )
            },
            {
                "name": "Mapping Validator",
                "description": "Validate mapping files against the schema and generate reports.",
                "module": "mapping_validator",
                "action": on_validate_file
            }
        ]
        
        for i, tool in enumerate(tools):
            tool_frame = ttk.Frame(tools_list, padding=10)
            tool_frame.pack(fill=tk.X, pady=5)
            
            # Tool name and availability
            available = self.import_module(tool["module"]) is not None
            status = "Available" if available else "Not Available"
            status_color = "green" if available else "red"
            
            tool_header = ttk.Frame(tool_frame)
            tool_header.pack(fill=tk.X)
            
            ttk.Label(tool_header, text=tool["name"], 
                     font=("Arial", 12, "bold")).pack(side=tk.LEFT)
            ttk.Label(tool_header, text=f"[{status}]", 
                     foreground=status_color).pack(side=tk.LEFT, padx=10)
            
            # Tool description
            ttk.Label(tool_frame, text=tool["description"], 
                     wraplength=600).pack(anchor=tk.W, pady=5)
            
            # Launch button (if available)
            if available:
                ttk.Button(tool_frame, text="Launch", 
                          command=tool["action"]).pack(anchor=tk.W)
        
        # About tab
        about_frame = ttk.Frame(notebook, padding=10)
        notebook.add(about_frame, text="About")
        
        ttk.Label(about_frame, text="FBNeo Memory Mapping Suite", 
                 style="Header.TLabel").pack(anchor=tk.W, pady=(0, 10))
        
        about_text = (
            "This suite provides tools for working with FBNeo memory mappings, "
            "which are used for AI training, replay analysis, and visualization.\n\n"
            
            "The suite includes:\n"
            "• Mapping Utility: For creating and editing memory mappings\n"
            "• Mapping Discoverer: For automatically finding memory locations\n"
            "• Mapping Validator: For ensuring mappings conform to the schema\n\n"
            
            "Documentation for the mapping schema and tools can be found in the README files."
        )
        
        ttk.Label(about_frame, text=about_text, 
                 wraplength=600).pack(anchor=tk.W, pady=10)
        
        # Import tkinter.simpledialog at a point where we're sure tkinter is initialized
        from tkinter import simpledialog
        
        # Start the GUI loop
        root.mainloop()


def main():
    parser = argparse.ArgumentParser(description="FBNeo Memory Mapping Suite")
    parser.add_argument("--base-dir", 
                       help="Base directory for mapping tools and files")
    parser.add_argument("--validate", metavar="FILE", 
                       help="Validate a specific mapping file")
    parser.add_argument("--validate-all", action="store_true", 
                       help="Validate all mapping files")
    parser.add_argument("--report", metavar="FILE", 
                       help="Output path for validation report")
    parser.add_argument("--edit", metavar="GAME", 
                       help="Edit mappings for a specific game")
    parser.add_argument("--discover", metavar="GAME", 
                       help="Discover mappings for a specific game")
    
    args = parser.parse_args()
    
    suite = MappingSuite(args.base_dir)
    
    if args.validate:
        is_valid, errors = suite.validate_mapping(args.validate)
        if is_valid:
            print(f"✅ {args.validate} is valid")
            return 0
        else:
            print(f"❌ {args.validate} is invalid:")
            for error in errors:
                print(f"  - {error}")
            return 1
    
    elif args.validate_all:
        results = suite.validate_all_mappings(args.report)
        valid_count = sum(1 for is_valid, _ in results.values() if is_valid)
        total_count = len(results)
        
        print(f"Validated {total_count} files:")
        print(f"- Valid: {valid_count}")
        print(f"- Invalid: {total_count - valid_count}")
        
        if args.report:
            print(f"Report saved to {args.report}")
        
        return 0 if valid_count == total_count else 1
    
    elif args.edit:
        # Find mapping file if it exists
        mapping_file = None
        if args.edit in suite.available_games:
            mapping_file = suite.available_games[args.edit]["path"]
            
        success = suite.launch_utility(args.edit, mapping_file)
        return 0 if success else 1
    
    elif args.discover:
        success = suite.launch_discoverer(args.discover)
        return 0 if success else 1
    
    else:
        # No specific action, launch the GUI
        suite.launch_gui()
        return 0


if __name__ == "__main__":
    sys.exit(main()) 