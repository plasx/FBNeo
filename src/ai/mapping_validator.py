#!/usr/bin/env python3
"""
Memory Mapping Validator Tool

This tool validates memory mapping JSON files against the schema
and performs additional custom validation checks to ensure the
mappings will work correctly with FBNeo's AI system.
"""

import os
import sys
import json
import glob
import argparse
from typing import Dict, List, Tuple, Set, Optional, Any


class MappingValidator:
    """Validator for FBNeo memory mapping files"""

    def __init__(self, schema_path: str = "src/ai/memory_mapping_schema.json"):
        """Initialize the validator with the schema path"""
        self.schema_path = schema_path
        self.schema = self._load_schema(schema_path)
        self.verbose = False
        self.errors = []
        self.warnings = []

    def _load_schema(self, schema_path: str) -> dict:
        """Load the JSON schema from the specified path"""
        # Try several potential paths if the specified one doesn't exist
        potential_paths = [
            schema_path,
            os.path.join("src/ai", os.path.basename(schema_path)),
            os.path.join("..", os.path.basename(schema_path)),
            os.path.join("mappings", os.path.basename(schema_path)),
            "memory_mapping_schema.json",
        ]

        for path in potential_paths:
            if os.path.exists(path):
                try:
                    with open(path, "r") as f:
                        return json.load(f)
                except json.JSONDecodeError as e:
                    print(f"Error parsing schema file: {e}")
                    sys.exit(1)
                except Exception as e:
                    print(f"Error loading schema file: {e}")
                    sys.exit(1)

        print(f"Error: Schema file not found. Tried paths: {potential_paths}")
        sys.exit(1)

    def validate(self, mapping_file: str) -> Tuple[bool, List[str], List[str]]:
        """Validate a mapping file against the schema and custom rules"""
        self.errors = []
        self.warnings = []

        # Check if file exists
        if not os.path.exists(mapping_file):
            self.errors.append(f"File not found: {mapping_file}")
            return False, self.errors, self.warnings

        # Load mapping file
        try:
            with open(mapping_file, "r") as f:
                mapping_data = json.load(f)
        except json.JSONDecodeError as e:
            self.errors.append(f"Invalid JSON in {mapping_file}: {e}")
            return False, self.errors, self.warnings
        except Exception as e:
            self.errors.append(f"Error reading {mapping_file}: {e}")
            return False, self.errors, self.warnings

        # Validate required fields
        for required_field in self.schema.get("required", []):
            if required_field not in mapping_data:
                self.errors.append(f"Missing required field: {required_field}")

        # Validate game name - should not be empty
        if "game_name" in mapping_data and not mapping_data["game_name"].strip():
            self.errors.append("Game name cannot be empty")

        # Validate architecture - should be one of the allowed values
        allowed_architectures = self.schema.get("properties", {}).get("architecture", {}).get("enum", [])
        if "architecture" in mapping_data and mapping_data["architecture"] not in allowed_architectures:
            self.errors.append(f"Invalid architecture: {mapping_data['architecture']}. Must be one of: {', '.join(allowed_architectures)}")

        # Validate mappings
        if "mappings" in mapping_data:
            mappings_valid = self._validate_mapping_entries(mapping_data["mappings"])
            if not mappings_valid:
                self.errors.append("Mappings validation failed - see specific errors above")

        # Validate groups
        if "groups" in mapping_data:
            groups_valid = self._validate_groups(mapping_data)
            if not groups_valid:
                self.errors.append("Groups validation failed - see specific errors above")

        # Return validation result
        is_valid = len(self.errors) == 0
        return is_valid, self.errors, self.warnings

    def _validate_mapping_entries(self, mappings: Dict[str, List[Dict[str, Any]]]) -> bool:
        """Validate all mapping entries in all categories"""
        is_valid = True
        mapping_names = set()

        for category, entries in mappings.items():
            if not isinstance(entries, list):
                self.errors.append(f"Category '{category}' must contain a list of mapping entries")
                is_valid = False
                continue

            for entry in entries:
                # Check required fields
                for field in ["name", "address", "type"]:
                    if field not in entry:
                        self.errors.append(f"Mapping in category '{category}' missing required field: {field}")
                        is_valid = False

                # Check for duplicate names
                if "name" in entry:
                    name = entry["name"]
                    if name in mapping_names:
                        self.errors.append(f"Duplicate mapping name: {name}")
                        is_valid = False
                    mapping_names.add(name)

                # Validate address format
                if "address" in entry:
                    address = entry["address"]
                    if not isinstance(address, str):
                        self.errors.append(f"Address in mapping '{entry.get('name', '?')}' must be a string")
                        is_valid = False
                    elif not (address.startswith("0x") or address.isdigit()):
                        self.errors.append(f"Invalid address format in mapping '{entry.get('name', '?')}': {address}")
                        is_valid = False

                # Validate data type
                allowed_types = self.schema.get("definitions", {}).get("memory_mapping_entry", {}).get("properties", {}).get("type", {}).get("enum", [])
                if "type" in entry and entry["type"] not in allowed_types:
                    self.errors.append(f"Invalid type in mapping '{entry.get('name', '?')}': {entry['type']}. Must be one of: {', '.join(allowed_types)}")
                    is_valid = False

                # Validate bit_position is present for bit type
                if entry.get("type") == "bit" and "bit_position" not in entry:
                    self.errors.append(f"Mapping '{entry.get('name', '?')}' has type 'bit' but no 'bit_position' specified")
                    is_valid = False

                # Validate numeric values
                for field in ["scale", "offset", "min_value", "max_value"]:
                    if field in entry and not isinstance(entry[field], (int, float)):
                        self.errors.append(f"Field '{field}' in mapping '{entry.get('name', '?')}' must be a number")
                        is_valid = False

                # Validate min_value < max_value if both are present
                if "min_value" in entry and "max_value" in entry:
                    if entry["min_value"] >= entry["max_value"]:
                        self.errors.append(f"min_value must be less than max_value in mapping '{entry.get('name', '?')}'")
                        is_valid = False

                # Validate endianness
                if "endianness" in entry and entry["endianness"] not in ["big", "little"]:
                    self.errors.append(f"Invalid endianness in mapping '{entry.get('name', '?')}': {entry['endianness']}. Must be 'big' or 'little'")
                    is_valid = False

        return is_valid

    def _validate_groups(self, mapping_data: Dict[str, Any]) -> bool:
        """Validate that all group entries reference valid mappings"""
        is_valid = True
        
        if "groups" not in mapping_data or not isinstance(mapping_data["groups"], dict):
            return is_valid
            
        # Collect all defined mapping names
        defined_mappings = set()
        if "mappings" in mapping_data and isinstance(mapping_data["mappings"], dict):
            for category, entries in mapping_data["mappings"].items():
                for entry in entries:
                    if "name" in entry:
                        defined_mappings.add(entry["name"])
        
        # Check that all group references point to defined mappings
        for group_name, mappings in mapping_data["groups"].items():
            if not isinstance(mappings, list):
                self.errors.append(f"Group '{group_name}' must contain a list of mapping names")
                is_valid = False
                continue
                
            # Check for duplicate entries in the same group
            seen_in_group = set()
            for mapping_name in mappings:
                if mapping_name in seen_in_group:
                    self.warnings.append(f"Duplicate mapping '{mapping_name}' in group '{group_name}'")
                seen_in_group.add(mapping_name)
                
                # Check that the mapping exists
                if mapping_name not in defined_mappings:
                    self.errors.append(f"Group '{group_name}' references undefined mapping: {mapping_name}")
                    is_valid = False
        
        return is_valid

    def validate_directory(self, directory: str) -> Dict[str, Tuple[bool, List[str], List[str]]]:
        """Validate all JSON files in a directory"""
        results = {}
        
        if not os.path.isdir(directory):
            print(f"Error: {directory} is not a directory")
            return results
            
        json_files = glob.glob(os.path.join(directory, "*.json"))
        if not json_files:
            print(f"No JSON files found in {directory}")
            return results
            
        for json_file in json_files:
            if os.path.basename(json_file) == os.path.basename(self.schema_path):
                continue  # Skip the schema file itself
                
            is_valid, errors, warnings = self.validate(json_file)
            results[json_file] = (is_valid, errors, warnings)
            
        return results

    def generate_report(self, results: Dict[str, Tuple[bool, List[str], List[str]]], output_file: Optional[str] = None) -> None:
        """Generate a validation report from results"""
        valid_count = sum(1 for is_valid, _, _ in results.values() if is_valid)
        invalid_count = len(results) - valid_count
        
        report = []
        report.append("=" * 80)
        report.append(f"Memory Mapping Validation Report")
        report.append("=" * 80)
        report.append(f"Files validated: {len(results)}")
        report.append(f"Valid files: {valid_count}")
        report.append(f"Invalid files: {invalid_count}")
        report.append("=" * 80)
        report.append("")
        
        # Generate detailed report for each file
        for file_path, (is_valid, errors, warnings) in results.items():
            report.append(f"File: {os.path.basename(file_path)}")
            report.append(f"  Status: {'VALID' if is_valid else 'INVALID'}")
            
            if errors:
                report.append(f"  Errors ({len(errors)}):")
                for error in errors:
                    report.append(f"    - {error}")
            
            if warnings:
                report.append(f"  Warnings ({len(warnings)}):")
                for warning in warnings:
                    report.append(f"    - {warning}")
                    
            report.append("")
        
        report_text = "\n".join(report)
        
        if output_file:
            try:
                with open(output_file, "w") as f:
                    f.write(report_text)
                print(f"Report written to {output_file}")
            except Exception as e:
                print(f"Error writing report to {output_file}: {e}")
                print(report_text)
        else:
            print(report_text)


def main():
    parser = argparse.ArgumentParser(description="Validate memory mapping JSON files")
    parser.add_argument("input", help="JSON file or directory to validate")
    parser.add_argument("-s", "--schema", default="src/ai/memory_mapping_schema.json", help="Path to schema file")
    parser.add_argument("-o", "--output", help="Output file for validation report")
    parser.add_argument("-v", "--verbose", action="store_true", help="Show verbose output")
    
    args = parser.parse_args()
    
    validator = MappingValidator(args.schema)
    validator.verbose = args.verbose
    
    if os.path.isdir(args.input):
        results = validator.validate_directory(args.input)
        validator.generate_report(results, args.output)
    else:
        is_valid, errors, warnings = validator.validate(args.input)
        results = {args.input: (is_valid, errors, warnings)}
        validator.generate_report(results, args.output)
        
    # Return appropriate exit code
    if any(not is_valid for is_valid, _, _ in results.values()):
        sys.exit(1)
    
    sys.exit(0)


if __name__ == "__main__":
    main() 