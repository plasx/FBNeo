#!/usr/bin/env python3
"""
Determinism Debug Dashboard for FBNeo Replays

This module provides a real-time visualization dashboard for debugging
determinism issues in replay files.
"""

import os
import sys
import json
import time
import argparse
import logging
import threading
import webbrowser
from datetime import datetime
from flask import Flask, render_template, jsonify, request, send_from_directory
from flask_socketio import SocketIO
import jsonlines
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
import pandas as pd
import numpy as np
import csv
import zipfile
import io

# Set up logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)

# Initialize Flask application
app = Flask(__name__, 
           static_folder='dashboard_static',
           template_folder='dashboard_templates')
app.config['SECRET_KEY'] = 'fbneo-replay-dashboard-secret'

# Initialize Socket.IO
socketio = SocketIO(app, cors_allowed_origins="*")

# Global variables for state
active_replay_file = None
active_validation_file = None
replay_frames = []
validation_frames = []
mismatches = []
monitoring = False
file_handlers = {}

class ReplayFileHandler(FileSystemEventHandler):
    """Handler for monitoring changes to replay files."""
    
    def __init__(self, file_path, is_validation=False):
        super().__init__()
        self.file_path = file_path
        self.is_validation = is_validation
        self.last_position = 0
        
    def on_modified(self, event):
        """Handle file modification events."""
        if event.src_path == self.file_path:
            self.process_file_updates()
    
    def process_file_updates(self):
        """Process new content in the file."""
        global replay_frames, validation_frames, mismatches
        
        try:
            # Open file and seek to last position
            with open(self.file_path, 'r') as file:
                file.seek(self.last_position)
                new_lines = file.readlines()
                self.last_position = file.tell()
            
            if not new_lines:
                return
                
            # Parse new lines
            new_frames = []
            for line in new_lines:
                try:
                    frame = json.loads(line.strip())
                    new_frames.append(frame)
                except json.JSONDecodeError:
                    logging.warning(f"Invalid JSON in {self.file_path}: {line}")
            
            if not new_frames:
                return
                
            # Update appropriate frame list
            if self.is_validation:
                validation_frames.extend(new_frames)
                # Sort by frame number
                validation_frames.sort(key=lambda x: x.get('frame', 0))
                # Emit update
                socketio.emit('validation_frames', new_frames)
            else:
                replay_frames.extend(new_frames)
                # Sort by frame number
                replay_frames.sort(key=lambda x: x.get('frame', 0))
                # Emit update
                socketio.emit('replay_frames', new_frames)
            
            # Find new mismatches
            if self.is_validation:
                new_mismatches = find_mismatches(replay_frames, new_frames)
            else:
                new_mismatches = find_mismatches(new_frames, validation_frames)
                
            if new_mismatches:
                mismatches.extend(new_mismatches)
                # Sort by frame number
                mismatches.sort(key=lambda x: x.get('frame', 0))
                # Emit update
                socketio.emit('mismatches', new_mismatches)
                logging.info(f"Found {len(new_mismatches)} new mismatches")
                
        except Exception as e:
            logging.error(f"Error processing file {self.file_path}: {e}")

def start_monitoring():
    """Start monitoring replay files for changes."""
    global monitoring, file_handlers
    
    if monitoring:
        return True
        
    if not active_replay_file:
        return False
        
    try:
        # Setup file handlers
        file_handlers['replay'] = ReplayFileHandler(active_replay_file)
        
        if active_validation_file:
            file_handlers['validation'] = ReplayFileHandler(active_validation_file, is_validation=True)
        
        # Setup watchdog observer
        observer = Observer()
        
        # Monitor replay file
        replay_dir = os.path.dirname(active_replay_file)
        observer.schedule(file_handlers['replay'], replay_dir, recursive=False)
        
        # Monitor validation file if available
        if active_validation_file and 'validation' in file_handlers:
            validation_dir = os.path.dirname(active_validation_file)
            observer.schedule(file_handlers['validation'], validation_dir, recursive=False)
        
        # Start observer
        observer.start()
        file_handlers['observer'] = observer
        
        # Set initial file positions
        if os.path.exists(active_replay_file):
            file_handlers['replay'].last_position = os.path.getsize(active_replay_file)
            
        if active_validation_file and os.path.exists(active_validation_file):
            file_handlers['validation'].last_position = os.path.getsize(active_validation_file)
        
        monitoring = True
        logging.info("Started monitoring replay files")
        return True
        
    except Exception as e:
        logging.error(f"Error starting monitoring: {e}")
        return False

