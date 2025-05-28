#!/usr/bin/env python3
"""
Run FBNeo Metal with forced unbuffered output and proper terminal display of debug info.
This script uses PTY (pseudo-terminal) to ensure output from GUI apps doesn't get buffered.
"""

import os
import sys
import time
import argparse
import pty
import subprocess
import select
import signal
import fcntl
import termios
import struct
import io

def get_terminal_size():
    """Get terminal dimensions."""
    try:
        with open(os.ctermid(), 'rb') as fd:
            dimensions = struct.unpack('hh', fcntl.ioctl(fd, termios.TIOCGWINSZ, b'1234'))
        return dimensions
    except:
        return (24, 80)  # Default fallback

def run_emulator_with_pty(rom_path):
    """Run the emulator through a PTY to force unbuffered output."""
    rows, cols = get_terminal_size()
    
    # Create timestamp for log file
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    log_dir = "/tmp/fbneo_logs"
    os.makedirs(log_dir, exist_ok=True)
    log_file = f"{log_dir}/fbneo_metal_{timestamp}.log"

    # Print banner
    print(f"\033[32m{'=' * cols}\033[0m")
    print(f"\033[32mRunning FBNeo Metal with debug output\033[0m")
    print(f"\033[32mROM: {rom_path}\033[0m")
    print(f"\033[32mLog file: {log_file}\033[0m")
    print(f"\033[32m{'=' * cols}\033[0m")
    
    # Create PTY
    master_fd, slave_fd = pty.openpty()
    
    # Set PTY to raw mode to avoid interpreting control characters
    term_attrs = termios.tcgetattr(master_fd)
    term_attrs[0] &= ~(termios.INLCR | termios.ICRNL | termios.IGNCR)
    term_attrs[1] &= ~termios.OCRNL
    termios.tcsetattr(master_fd, termios.TCSANOW, term_attrs)
    
    # Set environment variables to force unbuffered output
    env = os.environ.copy()
    env["FBNEO_DEBUG"] = "1"
    env["METAL_DEBUG"] = "1"
    env["FBNEO_ENHANCED_DEBUG"] = "1"
    env["PYTHONUNBUFFERED"] = "1"
    env["STDBUF"] = "0"
    
    # Open log file
    log_fd = open(log_file, "w")
    
    # Launch emulator
    cmd = ["./fbneo_metal", rom_path]
    process = subprocess.Popen(
        cmd,
        stdin=slave_fd,
        stdout=slave_fd,
        stderr=slave_fd,
        env=env,
        close_fds=True,
        preexec_fn=os.setsid  # Create new process group
    )
    
    # Close the slave end of the PTY in the parent process
    os.close(slave_fd)
    
    # Set up polling
    poll_obj = select.poll()
    poll_obj.register(master_fd, select.POLLIN)
    
    # Set non-blocking mode for master_fd
    flags = fcntl.fcntl(master_fd, fcntl.F_GETFL)
    fcntl.fcntl(master_fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)
    
    # Buffer for incomplete lines
    line_buffer = ""
    
    try:
        while True:
            # Check if the process is still running
            if process.poll() is not None:
                print(f"\033[33mProcess exited with code {process.returncode}\033[0m")
                break
            
            # Poll for output with a timeout
            events = poll_obj.poll(100)  # 100ms timeout
            
            if events:
                try:
                    data = os.read(master_fd, 1024)
                    if data:
                        # Convert bytes to string
                        text = data.decode('utf-8', errors='replace')
                        
                        # Buffer incomplete lines
                        line_buffer += text
                        lines = line_buffer.split('\n')
                        
                        # Process complete lines
                        if len(lines) > 1:
                            for line in lines[:-1]:
                                # Print to terminal
                                print(line)
                                sys.stdout.flush()
                                
                                # Write to log file
                                log_fd.write(line + '\n')
                                log_fd.flush()
                            
                            # Keep the last incomplete line in buffer
                            line_buffer = lines[-1]
                    else:
                        # EOF reached
                        break
                except OSError as e:
                    if e.errno != 11:  # EAGAIN
                        raise
            
            # Small sleep to avoid CPU spinning
            time.sleep(0.01)
            
    except KeyboardInterrupt:
        print("\033[33mReceived keyboard interrupt, terminating emulator...\033[0m")
        try:
            # Send SIGTERM to the process group
            os.killpg(os.getpgid(process.pid), signal.SIGTERM)
            # Wait for process to terminate
            process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            # Force kill if not responding
            print("\033[33mProcess not responding, force killing...\033[0m")
            os.killpg(os.getpgid(process.pid), signal.SIGKILL)
    
    finally:
        # Clean up
        os.close(master_fd)
        log_fd.close()
        
        # Show log file information
        print(f"\033[32mLog file saved to: {log_file}\033[0m")
        print(f"\033[32mTo view: less {log_file}\033[0m")

def main():
    parser = argparse.ArgumentParser(description="Run FBNeo Metal with debug output")
    parser.add_argument("rom_path", nargs="?", default="/Users/plasx/dev/ROMs/mvsc.zip", 
                      help="Path to ROM file (default: /Users/plasx/dev/ROMs/mvsc.zip)")
    args = parser.parse_args()
    
    run_emulator_with_pty(args.rom_path)

if __name__ == "__main__":
    main() 