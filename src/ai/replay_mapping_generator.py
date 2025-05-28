#!/usr/bin/env python3
"""
Replay Mapping Generator

A tool for automatically generating memory mapping files by analyzing replay data.
This tool compares states across replay frames to identify important memory regions
that change consistently with game actions.
"""

import os
import sys
import json
import argparse
import numpy as np
from typing import Dict, List, Set, Tuple, Optional, Any
from collections import defaultdict

class ReplayFrame:
    """Represents a single frame of replay data with memory state"""
    
    def __init__(self, frame_number: int, memory_data: bytes, inputs: Dict[str, Any] = None):
        self.frame_number = frame_number
        self.memory_data = memory_data
        self.inputs = inputs or {}
        
    def diff(self, other: 'ReplayFrame') -> Dict[int, Tuple[int, int]]:
        """Calculate differences between two replay frames
        
        Returns:
            Dict mapping address -> (old_value, new_value)
        """
        diffs = {}
        min_len = min(len(self.memory_data), len(other.memory_data))
        
        for i in range(min_len):
            if self.memory_data[i] != other.memory_data[i]:
                diffs[i] = (self.memory_data[i], other.memory_data[i])
                
        return diffs


class ReplayMappingGenerator:
    """Generates memory mappings from replay data analysis"""
    
    def __init__(self, game_name: str, architecture: str):
        """Initialize the generator
        
        Args:
            game_name: Name of the game
            architecture: Game architecture (CPS1, CPS2, NEOGEO, etc.)
        """
        self.game_name = game_name
        self.architecture = architecture
        self.frames = []
        self.frame_diffs = []
        self.address_patterns = defaultdict(list)
        self.input_correlated_addresses = defaultdict(set)
        
    def load_replay(self, replay_file: str) -> bool:
        """Load replay data from a file
        
        Args:
            replay_file: Path to the replay file
            
        Returns:
            True if successful, False otherwise
        """
        print(f"Loading replay file: {replay_file}")
        
        if not os.path.exists(replay_file):
            print(f"Error: Replay file not found: {replay_file}")
            return False
            
        try:
            # This is a simplified version of replay loading
            # In a real implementation, this would parse the actual replay format
            
            # For testing, we'll generate mock frames with synthetic memory data
            frame_count = 100
            memory_size = self._get_memory_size_for_arch()
            
            print(f"Generating {frame_count} mock frames for testing")
            for i in range(frame_count):
                # Generate deterministic but changing memory data
                memory_data = bytearray(memory_size)
                
                # Fill memory with base pattern
                for j in range(memory_size):
                    # Give some bytes consistent patterns
                    if j % 16 == 0:  # Counter
                        memory_data[j] = (i // 2) % 256
                    elif j % 16 == 1:  # Player 1 health (decreases)
                        memory_data[j] = max(0, 100 - i // 3)
                    elif j % 16 == 2:  # Player 2 health (increases then decreases)
                        if i < frame_count // 2:
                            memory_data[j] = min(100, 50 + i // 2)
                        else:
                            memory_data[j] = max(0, 100 - (i - frame_count // 2) // 2)
                    elif j % 16 == 3:  # Binary toggle
                        memory_data[j] = 1 if (i // 10) % 2 == 0 else 0
                    else:
                        # Random but deterministic changes
                        memory_data[j] = (j * i) % 256
                
                # Mock input data (some correlation with memory)
                inputs = {
                    "p1_left": (i % 20) < 5,
                    "p1_right": 5 <= (i % 20) < 10,
                    "p1_up": 10 <= (i % 20) < 12,
                    "p1_down": 12 <= (i % 20) < 14,
                    "p1_button1": 15 <= (i % 30) < 17,
                    "p1_button2": 20 <= (i % 40) < 23,
                    "p1_button3": i % 50 == 0,
                }
                
                self.frames.append(ReplayFrame(i, bytes(memory_data), inputs))
                
            print(f"Successfully loaded/generated {len(self.frames)} frames")
            return True
                
        except Exception as e:
            print(f"Error loading replay: {e}")
            return False
    
    def _get_memory_size_for_arch(self) -> int:
        """Get the expected memory size for the specified architecture"""
        return {
            "CPS1": 0x10000,
            "CPS2": 0x20000,
            "CPS3": 0x40000,
            "NEOGEO": 0x10000,
            "M68K": 0x20000,
            "Z80": 0x8000,
        }.get(self.architecture, 0x10000)
    
    def analyze_replay(self) -> Dict[str, Any]:
        """Analyze the loaded replay frames
        
        Returns:
            Dictionary with analysis results
        """
        if len(self.frames) < 2:
            print("Error: At least 2 frames are required for analysis")
            return {}
            
        print(f"Analyzing {len(self.frames)} frames...")
        results = {
            "constant_addresses": set(),
            "changing_addresses": defaultdict(list),
            "input_correlated": defaultdict(dict),
            "patterns": [],
            "grouped_addresses": []
        }
        
        # Analyze differences between consecutive frames
        diffs_by_address = defaultdict(list)
        all_changed_addresses = set()
        
        for i in range(1, len(self.frames)):
            prev_frame = self.frames[i-1]
            curr_frame = self.frames[i]
            
            diffs = curr_frame.diff(prev_frame)
            self.frame_diffs.append((i-1, i, diffs))
            all_changed_addresses.update(diffs.keys())
            
            for addr, (old_val, new_val) in diffs.items():
                diffs_by_address[addr].append((old_val, new_val, i))
                
                # Track input correlations (if input changed in this frame)
                for input_name, input_val in curr_frame.inputs.items():
                    prev_input = prev_frame.inputs.get(input_name, False)
                    if input_val != prev_input:
                        self.input_correlated_addresses[input_name].add(addr)
        
        # Find addresses that never change
        first_frame_size = len(self.frames[0].memory_data)
        all_addresses = set(range(first_frame_size))
        results["constant_addresses"] = all_addresses - all_changed_addresses
        
        # Analyze changing addresses
        for addr, changes in diffs_by_address.items():
            change_counts = len(changes)
            unique_values = set(old for old, _, _ in changes).union(set(new for _, new, _ in changes))
            
            results["changing_addresses"][addr] = {
                "changes": changes,
                "change_count": change_counts,
                "unique_values": len(unique_values),
                "values": sorted(list(unique_values)),
                "correlations": self._find_correlations(addr, changes)
            }
        
        # Add input correlations
        for input_name, addresses in self.input_correlated_addresses.items():
            results["input_correlated"][input_name] = {
                "addresses": sorted(list(addresses)),
                "count": len(addresses)
            }
        
        # Find patterns and groups
        results["patterns"] = self._find_patterns(diffs_by_address)
        results["grouped_addresses"] = self._group_addresses(diffs_by_address)
        
        return results
    
    def _find_correlations(self, addr: int, changes: List[Tuple[int, int, int]]) -> Dict[str, float]:
        """Find correlations between address changes and other factors
        
        Args:
            addr: Memory address
            changes: List of (old_val, new_val, frame_idx) tuples
            
        Returns:
            Dict with correlation scores
        """
        correlations = {}
        
        # Input correlations
        for input_name, correlated_addrs in self.input_correlated_addresses.items():
            if addr in correlated_addrs:
                change_frames = set(idx for _, _, idx in changes)
                
                # Calculate how many input changes coincide with address changes
                input_changes = 0
                for i in range(1, len(self.frames)):
                    prev_input = self.frames[i-1].inputs.get(input_name, False)
                    curr_input = self.frames[i].inputs.get(input_name, False)
                    
                    if prev_input != curr_input and i in change_frames:
                        input_changes += 1
                
                if input_changes > 0:
                    score = input_changes / len(change_frames)
                    if score > 0.3:  # Only include significant correlations
                        correlations[f"input_{input_name}"] = score
        
        # Trend correlations (increasing/decreasing)
        increases = 0
        decreases = 0
        
        for old_val, new_val, _ in changes:
            if new_val > old_val:
                increases += 1
            elif new_val < old_val:
                decreases += 1
        
        total_changes = len(changes)
        if increases / total_changes > 0.7:
            correlations["trend_increasing"] = increases / total_changes
        elif decreases / total_changes > 0.7:
            correlations["trend_decreasing"] = decreases / total_changes
        
        # Oscillation patterns (values that alternate)
        if len(changes) >= 4:
            oscillations = 0
            for i in range(len(changes) - 2):
                old1, new1, _ = changes[i]
                old2, new2, _ = changes[i+1]
                
                if (new1 > old1 and new2 < old2) or (new1 < old1 and new2 > old2):
                    oscillations += 1
            
            oscillation_score = oscillations / (len(changes) - 1)
            if oscillation_score > 0.5:
                correlations["oscillating"] = oscillation_score
        
        return correlations
    
    def _find_patterns(self, diffs_by_address: Dict[int, List[Tuple[int, int, int]]]) -> List[Dict[str, Any]]:
        """Find groups of addresses that change together
        
        Args:
            diffs_by_address: Dict mapping address to list of changes
            
        Returns:
            List of patterns
        """
        patterns = []
        
        # Create a map of frames where each address changed
        address_change_frames = {}
        for addr, changes in diffs_by_address.items():
            address_change_frames[addr] = set(idx for _, _, idx in changes)
        
        # Find addresses that change together
        processed_addresses = set()
        
        for addr, frames in address_change_frames.items():
            if addr in processed_addresses or len(frames) < 2:
                continue
                
            related_addresses = []
            
            for other_addr, other_frames in address_change_frames.items():
                if addr == other_addr or other_addr in processed_addresses:
                    continue
                    
                # Calculate similarity (Jaccard index)
                intersection = len(frames.intersection(other_frames))
                union = len(frames.union(other_frames))
                similarity = intersection / union if union > 0 else 0
                
                if similarity > 0.7:  # Highly related
                    related_addresses.append(other_addr)
            
            if related_addresses:
                # Add the original address to the group
                group = [addr] + related_addresses
                
                # Sort by address to find consecutive memory regions
                group.sort()
                
                patterns.append({
                    "addresses": group,
                    "frame_indices": list(frames),
                    "ranges": self._check_consecutive_ranges(group)
                })
                
                processed_addresses.update(group)
        
        # Sort patterns by size (largest first)
        patterns.sort(key=lambda x: len(x["addresses"]), reverse=True)
        return patterns
    
    def _check_consecutive_ranges(self, addresses: List[int]) -> List[List[int]]:
        """Check for consecutive address ranges within a group
        
        Args:
            addresses: List of addresses (sorted)
            
        Returns:
            List of consecutive address ranges [start, end]
        """
        if not addresses:
            return []
            
        ranges = []
        start = addresses[0]
        current = start
        
        for addr in addresses[1:]:
            if addr == current + 1:
                current = addr
            else:
                ranges.append([start, current])
                start = addr
                current = addr
                
        ranges.append([start, current])
        return ranges
    
    def _group_addresses(self, diffs_by_address: Dict[int, List[Tuple[int, int, int]]]) -> List[Dict[str, Any]]:
        """Group addresses that might represent the same data structure
        
        Args:
            diffs_by_address: Dict mapping address to list of changes
            
        Returns:
            List of potential data structures
        """
        structures = []
        
        # Find consecutive addresses
        all_addresses = sorted(diffs_by_address.keys())
        consecutive_groups = []
        
        if not all_addresses:
            return structures
            
        current_group = [all_addresses[0]]
        
        for i in range(1, len(all_addresses)):
            if all_addresses[i] == all_addresses[i-1] + 1:
                current_group.append(all_addresses[i])
            else:
                if len(current_group) > 1:
                    consecutive_groups.append(current_group)
                current_group = [all_addresses[i]]
        
        if len(current_group) > 1:
            consecutive_groups.append(current_group)
        
        # Analyze each group to determine potential data types
        for group in consecutive_groups:
            if len(group) >= 2:
                structures.append({
                    "start_address": group[0],
                    "end_address": group[-1],
                    "length": len(group),
                    "data_type": self._guess_data_type(group, diffs_by_address)
                })
        
        return structures
    
    def _guess_data_type(self, addresses: List[int], diffs_by_address: Dict[int, List[Tuple[int, int, int]]]) -> str:
        """Attempt to guess the data type based on address pattern and values
        
        Args:
            addresses: List of consecutive addresses
            diffs_by_address: Changes for each address
            
        Returns:
            String describing the likely data type
        """
        length = len(addresses)
        
        if length == 2:
            return "word (16-bit)"
        elif length == 4:
            return "dword (32-bit)"
        elif length == 8:
            return "qword (64-bit)"
        elif length == 4 and self._check_float_pattern(addresses, diffs_by_address):
            return "float (32-bit)"
        else:
            return f"array ({length} bytes)"
    
    def _check_float_pattern(self, addresses: List[int], diffs_by_address: Dict[int, List[Tuple[int, int, int]]]) -> bool:
        """Check if a 4-byte region resembles a floating point value
        
        Args:
            addresses: List of 4 consecutive addresses
            diffs_by_address: Changes for each address
            
        Returns:
            True if the pattern resembles a float
        """
        if len(addresses) != 4:
            return False
            
        change_counts = [len(diffs_by_address[addr]) for addr in addresses]
        
        # Typical float pattern: lower bytes change more frequently than higher bytes
        # This is a simplified heuristic
        return change_counts[0] <= change_counts[3] and change_counts[1] <= change_counts[3]
    
    def generate_mapping(self, analysis_results: Dict[str, Any]) -> Dict[str, Any]:
        """Generate a memory mapping file from analysis results
        
        Args:
            analysis_results: Results from analyze_replay
            
        Returns:
            Dictionary with memory mapping
        """
        if not analysis_results:
            return {}
            
        mappings = {
            "game_name": self.game_name,
            "architecture": self.architecture,
            "version": "1.0",
            "mappings": []
        }
        
        # Add potential player state mappings (health, position, etc.)
        player_mappings = self._generate_player_mappings(analysis_results)
        mappings["mappings"].extend(player_mappings)
        
        # Add game state mappings
        game_state_mappings = self._generate_game_state_mappings(analysis_results)
        mappings["mappings"].extend(game_state_mappings)
        
        # Add input-correlated mappings
        input_mappings = self._generate_input_mappings(analysis_results)
        mappings["mappings"].extend(input_mappings)
        
        # Add data structure mappings
        structure_mappings = self._generate_structure_mappings(analysis_results)
        mappings["mappings"].extend(structure_mappings)
        
        # Create groups based on categories
        mappings["groups"] = self._generate_groups(mappings["mappings"])
        
        return mappings
    
    def _generate_player_mappings(self, analysis_results: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Generate mappings for player-related data
        
        Args:
            analysis_results: Analysis results
            
        Returns:
            List of mapping entries
        """
        mappings = []
        
        # Look for health-like values (decreasing trend, limited range)
        health_candidates = []
        
        for addr, data in analysis_results["changing_addresses"].items():
            correlations = data.get("correlations", {})
            
            # Health typically decreases and has limited range
            if correlations.get("trend_decreasing", 0) > 0.5:
                unique_values = data["unique_values"]
                
                if 5 <= unique_values <= 100:
                    values = data["values"]
                    value_range = max(values) - min(values)
                    
                    if 10 <= value_range <= 255:
                        health_candidates.append({
                            "address": addr,
                            "max_value": max(values),
                            "min_value": min(values),
                            "score": correlations.get("trend_decreasing", 0)
                        })
        
        # Sort by score and take top candidates
        health_candidates.sort(key=lambda x: x["score"], reverse=True)
        
        # Add player 1 health (best candidate)
        if health_candidates:
            best = health_candidates[0]
            mappings.append({
                "name": "p1_health",
                "address": f"0x{best['address']:X}",
                "type": "byte",
                "category": "player1",
                "description": "Player 1 health",
                "min_value": best["min_value"],
                "max_value": best["max_value"]
            })
            
            # If we have another candidate, it might be player 2 health
            if len(health_candidates) > 1:
                p2 = health_candidates[1]
                mappings.append({
                    "name": "p2_health",
                    "address": f"0x{p2['address']:X}",
                    "type": "byte",
                    "category": "player2",
                    "description": "Player 2 health",
                    "min_value": p2["min_value"],
                    "max_value": p2["max_value"]
                })
        
        # Find position-like values (oscillating patterns)
        position_candidates = []
        
        for addr, data in analysis_results["changing_addresses"].items():
            correlations = data.get("correlations", {})
            
            # Positions typically oscillate as characters move back and forth
            if correlations.get("oscillating", 0) > 0.5:
                # For positions, look for input correlations with movement buttons
                input_correlations = {k: v for k, v in correlations.items() if k.startswith("input_p1_")}
                
                if any(k in input_correlations for k in ["input_p1_left", "input_p1_right"]):
                    position_candidates.append({
                        "address": addr,
                        "score": correlations.get("oscillating", 0),
                        "input_score": sum(input_correlations.values())
                    })
        
        # Sort by combined score and take top candidates
        position_candidates.sort(key=lambda x: x["score"] + x["input_score"], reverse=True)
        
        # Add player position mappings
        if position_candidates:
            x_pos = position_candidates[0]
            mappings.append({
                "name": "p1_x_position", 
                "address": f"0x{x_pos['address']:X}",
                "type": "word",
                "category": "player1",
                "description": "Player 1 X position"
            })
            
            # If we have another candidate with similar pattern, it might be Y position
            if len(position_candidates) > 1:
                y_pos = position_candidates[1]
                mappings.append({
                    "name": "p1_y_position",
                    "address": f"0x{y_pos['address']:X}",
                    "type": "word",
                    "category": "player1",
                    "description": "Player 1 Y position"
                })
        
        return mappings
    
    def _generate_game_state_mappings(self, analysis_results: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Generate mappings for game state data
        
        Args:
            analysis_results: Analysis results
            
        Returns:
            List of mapping entries
        """
        mappings = []
        
        # Look for timer-like values (steadily decreasing)
        timer_candidates = []
        
        for addr, data in analysis_results["changing_addresses"].items():
            correlations = data.get("correlations", {})
            
            # Timers typically decrease at a steady rate
            if correlations.get("trend_decreasing", 0) > 0.8:
                changes = data["changes"]
                
                # Check if the changes are regular (same decrement each time)
                decrements = [old - new for old, new, _ in changes if old > new]
                if len(decrements) >= 5:
                    # If most decrements are the same value, it's likely a timer
                    most_common = max(set(decrements), key=decrements.count)
                    if decrements.count(most_common) / len(decrements) > 0.6:
                        timer_candidates.append({
                            "address": addr,
                            "decrement": most_common,
                            "score": correlations.get("trend_decreasing", 0)
                        })
        
        # Sort by score and take top candidate
        timer_candidates.sort(key=lambda x: x["score"], reverse=True)
        
        if timer_candidates:
            timer = timer_candidates[0]
            mappings.append({
                "name": "timer",
                "address": f"0x{timer['address']:X}",
                "type": "byte",
                "category": "game_state",
                "description": "Round timer"
            })
        
        # Look for counter-like values (steadily increasing)
        counter_candidates = []
        
        for addr, data in analysis_results["changing_addresses"].items():
            correlations = data.get("correlations", {})
            
            # Counters typically increase steadily
            if correlations.get("trend_increasing", 0) > 0.8:
                changes = data["changes"]
                
                # Check if the changes are regular (same increment each time)
                increments = [new - old for old, new, _ in changes if new > old]
                if len(increments) >= 5:
                    # If most increments are the same value, it's likely a counter
                    most_common = max(set(increments), key=increments.count)
                    if increments.count(most_common) / len(increments) > 0.6:
                        counter_candidates.append({
                            "address": addr,
                            "increment": most_common,
                            "score": correlations.get("trend_increasing", 0)
                        })
        
        # Sort by score and take top candidates
        counter_candidates.sort(key=lambda x: x["score"], reverse=True)
        
        if counter_candidates:
            counter = counter_candidates[0]
            mappings.append({
                "name": "frame_counter",
                "address": f"0x{counter['address']:X}",
                "type": "word",
                "category": "game_state",
                "description": "Frame counter"
            })
        
        # Look for binary flags (values that toggle between 0/1)
        flag_candidates = []
        
        for addr, data in analysis_results["changing_addresses"].items():
            if data["unique_values"] == 2 and set(data["values"]) == {0, 1}:
                # Check how often the flag changes
                changes = data["changes"]
                change_rate = len(changes) / len(self.frames)
                
                flag_candidates.append({
                    "address": addr,
                    "change_rate": change_rate
                })
        
        # Sort by change rate (lower is better for stable game state flags)
        flag_candidates.sort(key=lambda x: x["change_rate"])
        
        # Add top 3 flags
        for i, flag in enumerate(flag_candidates[:3]):
            mappings.append({
                "name": f"flag_{i+1}",
                "address": f"0x{flag['address']:X}",
                "type": "bit",
                "bit_position": 0,
                "category": "game_state",
                "description": f"Game state flag {i+1}"
            })
        
        return mappings
    
    def _generate_input_mappings(self, analysis_results: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Generate mappings for input-correlated memory addresses
        
        Args:
            analysis_results: Analysis results
            
        Returns:
            List of mapping entries
        """
        mappings = []
        
        # Process input correlations
        for input_name, data in analysis_results.get("input_correlated", {}).items():
            if not data.get("addresses"):
                continue
                
            # Take top 2 best correlations per input
            top_addrs = data["addresses"][:2]
            
            for i, addr in enumerate(top_addrs):
                addr_data = analysis_results["changing_addresses"].get(addr, {})
                correlations = addr_data.get("correlations", {})
                
                if correlations.get(f"input_{input_name}", 0) > 0.5:
                    # Input highly correlated with this address
                    mappings.append({
                        "name": f"{input_name}_state",
                        "address": f"0x{addr:X}",
                        "type": "bit" if addr_data.get("unique_values") == 2 else "byte",
                        "bit_position": 0 if addr_data.get("unique_values") == 2 else None,
                        "category": "input_state",
                        "description": f"Memory state for {input_name} input"
                    })
                    
                    # Only take one address per input
                    break
        
        return mappings
    
    def _generate_structure_mappings(self, analysis_results: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Generate mappings for identified data structures
        
        Args:
            analysis_results: Analysis results
            
        Returns:
            List of mapping entries
        """
        mappings = []
        
        # Add mappings for multi-byte structures
        for i, structure in enumerate(analysis_results.get("grouped_addresses", [])[:5]):
            start_addr = structure["start_address"]
            length = structure["length"]
            data_type = structure["data_type"].split()[0]  # Get the base type (word, dword, etc.)
            
            if length >= 4:
                # This might be an important data structure
                mappings.append({
                    "name": f"struct_{i+1}",
                    "address": f"0x{start_addr:X}",
                    "type": data_type,
                    "category": "data_structures",
                    "description": f"Data structure {i+1} ({length} bytes)"
                })
        
        # Add mappings for patterns (addresses that change together)
        for i, pattern in enumerate(analysis_results.get("patterns", [])[:3]):
            if pattern.get("ranges"):
                # Use the first range as the basis for the mapping
                start_addr = pattern["ranges"][0][0]
                end_addr = pattern["ranges"][0][1]
                length = end_addr - start_addr + 1
                
                if length in (2, 4, 8):
                    data_type = {2: "word", 4: "dword", 8: "qword"}.get(length, "byte")
                    
                    mappings.append({
                        "name": f"pattern_{i+1}",
                        "address": f"0x{start_addr:X}",
                        "type": data_type,
                        "category": "patterns",
                        "description": f"Memory pattern {i+1} ({length} bytes)"
                    })
        
        return mappings
    
    def _generate_groups(self, mappings: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """Generate mapping groups from categorized mappings
        
        Args:
            mappings: List of mapping entries
            
        Returns:
            List of group definitions
        """
        # Group mappings by category
        categories = defaultdict(list)
        
        for mapping in mappings:
            category = mapping.get("category", "other")
            categories[category].append(mapping["name"])
        
        # Create group entries
        groups = []
        
        for category, names in categories.items():
            if names:
                groups.append({
                    "name": category,
                    "mappings": names
                })
        
        return groups
    
    def save_mapping(self, mapping: Dict[str, Any], output_file: str) -> None:
        """Save the mapping file
        
        Args:
            mapping: Mapping definition
            output_file: Output file path
        """
        # Create directory if it doesn't exist
        os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)
        
        with open(output_file, 'w') as f:
            json.dump(mapping, f, indent=2)
            
        print(f"Mapping saved to {output_file}")


def main():
    parser = argparse.ArgumentParser(description="Generate memory mappings from replay data")
    parser.add_argument("game", help="ROM name for the game")
    parser.add_argument("--architecture", default="CPS1", choices=["CPS1", "CPS2", "CPS3", "NEOGEO", "M68K", "Z80"], 
                        help="Game architecture type")
    parser.add_argument("--replay", help="Path to replay file")
    parser.add_argument("--output", help="Output file for mapping")
    
    args = parser.parse_args()
    
    generator = ReplayMappingGenerator(args.game, args.architecture)
    
    if args.replay:
        success = generator.load_replay(args.replay)
        if not success:
            return 1
    else:
        # Generate mock data for testing if no replay provided
        print("No replay file provided, generating mock data")
        success = generator.load_replay("mock_data")
        if not success:
            return 1
    
    # Analyze the replay data
    results = generator.analyze_replay()
    
    # Generate mapping
    mapping = generator.generate_mapping(results)
    
    # Save the mapping
    if args.output:
        generator.save_mapping(mapping, args.output)
    else:
        # Default output file in mappings directory
        output_file = os.path.join("mappings", f"{args.game}.json")
        generator.save_mapping(mapping, output_file)
    
    return 0


if __name__ == "__main__":
    sys.exit(main()) 