#!/usr/bin/env python3
import argparse
import json
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.widgets import Slider, Button
from pathlib import Path
import sys
import subprocess
import os
import webbrowser
import time
import jsonlines
import logging
from collections import defaultdict


class ReplayVisualizer:
    """
    Visualizes FBNeo replay data from JSONL files, with ability to:
    - Display frame-by-frame playback
    - Highlight mismatch frames
    - Plot health, position, and other time-series data
    - Export animations of replay segments
    """
    
    def __init__(self, replay_file, validation_file=None, focus_frame=None):
        """
        Initialize the visualizer with replay data.
        
        Args:
            replay_file: Path to the .jsonl replay file
            validation_file: Optional path to a validation results file
            focus_frame: Optional frame number to focus visualization on
        """
        self.replay_file = Path(replay_file)
        self.validation_file = Path(validation_file) if validation_file else None
        self.focus_frame = focus_frame
        
        # Load replay data
        self.frames = self.load_replay(self.replay_file)
        print(f"Loaded {len(self.frames)} frames from {self.replay_file}")
        
        # Load validation data if provided
        self.validation_frames = None
        self.mismatches = []
        if self.validation_file and self.validation_file.exists():
            self.validation_frames = self.load_replay(self.validation_file)
            print(f"Loaded {len(self.validation_frames)} validation frames")
            self.mismatches = self.find_mismatches()
            print(f"Found {len(self.mismatches)} mismatched frames")
        
        # Set up time series data
        self.setup_time_series()
        
        # Initialize visualization objects
        self.fig = None
        self.axes = {}
        self.anim = None
        self.current_frame_idx = 0
        
        # Initialize stage boundaries (for position plotting)
        self.stage_bounds = {
            'x_min': 0,
            'x_max': 400,  # Typical stage width
            'y_min': 0,
            'y_max': 100   # Typical stage height
        }
    
    def load_replay(self, file_path):
        """Load replay data from a JSONL file."""
        frames = []
        try:
            with jsonlines.open(file_path) as reader:
                for line in reader:
                    frames.append(line)
        except Exception as e:
            logging.error(f"Error loading replay file {file_path}: {e}")
            return []

        # Sort frames by frame number if available
        if frames and 'frame' in frames[0]:
            frames.sort(key=lambda x: x.get('frame', 0))
        
        return frames
    
    def setup_time_series(self):
        """Extract time series data from frames for plotting."""
        frame_numbers = []
        p1_health = []
        p2_health = []
        p1_x = []
        p2_x = []
        hashes = []
        
        for frame in self.frames:
            frame_numbers.append(frame.get('frame', 0))
            p1_health.append(frame.get('p1_health', 0))
            p2_health.append(frame.get('p2_health', 0))
            p1_x.append(frame.get('p1_x', 0))
            p2_x.append(frame.get('p2_x', 0))
            hashes.append(frame.get('state_hash', ''))
        
        self.time_series = {
            'frame_numbers': np.array(frame_numbers),
            'p1_health': np.array(p1_health),
            'p2_health': np.array(p2_health),
            'p1_x': np.array(p1_x),
            'p2_x': np.array(p2_x),
            'hashes': hashes
        }
    
    def find_mismatches(self):
        """Find frames where replay and validation data don't match."""
        if not self.validation_frames:
            return []
        
        mismatches = []
        min_len = min(len(self.frames), len(self.validation_frames))
        
        for i in range(min_len):
            orig_frame = self.frames[i]
            val_frame = self.validation_frames[i]
            
            # Check if frame numbers match
            if orig_frame.get('frame') != val_frame.get('frame'):
                print(f"Warning: Frame number mismatch at index {i}: {orig_frame.get('frame')} vs {val_frame.get('frame')}")
                continue
            
            # Check for mismatches
            mismatch_types = []
            
            # Check state hash
            if orig_frame.get('state_hash') != val_frame.get('state_hash'):
                mismatch_types.append('hash')
            
            # Check health values
            if abs(orig_frame.get('p1_health', 0) - val_frame.get('p1_health', 0)) > 0.001:
                mismatch_types.append('p1_health')
            if abs(orig_frame.get('p2_health', 0) - val_frame.get('p2_health', 0)) > 0.001:
                mismatch_types.append('p2_health')
            
            # Check positions
            if abs(orig_frame.get('p1_x', 0) - val_frame.get('p1_x', 0)) > 0.001:
                mismatch_types.append('p1_x')
            if abs(orig_frame.get('p2_x', 0) - val_frame.get('p2_x', 0)) > 0.001:
                mismatch_types.append('p2_x')
            
            # Check inputs
            if orig_frame.get('inputs') != val_frame.get('inputs'):
                mismatch_types.append('inputs')
            
            # Check RNG seed
            if orig_frame.get('rng_seed') != val_frame.get('rng_seed'):
                mismatch_types.append('rng_seed')
            
            if mismatch_types:
                mismatches.append({
                    'frame': orig_frame.get('frame'),
                    'index': i,
                    'types': mismatch_types
                })
        
        return mismatches
    
    def decode_inputs(self, inputs_value):
        """
        Decode the inputs bitmask into a list of active inputs.
        Assuming inputs are stored as a bitmask where each bit represents a button.
        """
        if inputs_value is None:
            return []
        
        input_map = {
            0: 'UP',
            1: 'DOWN',
            2: 'LEFT',
            3: 'RIGHT',
            4: 'BUTTON1',
            5: 'BUTTON2',
            6: 'BUTTON3',
            7: 'BUTTON4',
            8: 'BUTTON5',
            9: 'BUTTON6',
            10: 'START',
            11: 'COIN'
        }
        
        active_inputs = []
        for bit, name in input_map.items():
            if inputs_value & (1 << bit):
                active_inputs.append(name)
        
        return active_inputs
    
    def create_visualization(self):
        """Create the main visualization figure with all subplots."""
        # Create figure with subplots
        self.fig = plt.figure(figsize=(12, 9), constrained_layout=True)
        gs = self.fig.add_gridspec(3, 3)
        
        # Stage view (top-left)
        self.axes['stage'] = self.fig.add_subplot(gs[0, 0:2])
        self.axes['stage'].set_title('Stage View')
        self.axes['stage'].set_xlim(self.stage_bounds['x_min'], self.stage_bounds['x_max'])
        self.axes['stage'].set_ylim(self.stage_bounds['y_min'], self.stage_bounds['y_max'])
        self.axes['stage'].set_xlabel('X Position')
        self.axes['stage'].set_aspect('equal')
        
        # Health bars (top-right)
        self.axes['health'] = self.fig.add_subplot(gs[0, 2])
        self.axes['health'].set_title('Health')
        self.axes['health'].set_xlim(0, 1)
        self.axes['health'].set_ylim(0, 1)
        self.axes['health'].set_xticks([])
        
        # Inputs display (middle-left)
        self.axes['inputs'] = self.fig.add_subplot(gs[1, 0:2])
        self.axes['inputs'].set_title('Inputs')
        self.axes['inputs'].set_xlim(0, 1)
        self.axes['inputs'].set_ylim(0, 1)
        self.axes['inputs'].set_xticks([])
        self.axes['inputs'].set_yticks([])
        
        # Frame hash (middle-right)
        self.axes['hash'] = self.fig.add_subplot(gs[1, 2])
        self.axes['hash'].set_title('Frame Hash')
        self.axes['hash'].set_xlim(0, 1)
        self.axes['hash'].set_ylim(0, 1)
        self.axes['hash'].set_xticks([])
        self.axes['hash'].set_yticks([])
        
        # Health over time (bottom-left)
        self.axes['health_time'] = self.fig.add_subplot(gs[2, 0])
        self.axes['health_time'].set_title('Health over Time')
        self.axes['health_time'].set_xlabel('Frame')
        self.axes['health_time'].set_ylabel('Health')
        self.axes['health_time'].set_ylim(0, 1)
        
        # Position over time (bottom-middle)
        self.axes['pos_time'] = self.fig.add_subplot(gs[2, 1])
        self.axes['pos_time'].set_title('Position over Time')
        self.axes['pos_time'].set_xlabel('Frame')
        self.axes['pos_time'].set_ylabel('X Position')
        
        # Frame timeline with mismatches (bottom-right)
        self.axes['timeline'] = self.fig.add_subplot(gs[2, 2])
        self.axes['timeline'].set_title('Frame Timeline')
        self.axes['timeline'].set_xlabel('Frame')
        self.axes['timeline'].set_yticks([])
        
        # Plot health and position over time
        self.axes['health_time'].plot(self.time_series['frame_numbers'], self.time_series['p1_health'], 'b-', label='P1')
        self.axes['health_time'].plot(self.time_series['frame_numbers'], self.time_series['p2_health'], 'r-', label='P2')
        self.axes['health_time'].legend()
        
        self.axes['pos_time'].plot(self.time_series['frame_numbers'], self.time_series['p1_x'], 'b-', label='P1')
        self.axes['pos_time'].plot(self.time_series['frame_numbers'], self.time_series['p2_x'], 'r-', label='P2')
        self.axes['pos_time'].legend()
        
        # Highlight mismatches on timeline
        if self.mismatches:
            mismatch_frames = [m['frame'] for m in self.mismatches]
            self.axes['timeline'].vlines(mismatch_frames, 0, 1, color='red', alpha=0.7, label='Mismatches')
            self.axes['timeline'].legend()
        
        # Add frame marker on timelines
        self.frame_marker_health = self.axes['health_time'].axvline(0, color='g', linestyle='-')
        self.frame_marker_pos = self.axes['pos_time'].axvline(0, color='g', linestyle='-')
        self.frame_marker_timeline = self.axes['timeline'].axvline(0, color='g', linestyle='-')
        
        # Add slider for frame navigation
        plt.subplots_adjust(bottom=0.15)
        self.slider_ax = plt.axes([0.15, 0.05, 0.7, 0.03])
        max_frame = len(self.frames) - 1
        self.frame_slider = Slider(
            self.slider_ax, 'Frame', 0, max_frame,
            valinit=0, valstep=1, valfmt='%0.0f'
        )
        self.frame_slider.on_changed(self.update_frame_from_slider)
        
        # Add navigation buttons
        prev_button_ax = plt.axes([0.05, 0.05, 0.06, 0.03])
        next_button_ax = plt.axes([0.89, 0.05, 0.06, 0.03])
        mismatch_button_ax = plt.axes([0.45, 0.01, 0.1, 0.03])
        
        self.prev_button = Button(prev_button_ax, 'Prev')
        self.next_button = Button(next_button_ax, 'Next')
        self.mismatch_button = Button(mismatch_button_ax, 'Jump to Mismatch')
        
        self.prev_button.on_clicked(self.prev_frame)
        self.next_button.on_clicked(self.next_frame)
        self.mismatch_button.on_clicked(self.jump_to_mismatch)
        
        # Initialize with first frame
        self.update_plot(0)
        
        # Set window title
        self.fig.canvas.manager.set_window_title(f"Replay Visualizer: {self.replay_file.name}")
        
        # If focus frame is specified, jump to it
        if self.focus_frame is not None:
            # Find the closest frame
            frame_idx = np.argmin(np.abs(self.time_series['frame_numbers'] - self.focus_frame))
            self.update_plot(frame_idx)
            self.frame_slider.set_val(frame_idx)
    
    def update_plot(self, frame_idx):
        """Update all plot elements to show the data for the given frame index."""
        if frame_idx < 0 or frame_idx >= len(self.frames):
            return
        
        self.current_frame_idx = frame_idx
        frame = self.frames[frame_idx]
        frame_num = frame.get('frame', 0)
        
        # Update stage view (positions)
        self.axes['stage'].clear()
        self.axes['stage'].set_title(f'Stage View - Frame {frame_num}')
        self.axes['stage'].set_xlim(self.stage_bounds['x_min'], self.stage_bounds['x_max'])
        self.axes['stage'].set_ylim(self.stage_bounds['y_min'], self.stage_bounds['y_max'])
        
        p1_x = frame.get('p1_x', 0)
        p2_x = frame.get('p2_x', 0)
        
        # Draw players as circles
        p1_circle = plt.Circle((p1_x, 50), 10, color='blue', alpha=0.7, label='P1')
        p2_circle = plt.Circle((p2_x, 50), 10, color='red', alpha=0.7, label='P2')
        self.axes['stage'].add_patch(p1_circle)
        self.axes['stage'].add_patch(p2_circle)
        self.axes['stage'].legend()
        
        # Update health bars
        self.axes['health'].clear()
        self.axes['health'].set_title('Health')
        self.axes['health'].set_xlim(0, 1)
        self.axes['health'].set_ylim(0, 2)
        self.axes['health'].set_xticks([])
        
        p1_health = frame.get('p1_health', 0)
        p2_health = frame.get('p2_health', 0)
        
        self.axes['health'].barh(0.5, p1_health, height=0.3, color='blue', label='P1')
        self.axes['health'].barh(1.5, p2_health, height=0.3, color='red', label='P2')
        self.axes['health'].text(0.05, 0.5, f"{p1_health:.2f}", va='center')
        self.axes['health'].text(0.05, 1.5, f"{p2_health:.2f}", va='center')
        self.axes['health'].legend()
        
        # Update inputs display
        self.axes['inputs'].clear()
        self.axes['inputs'].set_title('Inputs')
        self.axes['inputs'].set_xlim(0, 1)
        self.axes['inputs'].set_ylim(0, 1)
        self.axes['inputs'].set_xticks([])
        self.axes['inputs'].set_yticks([])
        
        inputs_value = frame.get('inputs', 0)
        active_inputs = self.decode_inputs(inputs_value)
        inputs_text = ", ".join(active_inputs) if active_inputs else "None"
        self.axes['inputs'].text(0.5, 0.5, inputs_text, ha='center', va='center')
        
        # Update hash display
        self.axes['hash'].clear()
        self.axes['hash'].set_title('Frame Hash')
        self.axes['hash'].set_xlim(0, 1)
        self.axes['hash'].set_ylim(0, 1)
        self.axes['hash'].set_xticks([])
        self.axes['hash'].set_yticks([])
        
        state_hash = frame.get('state_hash', 'Unknown')
        
        # Check if this frame has a mismatch
        is_mismatch = False
        if self.validation_frames and frame_idx < len(self.validation_frames):
            val_frame = self.validation_frames[frame_idx]
            if val_frame.get('state_hash') != state_hash:
                is_mismatch = True
                self.axes['hash'].set_facecolor('#ffdddd')  # Light red background for mismatch
                self.axes['hash'].text(0.5, 0.7, f"Original: {state_hash}", ha='center', va='center')
                self.axes['hash'].text(0.5, 0.3, f"Validation: {val_frame.get('state_hash')}", ha='center', va='center')
            else:
                self.axes['hash'].text(0.5, 0.5, state_hash, ha='center', va='center')
                self.axes['hash'].text(0.5, 0.3, "MATCH ✓", ha='center', va='center', color='green')
        else:
            self.axes['hash'].text(0.5, 0.5, state_hash, ha='center', va='center')
        
        # Update frame markers on timelines
        self.frame_marker_health.set_xdata(frame_num)
        self.frame_marker_pos.set_xdata(frame_num)
        self.frame_marker_timeline.set_xdata(frame_num)
        
        # Highlight current frame
        if is_mismatch:
            for marker in [self.frame_marker_health, self.frame_marker_pos, self.frame_marker_timeline]:
                marker.set_color('red')
                marker.set_linewidth(2)
        else:
            for marker in [self.frame_marker_health, self.frame_marker_pos, self.frame_marker_timeline]:
                marker.set_color('green')
                marker.set_linewidth(1)
        
        self.fig.canvas.draw_idle()
    
    def update_frame_from_slider(self, val):
        """Callback for slider movement."""
        frame_idx = int(val)
        self.update_plot(frame_idx)
    
    def prev_frame(self, event):
        """Go to previous frame."""
        self.update_plot(self.current_frame_idx - 1)
        self.frame_slider.set_val(self.current_frame_idx)
    
    def next_frame(self, event):
        """Go to next frame."""
        self.update_plot(self.current_frame_idx + 1)
        self.frame_slider.set_val(self.current_frame_idx)
    
    def jump_to_mismatch(self, event):
        """Jump to the next mismatch frame after the current frame."""
        if not self.mismatches:
            return
        
        current_frame = self.frames[self.current_frame_idx].get('frame', 0)
        next_mismatch = None
        
        for mismatch in self.mismatches:
            if mismatch['frame'] > current_frame:
                next_mismatch = mismatch
                break
        
        if next_mismatch is None and self.mismatches:  # Wrap around
            next_mismatch = self.mismatches[0]
        
        if next_mismatch:
            # Find the index for this frame
            idx = next(
                (i for i, f in enumerate(self.frames) if f.get('frame') == next_mismatch['frame']),
                0
            )
            self.update_plot(idx)
            self.frame_slider.set_val(idx)
    
    def create_animation(self, output_file=None, start_frame=None, end_frame=None, fps=30):
        """
        Create an animation of the replay.
        
        Args:
            output_file: Path to save the animation (must end with .mp4, .gif, etc.)
            start_frame: First frame to include in animation
            end_frame: Last frame to include in animation
            fps: Frames per second for the animation
        """
        if not output_file:
            output_file = self.replay_file.with_suffix('.mp4')
        
        # Create figure if not already created
        if self.fig is None:
            self.create_visualization()
        
        # Determine frame range
        if self.mismatches and (start_frame is None or end_frame is None):
            # Focus on a mismatch with context
            mismatch_idx = 0  # Use first mismatch by default
            if self.focus_frame is not None:
                # Find mismatch closest to focus frame
                closest_mismatch = min(
                    self.mismatches,
                    key=lambda m: abs(m['frame'] - self.focus_frame)
                )
                mismatch_idx = next(
                    (i for i, f in enumerate(self.frames) if f.get('frame') == closest_mismatch['frame']),
                    0
                )
            
            # Use 10 frames before and after the mismatch
            start_frame = max(0, mismatch_idx - 10)
            end_frame = min(len(self.frames) - 1, mismatch_idx + 10)
        
        start_frame = 0 if start_frame is None else start_frame
        end_frame = len(self.frames) - 1 if end_frame is None else end_frame
        
        frame_range = range(start_frame, end_frame + 1)
        
        # Create animation
        self.anim = animation.FuncAnimation(
            self.fig, 
            self.update_plot, 
            frames=frame_range,
            interval=1000/fps,
            blit=False
        )
        
        # Save animation
        writer = animation.FFMpegWriter(fps=fps)
        self.anim.save(output_file, writer=writer)
        print(f"Animation saved to {output_file}")
    
    def show(self):
        """Display the interactive visualization."""
        if self.fig is None:
            self.create_visualization()
        plt.show()