def stop_monitoring():
    """Stop monitoring replay files."""
    global monitoring, file_handlers
    
    if not monitoring:
        return True
        
    try:
        # Stop observer
        if 'observer' in file_handlers:
            file_handlers['observer'].stop()
            file_handlers['observer'].join()
            
        # Clear handlers
        file_handlers = {}
        
        monitoring = False
        logging.info("Stopped monitoring replay files")
        return True
        
    except Exception as e:
        logging.error(f"Error stopping monitoring: {e}")
        return False

def find_mismatches(replay_frames, validation_frames):
    """Find mismatches between replay and validation frames."""
    mismatches = []
    
    # Create dictionaries mapping frame numbers to indices
    replay_map = {f.get('frame'): i for i, f in enumerate(replay_frames) if 'frame' in f}
    validation_map = {f.get('frame'): i for i, f in enumerate(validation_frames) if 'frame' in f}
    
    # Find common frames
    common_frames = set(replay_map.keys()) & set(validation_map.keys())
    
    # Compare frames
    for frame_num in common_frames:
        r_idx = replay_map[frame_num]
        v_idx = validation_map[frame_num]
        
        r_frame = replay_frames[r_idx]
        v_frame = validation_frames[v_idx]
        
        differences = []
        
        # Compare all fields
        for key in set(r_frame.keys()) & set(v_frame.keys()):
            if key == 'frame':
                continue
                
            if r_frame[key] != v_frame[key]:
                differences.append({
                    'field': key,
                    'replay_value': r_frame[key],
                    'validation_value': v_frame[key]
                })
        
        if differences:
            mismatches.append({
                'frame': frame_num,
                'replay_idx': r_idx,
                'validation_idx': v_idx,
                'differences': differences
            })
    
    return mismatches

def reset_state():
    """Reset the state of the dashboard."""
    global replay_frames, validation_frames, mismatches
    
    replay_frames = []
    validation_frames = []
    mismatches = []

@app.route('/')
def index():
    """Render the main dashboard page."""
    return render_template('index.html')

@app.route('/api/status')
def get_status():
    """Get the current status of the dashboard."""
    return jsonify({
        'replay_file': active_replay_file,
        'validation_file': active_validation_file,
        'replay_frame_count': len(replay_frames),
        'validation_frame_count': len(validation_frames),
        'mismatch_count': len(mismatches),
        'monitoring': monitoring
    })

@app.route('/api/frames')
def get_frames():
    """Get all replay frames."""
    return jsonify(replay_frames)

@app.route('/api/frames/<int:frame_num>')
def get_frame(frame_num):
    """Get a specific frame."""
    for frame in replay_frames:
        if frame.get('frame') == frame_num:
            return jsonify(frame)
    
    return jsonify({'error': 'Frame not found'}), 404

@app.route('/api/validation_frames')
def get_validation_frames():
    """Get all validation frames."""
    return jsonify(validation_frames)

@app.route('/api/validation_frames/<int:frame_num>')
def get_validation_frame(frame_num):
    """Get a specific validation frame."""
    for frame in validation_frames:
        if frame.get('frame') == frame_num:
            return jsonify(frame)
    
    return jsonify({'error': 'Frame not found'}), 404

@app.route('/api/mismatches')
def get_mismatches():
    """Get all mismatches."""
    return jsonify(mismatches)

@app.route('/api/set_files', methods=['POST'])
def set_files():
    """Set the active replay and validation files."""
    global active_replay_file, active_validation_file
    
    # Get file paths from request
    data = request.json
    replay_file = data.get('replay_file')
    validation_file = data.get('validation_file')
    
    # Validate replay file
    if not replay_file or not os.path.exists(replay_file):
        return jsonify({'error': 'Invalid replay file'}), 400
    
    # Check validation file if provided
    if validation_file and not os.path.exists(validation_file):
        return jsonify({'error': 'Invalid validation file'}), 400
    
    # Stop monitoring if active
    if monitoring:
        stop_monitoring()
    
    # Reset state
    reset_state()
    
    # Set new files
    active_replay_file = replay_file
    active_validation_file = validation_file
    
    # Load initial data
    try:
        with jsonlines.open(replay_file) as reader:
            for line in reader:
                replay_frames.append(line)
        
        if validation_file:
            with jsonlines.open(validation_file) as reader:
                for line in reader:
                    validation_frames.append(line)
                    
            # Find mismatches
            mismatches.extend(find_mismatches(replay_frames, validation_frames))
    
    except Exception as e:
        logging.error(f"Error loading initial data: {e}")
        return jsonify({'error': f'Error loading data: {str(e)}'}), 500
    
    return jsonify({
        'replay_file': active_replay_file,
        'validation_file': active_validation_file,
        'replay_frame_count': len(replay_frames),
        'validation_frame_count': len(validation_frames),
        'mismatch_count': len(mismatches)
    })

