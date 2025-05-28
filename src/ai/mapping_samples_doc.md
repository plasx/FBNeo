# Memory Mapping Workflow Documentation

This document explains the workflow for creating memory mappings for FBNeo games, from sample collection to analysis to final mapping creation.

## Overview

The memory mapping workflow consists of three main stages:
1. **Sample Collection**: Collecting memory dumps while playing a game
2. **Memory Analysis**: Analyzing the collected samples to identify patterns
3. **Mapping Creation**: Creating and refining memory mapping files

## Sample Collection

The `mapping_utility.py` script provides functionality for collecting memory samples from FBNeo games. Samples are collected as JSON files containing memory dumps at specific points during gameplay.

### Collection Process

1. Run the mapping utility with the game ROM name and architecture:
   ```
   python src/ai/mapping_utility.py --game sf3 --arch CPS3 --interactive
   ```

2. In the interactive mode, use the provided GUI to:
   - Set the number of samples to collect (typically 50-100)
   - Set the sampling interval (in milliseconds, typically 100-500ms)
   - Press the "Collect Samples" button when ready

3. During collection:
   - Play the game, covering various game states and situations
   - Press the mapped key (default: F8) to trigger a sample collection
   - Try to capture different game states: menu screens, character selection, different match states, etc.

4. Samples are saved to `samples/<game_name>_samples.json`

### Sample File Format

The sample file contains an array of memory snapshots with the following structure:

```json
{
  "samples": [
    {
      "timestamp": "2023-07-15T14:30:22.456",
      "frame_number": 1234,
      "memory": {
        "0xFF8400": "0x0064",
        "0xFF8402": "0x0032",
        ...
      }
    },
    ...
  ]
}
```

## Memory Analysis

The `memory_analyzer.py` script analyzes collected samples to identify patterns and potential memory mappings.

### Running Analysis

1. After collecting samples, run the analyzer:
   ```
   python src/ai/memory_analyzer.py samples/sf3_samples.json --output analysis/sf3_analysis.json --suggest
   ```

2. The analyzer will:
   - Identify constant values
   - Find high-variance addresses
   - Detect correlated addresses
   - Identify counters and timers
   - Detect state variables
   - Suggest potential mappings

### Analysis Options

- `--plot <addresses>`: Plot value changes for specific addresses
- `--heatmap`: Generate a correlation heatmap
- `--suggest`: Generate mapping suggestions

### Interpreting Results

The analysis output includes:

1. **Constants**: Memory addresses that rarely or never change
2. **High Variance**: Addresses that change frequently (often player-related values)
3. **Correlations**: Addresses that change together (e.g., player position X and Y)
4. **Counters**: Addresses that increment or decrement consistently
5. **State Variables**: Addresses with discrete states (often game state indicators)

### Visualization

The analyzer can generate visualizations to help understand memory patterns:

1. **Value Change Plots**: Show how values change over time
   ```
   python src/ai/memory_analyzer.py samples/sf3_samples.json --plot 0xFF8400 0xFF8402
   ```

2. **Correlation Heatmaps**: Show relationships between memory addresses
   ```
   python src/ai/memory_analyzer.py samples/sf3_samples.json --heatmap
   ```

## Mapping Creation

After analysis, use the mapping utility to create and refine memory mappings.

### Creating Mappings

1. Based on the analysis results, create a JSON mapping file:
   ```json
   {
     "game": "Street Fighter III: 3rd Strike",
     "architecture": "CPS3",
     "version": "1.0.0",
     "description": "Memory mappings for SF3: 3rd Strike",
     "supported_roms": ["sfiii3"],
     "mappings": [
       {
         "name": "p1_health",
         "address": "0xFF8400",
         "type": "word",
         "description": "Player 1 health",
         "category": "player_state",
         "player_index": 1,
         "min_value": 0,
         "max_value": 160
       },
       ...
     ]
   }
   ```

2. Save this file to `src/ai/mappings/<game_name>.json`

### Validating Mappings

Use the mapping validator to check your mapping file:

```
python src/ai/mapping_validator.py src/ai/mappings/sf3.json
```

This will check for:
- Conformance to the JSON schema
- Valid game name, architecture, and ROM IDs
- Valid address formats
- Proper data types
- Other mapping-specific validations

### Testing Mappings

Test your mappings in-game using the mapping utility:

1. Load the mappings:
   ```
   python src/ai/mapping_utility.py --game sf3 --mapping src/ai/mappings/sf3.json --interactive
   ```

2. In the interactive mode:
   - Click "Test Mappings" to launch FBNeo with the game
   - The utility will show live values from the mapped memory addresses
   - Verify that the values change as expected during gameplay

### Refining Mappings

Based on testing results:
1. Add missing mappings for important game states
2. Remove or fix incorrect mappings
3. Adjust min/max values based on observed ranges
4. Group related mappings into logical categories
5. Re-validate and re-test the updated mapping file

## Best Practices

1. **Collect Diverse Samples**: Ensure samples cover different game states, characters, and situations
2. **Focus on Key Game Elements**: Prioritize mapping health, position, game state, and other essential elements
3. **Use Consistent Naming**: Follow naming conventions (e.g., `p1_health`, `p2_health`)
4. **Categorize Mappings**: Group mappings by category (player_state, game_state, etc.)
5. **Document Mappings**: Add clear descriptions to mappings
6. **Version Control**: Track changes to mapping files
7. **Validate Thoroughly**: Ensure mappings work correctly before finalizing

## Advanced Techniques

### Bit-Level Mappings

For packed data, use bit-specific mappings:

```json
{
  "name": "p1_is_blocking",
  "address": "0xFF8A10",
  "type": "byte",
  "bit": 3,
  "description": "Player 1 blocking state (bit 3)"
}
```

### Derived Values

For values that need scaling or offsetting:

```json
{
  "name": "p1_health_percent",
  "address": "0xFF8400",
  "type": "word",
  "scale": 0.625,
  "description": "Player 1 health percentage (0-100)"
}
```

### Multi-Byte Processing

For values spanning multiple bytes:

```json
{
  "name": "score",
  "address": "0xFF9000",
  "type": "dword",
  "endianness": "little",
  "description": "Player score (4 bytes)"
}
```

## Troubleshooting

1. **Inconsistent Values**: 
   - Ensure you're using the correct architecture
   - Check for alternate addresses that might be used
   - Verify endianness settings

2. **Missing Mappings**:
   - Collect more samples in different game states
   - Try advanced analysis techniques

3. **Validation Errors**:
   - Check address format (should be hexadecimal)
   - Verify data types match expected values
   - Ensure all required fields are present 