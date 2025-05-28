# Memory Mapping Utility

This utility helps you create and edit memory mappings for FBNeo games, used for various AI and replay features.

## Features

- Interactive GUI for creating and editing memory mappings
- Load existing mapping files
- Save mappings in the standard JSON format
- Analyze memory patterns from game samples

## Usage

### Command Line

```bash
python src/ai/mapping_utility.py <game_name> [options]
```

#### Options:

- `--architecture` - Specify game architecture (CPS1, CPS2, etc.)
- `--mapping-file` - Path to existing mapping file to load
- `--output` - Path to save the mapping file
- `--interactive` - Launch the interactive GUI editor

### Interactive Editor

The interactive editor provides a GUI for managing memory mappings:

1. Launch with `python src/ai/mapping_utility.py <game_name> --interactive`
2. Use the interface to add, edit, and remove mappings
3. Save your work to a JSON file

## Memory Mapping Format

The utility uses the standard FBNeo memory mapping format:

```json
{
  "game": "sf2",
  "architecture": "CPS1",
  "mappings": [
    {
      "name": "p1_health",
      "address": "0xFF83C6",
      "type": "byte",
      "description": "Player 1 health",
      "scale": 1.0,
      "offset": 0,
      "min": 0,
      "max": 100
    },
    ...
  ]
}
```

### Field Descriptions:

- `name`: Identifier for the memory value
- `address`: Memory address (hexadecimal or decimal)
- `type`: Data type (byte, word, dword, float)
- `description`: Human-readable description
- `scale` (optional): Scaling factor for the raw value
- `offset` (optional): Offset to add after scaling
- `min` (optional): Minimum possible value
- `max` (optional): Maximum possible value

## Integration with FBNeo

This utility is designed to work with FBNeo's memory mapping system. The mappings created with this tool can be used by:

1. The AI training pipeline
2. The replay visualizer
3. The determinism testing framework

## Requirements

- Python 3.6+
- Tkinter (for GUI mode)
- NumPy
- Matplotlib (for visualization features)

## Example Workflow

1. Start a new mapping: `python src/ai/mapping_utility.py sf2 --interactive`
2. Add essential memory mappings (health, positions, etc.)
3. Save the mapping file to `mappings/sf2.json`
4. Use the mapping with other tools in the FBNeo AI framework 