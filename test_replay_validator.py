#!/usr/bin/env python3
"""
Unit Tests for the Replay Validation System

This module contains automated tests to verify that the replay validation
system correctly identifies mismatches between original and validation replays.
"""

import unittest
import os
import tempfile
import json
import jsonlines
import shutil
import random
import subprocess
import sys
from datetime import datetime

# Import the visualizer to test its methods
try:
    from replay_visualizer import ReplayVisualizer
except ImportError:
    # If running from a different directory
    sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    from replay_visualizer import ReplayVisualizer


class TestReplayValidator(unittest.TestCase):
    """Test case for the replay validation system."""

    def setUp(self):
        """Set up test fixtures."""
        # Create a temporary directory for test files
        self.test_dir = tempfile.mkdtemp()
        
        # Create visualizer instance
        self.visualizer = ReplayVisualizer()
        
        # Create sample replay data
        self.create_sample_replay_data()
    
    def tearDown(self):
        """Tear down test fixtures."""
        # Remove temporary directory and its contents
        shutil.rmtree(self.test_dir)
    
    def create_sample_replay_data(self):
        """Create sample replay data files for testing."""
        # Create original replay file
        self.original_replay_path = os.path.join(self.test_dir, "original.jsonl")
        
        # Create validation replay file (with some differences)
        self.validation_replay_path = os.path.join(self.test_dir, "validation.jsonl")
        
        # Generate 100 frames of mock data
        original_frames = []
        validation_frames = []
        
        # Known mismatches to verify the detection
        self.expected_mismatches = []
        
        for i in range(100):
            frame_num = i * 10  # Frame numbers by 10s for simplicity
            
            # Create base frame data
            orig_frame = {
                "frame": frame_num,
                "p1_health": 1.0 - (i * 0.01),  # Decreasing health
                "p2_health": 1.0 - (i * 0.005),  # Decreasing health slower
                "p1_x": 0.2 + (i * 0.005),  # Moving right
                "p1_y": 0.5,
                "p2_x": 0.8 - (i * 0.003),  # Moving left
                "p2_y": 0.5,
                "hash": f"frame_{frame_num}_hash_original"
            }
            
            # Create validation frame (mostly the same)
            val_frame = orig_frame.copy()
            val_frame["hash"] = f"frame_{frame_num}_hash_validation"
            
            # Introduce deliberate differences in specific frames
            if i % 20 == 0:  # Every 20th frame has a health difference
                val_frame["p1_health"] = orig_frame["p1_health"] - 0.05
                self.expected_mismatches.append({
                    "frame": frame_num,
                    "reason": "health_mismatch"
                })
            
            if i % 25 == 0:  # Every 25th frame has a position difference
                val_frame["p1_x"] = orig_frame["p1_x"] + 0.1
                self.expected_mismatches.append({
                    "frame": frame_num,
                    "reason": "position_mismatch"
                })
            
            if i % 33 == 0:  # Every 33rd frame has a hash mismatch
                # This is already different due to different hash value construction
                self.expected_mismatches.append({
                    "frame": frame_num,
                    "reason": "hash_mismatch"
                })
                
            original_frames.append(orig_frame)
            validation_frames.append(val_frame)
        
        # Write to JSONL files
        with jsonlines.open(self.original_replay_path, mode='w') as writer:
            writer.write_all(original_frames)
        
        with jsonlines.open(self.validation_replay_path, mode='w') as writer:
            writer.write_all(validation_frames)
    
    def test_load_replay(self):
        """Test loading replay data from files."""
        # Load original replay
        original_frames = self.visualizer.load_replay(self.original_replay_path)
        self.assertEqual(len(original_frames), 100, "Should load 100 frames from original replay")
        
        # Load validation replay
        validation_frames = self.visualizer.load_replay(self.validation_replay_path)
        self.assertEqual(len(validation_frames), 100, "Should load 100 frames from validation replay")
        
        # Check frame sorting
        self.assertEqual(original_frames[0]['frame'], 0, "First frame should have number 0")
        self.assertEqual(original_frames[-1]['frame'], 990, "Last frame should have number 990")
    
    def test_find_mismatches(self):
        """Test finding mismatches between original and validation replays."""
        # Load replays
        original_frames = self.visualizer.load_replay(self.original_replay_path)
        validation_frames = self.visualizer.load_replay(self.validation_replay_path)
        
        # Find mismatches
        mismatches = self.visualizer.find_mismatches(original_frames, validation_frames)
        
        # Verify mismatches
        self.assertGreater(len(mismatches), 0, "Should find at least one mismatch")
        
        # Count expected mismatches by type
        expected_mismatch_frames = set(m["frame"] for m in self.expected_mismatches)
        
        # Convert actual mismatches to set of frame numbers for comparison
        actual_mismatch_frames = set(m["frame"] for m in mismatches)
        
        # Check that all expected mismatches were found
        self.assertEqual(expected_mismatch_frames, actual_mismatch_frames, 
                        "All expected mismatch frames should be detected")
    
    def test_setup_time_series(self):
        """Test extracting time series data for plotting."""
        # Load original replay
        original_frames = self.visualizer.load_replay(self.original_replay_path)
        
        # Setup time series
        time_series = self.visualizer.setup_time_series(original_frames)
        
        # Verify time series data
        self.assertIn('frame', time_series, "Should have 'frame' in time series")
        self.assertIn('p1_health', time_series, "Should have 'p1_health' in time series")
        self.assertIn('p2_health', time_series, "Should have 'p2_health' in time series")
        self.assertIn('p1_x', time_series, "Should have 'p1_x' in time series")
        self.assertIn('p2_x', time_series, "Should have 'p2_x' in time series")
        
        # Verify data length
        self.assertEqual(len(time_series['frame']), 100, "Should have 100 frames in time series")
        
        # Verify data values (first and last)
        self.assertEqual(time_series['frame'][0], 0, "First frame should be 0")
        self.assertEqual(time_series['frame'][-1], 990, "Last frame should be 990")
    
    def test_create_visualization(self):
        """Test creating visualization figure."""
        # Load data into visualizer
        self.visualizer.replay_frames = self.visualizer.load_replay(self.original_replay_path)
        self.visualizer.validation_frames = self.visualizer.load_replay(self.validation_replay_path)
        self.visualizer.mismatches = self.visualizer.find_mismatches(
            self.visualizer.replay_frames, self.visualizer.validation_frames)
        
        # Create visualization (but don't display it)
        fig = self.visualizer.create_visualization()
        
        # Verify that figure was created
        self.assertIsNotNone(fig, "Should create a figure object")
    
    def test_create_animation(self):
        """Test creating animation output."""
        # Load data into visualizer
        self.visualizer.replay_frames = self.visualizer.load_replay(self.original_replay_path)
        self.visualizer.validation_frames = self.visualizer.load_replay(self.validation_replay_path)
        self.visualizer.mismatches = self.visualizer.find_mismatches(
            self.visualizer.replay_frames, self.visualizer.validation_frames)
        
        # Define output path for animation
        output_path = os.path.join(self.test_dir, "test_animation.mp4")
        
        # Skip this test if ffmpeg is not available
        try:
            subprocess.run(["ffmpeg", "-version"], 
                          stdout=subprocess.PIPE, 
                          stderr=subprocess.PIPE)
        except FileNotFoundError:
            self.skipTest("ffmpeg not available, skipping animation test")
        
        # Create animation
        result = self.visualizer.create_animation(
            output_path, 
            fps=10,
            dpi=72,
            frame_range=(0, 100)  # Just first 10 frames to make it quick
        )
        
        # Verify animation was created
        self.assertTrue(result, "Animation creation should succeed")
        self.assertTrue(os.path.exists(output_path), "Animation file should exist")
        self.assertGreater(os.path.getsize(output_path), 0, "Animation file should not be empty")