def start_dashboard(replay_file=None, validation_file=None, port=5000, browser=True):
    """
    Start the determinism debug dashboard.
    
    Args:
        replay_file: Path to the replay file
        validation_file: Path to the validation file
        port: Port to run the dashboard on
        browser: Whether to open a browser window
    """
    script_dir = Path(__file__).parent.absolute()
    dashboard_script = script_dir / 'debug_dashboard.py'
    
    # Build command
    cmd = [sys.executable, str(dashboard_script)]
    
    if replay_file:
        cmd.extend(['--replay', str(replay_file)])
    
    if validation_file:
        cmd.extend(['--validation', str(validation_file)])
    
    cmd.extend(['--port', str(port)])
    cmd.extend(['--monitor'])
    
    if browser:
        cmd.extend(['--browser'])
    
    # Start dashboard process
    print(f"Starting debug dashboard on port {port}...")
    process = subprocess.Popen(cmd, 
                               stdout=subprocess.PIPE, 
                               stderr=subprocess.PIPE, 
                               text=True)
    
    # Give it a moment to start
    time.sleep(1)
    
    if browser and not '--browser' in cmd:
        webbrowser.open(f'http://localhost:{port}')
    
    return process


def main():
    parser = argparse.ArgumentParser(description='FBNeo AI Replay Visualizer')
    parser.add_argument('--input', required=True, help='Input replay JSONL file')
    parser.add_argument('--validation', help='Validation replay JSONL file for comparison')
    parser.add_argument('--focus-frame', type=int, help='Frame number to focus on')
    parser.add_argument('--output', help='Output file for animation export (.mp4, .gif)')
    parser.add_argument('--export-only', action='store_true', help='Export animation without displaying UI')
    parser.add_argument('--start-frame', type=int, help='Start frame for animation export')
    parser.add_argument('--end-frame', type=int, help='End frame for animation export')
    parser.add_argument('--fps', type=int, default=30, help='Frames per second for animation export')
    parser.add_argument('--dashboard', action='store_true', help='Launch the determinism debug dashboard instead of the static visualizer')
    parser.add_argument('--port', type=int, default=5000, help='Port for the debug dashboard (if --dashboard is used)')
    parser.add_argument('--no-browser', action='store_true', help="Don't open a browser window for the dashboard")
    
    args = parser.parse_args()
    
    # Check if input file exists
    if not Path(args.input).exists():
        print(f"Error: Input file {args.input} not found")
        return 1
    
    # If dashboard mode is selected, start the dashboard instead
    if args.dashboard:
        dashboard_process = start_dashboard(
            replay_file=args.input,
            validation_file=args.validation,
            port=args.port,
            browser=not args.no_browser
        )
        
        try:
            # Keep the dashboard running until Ctrl+C
            print("Dashboard is running. Press Ctrl+C to stop.")
            dashboard_process.wait()
        except KeyboardInterrupt:
            print("Stopping dashboard...")
            dashboard_process.terminate()
        
        return dashboard_process.returncode
    
    # Otherwise, use the static visualizer
    visualizer = ReplayVisualizer(
        replay_file=args.input,
        validation_file=args.validation,
        focus_frame=args.focus_frame
    )
    
    # Export animation if requested
    if args.output:
        visualizer.create_animation(
            output_file=args.output,
            start_frame=args.start_frame,
            end_frame=args.end_frame,
            fps=args.fps
        )
    
    # Show interactive visualization unless export-only
    if not args.export_only:
        visualizer.show()
    
    return 0


if __name__ == "__main__":
    sys.exit(main()) 