@app.route('/api/start_monitoring', methods=['POST'])
def api_start_monitoring():
    """Start monitoring files for changes."""
    success = start_monitoring()
    
    if success:
        return jsonify({'status': 'monitoring_started'})
    else:
        return jsonify({'error': 'Failed to start monitoring'}), 500

@app.route('/api/stop_monitoring', methods=['POST'])
def api_stop_monitoring():
    """Stop monitoring files for changes."""
    success = stop_monitoring()
    
    if success:
        return jsonify({'status': 'monitoring_stopped'})
    else:
        return jsonify({'error': 'Failed to stop monitoring'}), 500

@app.route('/api/export/mismatches', methods=['GET'])
def export_mismatches():
    """Export mismatches to a JSON file."""
    if not mismatches:
        return jsonify({'error': 'No mismatches to export'}), 400
        
    try:
        # Create timestamp for filename
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"mismatches_{timestamp}.json"
        
        # Create export data
        export_data = {
            'timestamp': timestamp,
            'replay_file': active_replay_file,
            'validation_file': active_validation_file,
            'mismatches': mismatches
        }
        
        # Create file in memory
        json_data = json.dumps(export_data, indent=2)
        memory_file = io.BytesIO(json_data.encode())
        memory_file.seek(0)
        
        # Serve file
        return send_from_directory(
            directory=os.getcwd(),
            path=filename,
            as_attachment=True,
            download_name=filename,
            mimetype='application/json',
            etag=True,
            conditional=True,
            attachment_filename=filename  # For Flask < 2.0
        )
        
    except Exception as e:
        logging.error(f"Error exporting mismatches: {e}")
        return jsonify({'error': f'Error exporting mismatches: {str(e)}'}), 500

@app.route('/api/export/csv', methods=['GET'])
def export_csv():
    """Export frame data to CSV files."""
    if not replay_frames:
        return jsonify({'error': 'No data to export'}), 400
        
    try:
        # Create timestamp for filename
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"replay_data_{timestamp}.zip"
        
        # Create in-memory zip file
        memory_file = io.BytesIO()
        with zipfile.ZipFile(memory_file, 'w') as zf:
            # Export replay frames
            replay_csv = io.StringIO()
            if replay_frames:
                # Get all unique keys
                keys = set()
                for frame in replay_frames:
                    keys.update(frame.keys())
                
                # Create CSV writer
                writer = csv.DictWriter(replay_csv, fieldnames=sorted(keys))
                writer.writeheader()
                
                # Write all frames
                for frame in replay_frames:
                    writer.writerow(frame)
                
                # Add to zip
                zf.writestr('replay_frames.csv', replay_csv.getvalue())
            
            # Export validation frames
            if validation_frames:
                validation_csv = io.StringIO()
                
                # Get all unique keys
                keys = set()
                for frame in validation_frames:
                    keys.update(frame.keys())
                
                # Create CSV writer
                writer = csv.DictWriter(validation_csv, fieldnames=sorted(keys))
                writer.writeheader()
                
                # Write all frames
                for frame in validation_frames:
                    writer.writerow(frame)
                
                # Add to zip
                zf.writestr('validation_frames.csv', validation_csv.getvalue())
            
            # Export mismatches
            if mismatches:
                mismatches_json = json.dumps(mismatches, indent=2)
                zf.writestr('mismatches.json', mismatches_json)
        
        # Reset file pointer
        memory_file.seek(0)
        
        # Serve file
        return send_from_directory(
            directory=os.getcwd(),
            path=filename,
            as_attachment=True,
            download_name=filename,
            mimetype='application/zip',
            etag=True,
            conditional=True,
            attachment_filename=filename  # For Flask < 2.0
        )
        
    except Exception as e:
        logging.error(f"Error exporting CSV: {e}")
        return jsonify({'error': f'Error exporting CSV: {str(e)}'}), 500