class TestEdgeCases(unittest.TestCase):
    """Test edge cases for the replay validation system."""

    def setUp(self):
        """Set up test fixtures."""
        # Create a temporary directory for test files
        self.test_dir = tempfile.mkdtemp()
        
        # Create visualizer instance
        self.visualizer = ReplayVisualizer()
    
    def tearDown(self):
        """Tear down test fixtures."""
        # Remove temporary directory and its contents
        shutil.rmtree(self.test_dir)
    
    def test_empty_replay(self):
        """Test handling of empty replay files."""
        # Create empty replay files
        empty_replay_path = os.path.join(self.test_dir, "empty.jsonl")
        with open(empty_replay_path, 'w') as f:
            pass  # Create empty file
        
        # Load empty replay
        frames = self.visualizer.load_replay(empty_replay_path)
        
        # Verify result
        self.assertEqual(len(frames), 0, "Should handle empty replay file")
    
    def test_missing_replay(self):
        """Test handling of missing replay files."""
        # Try to load non-existent file
        missing_path = os.path.join(self.test_dir, "nonexistent.jsonl")
        frames = self.visualizer.load_replay(missing_path)
        
        # Verify result
        self.assertEqual(len(frames), 0, "Should handle missing replay file")
    
    def test_corrupt_replay(self):
        """Test handling of corrupt replay files."""
        # Create corrupt JSON file
        corrupt_path = os.path.join(self.test_dir, "corrupt.jsonl")
        with open(corrupt_path, 'w') as f:
            f.write('{"frame": 0}\n')            # Valid line
            f.write('{"frame": 1, "corrupt}\n')  # Invalid JSON
            f.write('{"frame": 2}\n')            # Valid line
        
        # Load corrupt replay
        frames = self.visualizer.load_replay(corrupt_path)
        
        # Verify result
        self.assertEqual(len(frames), 2, "Should load valid lines from corrupt file")
    
    def test_no_common_frames(self):
        """Test comparison with no common frame numbers."""
        # Create replays with different frame numbers
        replay1_path = os.path.join(self.test_dir, "replay1.jsonl")
        replay2_path = os.path.join(self.test_dir, "replay2.jsonl")
        
        with jsonlines.open(replay1_path, mode='w') as writer:
            writer.write_all([{"frame": i} for i in range(1, 10)])
        
        with jsonlines.open(replay2_path, mode='w') as writer:
            writer.write_all([{"frame": i + 100} for i in range(1, 10)])
        
        # Load replays
        frames1 = self.visualizer.load_replay(replay1_path)
        frames2 = self.visualizer.load_replay(replay2_path)
        
        # Find mismatches
        mismatches = self.visualizer.find_mismatches(frames1, frames2)
        
        # Verify result
        self.assertEqual(len(mismatches), 0, "Should find no mismatches when no common frames")
    
    def test_different_frame_order(self):
        """Test comparison with frames in different order."""
        # Create replays with frames in different order
        replay1_path = os.path.join(self.test_dir, "replay1.jsonl")
        replay2_path = os.path.join(self.test_dir, "replay2.jsonl")
        
        frames1 = [{"frame": i, "p1_health": 1.0} for i in range(10)]
        frames2 = [{"frame": i, "p1_health": 1.0} for i in range(10)]
        
        # Change order in second replay
        random.shuffle(frames2)
        
        with jsonlines.open(replay1_path, mode='w') as writer:
            writer.write_all(frames1)
        
        with jsonlines.open(replay2_path, mode='w') as writer:
            writer.write_all(frames2)
        
        # Load replays
        loaded_frames1 = self.visualizer.load_replay(replay1_path)
        loaded_frames2 = self.visualizer.load_replay(replay2_path)
        
        # Find mismatches
        mismatches = self.visualizer.find_mismatches(loaded_frames1, loaded_frames2)
        
        # Verify result
        self.assertEqual(len(mismatches), 0, "Should handle replays with frames in different order")


def run_tests():
    """Run the tests."""
    unittest.main()


if __name__ == "__main__":
    run_tests() 