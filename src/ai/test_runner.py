#!/usr/bin/env python3
"""
FBNeo Memory Mapping Test Runner
This script executes test cases against FBNeo memory mappings to validate mapping correctness.
"""

import argparse
import json
import os
import sys
import time
import logging
from datetime import datetime
from pathlib import Path

# Try to import the AIMemoryMapping class from the ai module
try:
    sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    from ai.ai_memory_mapping import AIMemoryMapping
except ImportError:
    logging.error("Could not import AIMemoryMapping. Make sure you're running from the FBNeo root directory.")
    sys.exit(1)

class TestRunner:
    """Executes test cases against FBNeo memory mappings"""
    
    def __init__(self, test_file=None, mapping_file=None, fbneo_path=None, verbose=False):
        """Initialize the test runner"""
        self.test_file = test_file
        self.mapping_file = mapping_file
        self.fbneo_path = fbneo_path
        self.verbose = verbose
        self.test_cases = []
        self.results = {}
        self.passed = 0
        self.failed = 0
        self.memory_mapping = None
        
        # Configure logging
        log_level = logging.DEBUG if verbose else logging.INFO
        logging.basicConfig(
            level=log_level,
            format='%(asctime)s - %(levelname)s - %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S'
        )
        
    def find_fbneo_executable(self):
        """Find the FBNeo executable"""
        # Default paths to check
        possible_paths = [
            self.fbneo_path,
            "./fbneo",
            "./fbneo.exe",
            "./FinalBurn Neo",
            "./FinalBurn Neo.exe",
            "./fbneo/fbneo",
            "./fbneo/fbneo.exe",
            "/Applications/FinalBurn Neo.app/Contents/MacOS/FinalBurn Neo",
            "C:\\Program Files\\FinalBurn Neo\\fbneo.exe",
            "C:\\Program Files (x86)\\FinalBurn Neo\\fbneo.exe"
        ]
        
        for path in possible_paths:
            if path and os.path.isfile(path) and os.access(path, os.X_OK):
                return path
        
        logging.error("Could not find FBNeo executable. Please specify with --fbneo-path")
        return None
        
    def load_test_file(self):
        """Load the test file"""
        if not self.test_file:
            logging.error("No test file specified")
            return False
            
        try:
            with open(self.test_file, 'r') as f:
                test_data = json.load(f)
                
            self.test_cases = test_data.get('test_cases', [])
            
            # If mapping file not specified via command line, use the one in the test file
            if not self.mapping_file and 'mapping_file' in test_data:
                self.mapping_file = test_data['mapping_file']
                
            self.game = test_data.get('game', '')
            self.architecture = test_data.get('architecture', '')
            
            logging.info(f"Loaded {len(self.test_cases)} test cases for {self.game} ({self.architecture})")
            return True
        except (json.JSONDecodeError, FileNotFoundError) as e:
            logging.error(f"Error loading test file: {e}")
            return False
            
    def initialize_memory_mapping(self):
        """Initialize memory mapping"""
        if not self.mapping_file:
            logging.error("No mapping file specified")
            return False
            
        try:
            self.memory_mapping = AIMemoryMapping()
            success = self.memory_mapping.loadFromFile(self.mapping_file)
            
            if not success:
                logging.error(f"Failed to load mapping file: {self.mapping_file}")
                return False
                
            logging.info(f"Successfully loaded mapping file: {self.mapping_file}")
            return True
        except Exception as e:
            logging.error(f"Error initializing memory mapping: {e}")
            return False
            
    def run_manual_tests(self):
        """Run tests manually with user verification"""
        if not self.test_cases:
            logging.error("No test cases to run")
            return False
            
        for i, test_case in enumerate(self.test_cases):
            test_name = test_case.get('name', f'Test {i+1}')
            description = test_case.get('description', 'No description')
            setup_steps = test_case.get('setup_steps', [])
            expected_values = test_case.get('expected_values', {})
            
            print("\n" + "="*80)
            print(f"Test Case: {test_name}")
            print(f"Description: {description}")
            print("\nSetup Steps:")
            for j, step in enumerate(setup_steps):
                print(f"  {j+1}. {step}")
                
            input("\nPerform the setup steps above, then press Enter to verify the expected values...")
            
            # Match expected values against current memory values
            results = {}
            all_passed = True
            
            for address, expected_value in expected_values.items():
                try:
                    # Convert address string to int if needed
                    addr = int(address, 16) if isinstance(address, str) else address
                    
                    # Get memory value based on the mapping
                    actual_value = None
                    
                    # First try to find a mapping with this address
                    for mapping in self.memory_mapping.mappings:
                        if mapping.get('address') == address:
                            mapping_name = mapping.get('name')
                            actual_value = self.memory_mapping.getValue(mapping_name)
                            break
                    
                    # If no mapping found, try to read memory directly
                    if actual_value is None:
                        # This is a placeholder for direct memory read
                        # In a real implementation, we would use the memory read function
                        # based on the architecture
                        logging.warning(f"No mapping found for address {address}, skipping")
                        continue
                        
                    passed = actual_value == expected_value
                    results[address] = {
                        'expected': expected_value,
                        'actual': actual_value,
                        'passed': passed
                    }
                    
                    if not passed:
                        all_passed = False
                        
                except Exception as e:
                    logging.error(f"Error checking value at {address}: {e}")
                    results[address] = {
                        'expected': expected_value,
                        'actual': 'ERROR',
                        'passed': False
                    }
                    all_passed = False
            
            # Display results
            print("\nResults:")
            for address, result in results.items():
                status = "✓ PASS" if result['passed'] else "✗ FAIL"
                print(f"  {address}: Expected={result['expected']}, Actual={result['actual']} - {status}")
                
            self.results[test_name] = {
                'passed': all_passed,
                'details': results
            }
            
            if all_passed:
                self.passed += 1
                print(f"\nTest {test_name} PASSED")
            else:
                self.failed += 1
                print(f"\nTest {test_name} FAILED")
                
            continue_testing = input("\nContinue to next test? (y/n): ").lower() == 'y'
            if not continue_testing:
                break
                
        return True
        
    def generate_report(self):
        """Generate a test report"""
        total = self.passed + self.failed
        
        print("\n" + "="*80)
        print(f"Test Report: {self.test_file}")
        print(f"Game: {self.game} ({self.architecture})")
        print(f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"Total Tests: {total}")
        print(f"Passed: {self.passed} ({self.passed/total*100:.1f}%)")
        print(f"Failed: {self.failed} ({self.failed/total*100:.1f}%)")
        print("="*80)
        
        # Generate a more detailed report
        report_path = f"test_report_{self.game}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        try:
            with open(report_path, 'w') as f:
                json.dump({
                    'test_file': self.test_file,
                    'mapping_file': self.mapping_file,
                    'game': self.game,
                    'architecture': self.architecture,
                    'date': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                    'total_tests': total,
                    'passed': self.passed,
                    'failed': self.failed,
                    'results': self.results
                }, f, indent=2)
                
            logging.info(f"Detailed report saved to {report_path}")
        except Exception as e:
            logging.error(f"Error saving report: {e}")
            
        return total > 0 and self.failed == 0
        
    def run(self):
        """Run the test suite"""
        if not self.load_test_file():
            return False
            
        if not self.initialize_memory_mapping():
            return False
            
        if not self.find_fbneo_executable():
            logging.warning("FBNeo executable not found. Tests will rely on manual memory value verification.")
            
        if not self.run_manual_tests():
            return False
            
        return self.generate_report()
        

def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description='FBNeo Memory Mapping Test Runner')
    parser.add_argument('test_file', help='Path to the test case file')
    parser.add_argument('--mapping-file', help='Path to the memory mapping file (overrides the one in test file)')
    parser.add_argument('--fbneo-path', help='Path to the FBNeo executable')
    parser.add_argument('--verbose', '-v', action='store_true', help='Enable verbose logging')
    
    args = parser.parse_args()
    
    runner = TestRunner(
        test_file=args.test_file,
        mapping_file=args.mapping_file,
        fbneo_path=args.fbneo_path,
        verbose=args.verbose
    )
    
    success = runner.run()
    sys.exit(0 if success else 1)
    

if __name__ == "__main__":
    main() 