@app.route('/api/export/analysis', methods=['GET'])
def export_analysis():
    """Export analysis of replay data."""
    if not replay_frames:
        return jsonify({'error': 'No data to analyze'}), 400
        
    try:
        # Create timestamp for filename
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"replay_analysis_{timestamp}.json"
        
        # Perform basic analysis
        analysis = analyze_replay_data()
        
        # Create export data
        export_data = {
            'timestamp': timestamp,
            'replay_file': active_replay_file,
            'validation_file': active_validation_file,
            'analysis': analysis
        }
        
        # Create file in memory
        json_data = json.dumps(export_data, indent=2)
        memory_file = io.BytesIO(json_data.encode())
        memory_file.seek(0)
        
        # Serve file
        return send_from_directory(
            directory=os.getcwd(),
            path=filename,
            as_attachment=True,
            download_name=filename,
            mimetype='application/json',
            etag=True,
            conditional=True,
            attachment_filename=filename  # For Flask < 2.0
        )
        
    except Exception as e:
        logging.error(f"Error exporting analysis: {e}")
        return jsonify({'error': f'Error exporting analysis: {str(e)}'}), 500

def analyze_replay_data():
    """Perform basic analysis of replay data."""
    analysis = {
        'replay_stats': {},
        'mismatch_stats': {},
        'time_series': {}
    }
    
    # Basic replay stats
    if replay_frames:
        # Frame count and range
        frame_numbers = [f.get('frame', 0) for f in replay_frames if 'frame' in f]
        analysis['replay_stats']['frame_count'] = len(replay_frames)
        analysis['replay_stats']['frame_range'] = {
            'min': min(frame_numbers) if frame_numbers else None,
            'max': max(frame_numbers) if frame_numbers else None
        }
        
        # Health stats if available
        p1_health = [f.get('p1_health') for f in replay_frames if 'p1_health' in f]
        p2_health = [f.get('p2_health') for f in replay_frames if 'p2_health' in f]
        
        if p1_health:
            analysis['replay_stats']['p1_health'] = {
                'min': min(p1_health),
                'max': max(p1_health),
                'mean': sum(p1_health) / len(p1_health)
            }
        
        if p2_health:
            analysis['replay_stats']['p2_health'] = {
                'min': min(p2_health),
                'max': max(p2_health),
                'mean': sum(p2_health) / len(p2_health)
            }
    
    # Mismatch stats
    if mismatches:
        # Count by field
        field_counts = {}
        for mismatch in mismatches:
            for diff in mismatch.get('differences', []):
                field = diff.get('field', 'unknown')
                field_counts[field] = field_counts.get(field, 0) + 1
        
        analysis['mismatch_stats']['count'] = len(mismatches)
        analysis['mismatch_stats']['field_counts'] = field_counts
        
        # Temporal distribution
        mismatch_frames = sorted(m.get('frame', 0) for m in mismatches)
        
        if len(mismatch_frames) > 1:
            intervals = [mismatch_frames[i] - mismatch_frames[i-1] for i in range(1, len(mismatch_frames))]
            
            analysis['mismatch_stats']['frame_intervals'] = {
                'min': min(intervals),
                'max': max(intervals),
                'mean': sum(intervals) / len(intervals),
                'median': sorted(intervals)[len(intervals) // 2]
            }
    
    # Time series data (for plotting)
    if replay_frames:
        # Extract time series
        time_series = {}
        
        # Health over time
        p1_health_series = [(f.get('frame', i), f.get('p1_health')) 
                           for i, f in enumerate(replay_frames) if 'p1_health' in f]
        p2_health_series = [(f.get('frame', i), f.get('p2_health')) 
                           for i, f in enumerate(replay_frames) if 'p2_health' in f]
        
        if p1_health_series:
            time_series['p1_health'] = p1_health_series
        
        if p2_health_series:
            time_series['p2_health'] = p2_health_series
        
        # Positions over time
        p1_x_series = [(f.get('frame', i), f.get('p1_x')) 
                      for i, f in enumerate(replay_frames) if 'p1_x' in f]
        p1_y_series = [(f.get('frame', i), f.get('p1_y')) 
                      for i, f in enumerate(replay_frames) if 'p1_y' in f]
        p2_x_series = [(f.get('frame', i), f.get('p2_x')) 
                      for i, f in enumerate(replay_frames) if 'p2_x' in f]
        p2_y_series = [(f.get('frame', i), f.get('p2_y')) 
                      for i, f in enumerate(replay_frames) if 'p2_y' in f]
        
        if p1_x_series:
            time_series['p1_x'] = p1_x_series
        
        if p1_y_series:
            time_series['p1_y'] = p1_y_series
        
        if p2_x_series:
            time_series['p2_x'] = p2_x_series
        
        if p2_y_series:
            time_series['p2_y'] = p2_y_series
        
        analysis['time_series'] = time_series
    
    return analysis

@app.route('/api/export/full_data', methods=['GET'])
def export_full_data():
    """Export all dashboard data and state."""
    if not replay_frames and not validation_frames:
        return jsonify({'error': 'No data to export'}), 400
        
    try:
        # Create timestamp for filename
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"dashboard_export_{timestamp}.zip"
        
        # Create in-memory zip file
        memory_file = io.BytesIO()
        with zipfile.ZipFile(memory_file, 'w') as zf:
            # Export replay frames
            if replay_frames:
                replay_json = json.dumps(replay_frames, indent=2)
                zf.writestr('replay_frames.json', replay_json)
            
            # Export validation frames
            if validation_frames:
                validation_json = json.dumps(validation_frames, indent=2)
                zf.writestr('validation_frames.json', validation_json)
            
            # Export mismatches
            if mismatches:
                mismatches_json = json.dumps(mismatches, indent=2)
                zf.writestr('mismatches.json', mismatches_json)
            
            # Export analysis
            analysis = analyze_replay_data()
            analysis_json = json.dumps(analysis, indent=2)
            zf.writestr('analysis.json', analysis_json)
            
            # Export metadata
            metadata = {
                'timestamp': timestamp,
                'replay_file': active_replay_file,
                'validation_file': active_validation_file,
                'replay_frame_count': len(replay_frames),
                'validation_frame_count': len(validation_frames),
                'mismatch_count': len(mismatches),
                'monitoring': monitoring
            }
            metadata_json = json.dumps(metadata, indent=2)
            zf.writestr('metadata.json', metadata_json)
        
        # Reset file pointer
        memory_file.seek(0)
        
        # Serve file
        return send_from_directory(
            directory=os.getcwd(),
            path=filename,
            as_attachment=True,
            download_name=filename,
            mimetype='application/zip',
            etag=True,
            conditional=True,
            attachment_filename=filename  # For Flask < 2.0
        )
        
    except Exception as e:
        logging.error(f"Error exporting full data: {e}")
        return jsonify({'error': f'Error exporting full data: {str(e)}'}), 500

@socketio.on('connect')
def handle_connect():
    """Handle client connection."""
    logging.info("Client connected")

@socketio.on('disconnect')
def handle_disconnect():
    """Handle client disconnection."""
    logging.info("Client disconnected")

def parse_args():
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description="FBNeo Determinism Debug Dashboard")
    parser.add_argument("--replay", help="Path to original replay file")
    parser.add_argument("--validation", help="Path to validation replay file")
    parser.add_argument("--port", type=int, default=5000, help="Port to run the server on")
    parser.add_argument("--browser", action="store_true", help="Open browser automatically")
    parser.add_argument("--monitor", action="store_true", help="Start monitoring files immediately")
    return parser.parse_args()

def main():
    """Main function to run the dashboard."""
    args = parse_args()
    
    # Set global file paths
    global active_replay_file, active_validation_file
    
    if args.replay:
        active_replay_file = os.path.abspath(args.replay)
        logging.info(f"Using replay file: {active_replay_file}")
    
    if args.validation:
        active_validation_file = os.path.abspath(args.validation)
        logging.info(f"Using validation file: {active_validation_file}")
    
    # Load initial data if files are provided
    if active_replay_file and os.path.exists(active_replay_file):
        try:
            # Load replay frames
            with jsonlines.open(active_replay_file) as reader:
                global replay_frames
                replay_frames = list(reader)
            logging.info(f"Loaded {len(replay_frames)} replay frames")
            
            # Load validation frames
            if active_validation_file and os.path.exists(active_validation_file):
                with jsonlines.open(active_validation_file) as reader:
                    global validation_frames
                    validation_frames = list(reader)
                logging.info(f"Loaded {len(validation_frames)} validation frames")
                
                # Find mismatches
                global mismatches
                mismatches = find_mismatches(replay_frames, validation_frames)
                logging.info(f"Found {len(mismatches)} mismatches")
        
        except Exception as e:
            logging.error(f"Error loading initial data: {e}")
    
    # Start monitoring if requested
    if args.monitor and active_replay_file:
        start_monitoring()
    
    # Open browser if requested
    if args.browser:
        url = f"http://localhost:{args.port}"
        threading.Timer(1.0, lambda: webbrowser.open(url)).start()
        logging.info(f"Opening browser at {url}")
    
    # Run the server
    logging.info(f"Starting dashboard server on port {args.port}")
    socketio.run(app, host="0.0.0.0", port=args.port, debug=False)

if __name__ == "__main__":
    main() 