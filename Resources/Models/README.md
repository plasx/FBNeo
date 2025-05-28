# FBNeo AI Models

This directory contains AI models for FBNeo's AI features. Models should be in CoreML format (.mlmodel or .mlpackage).

## Model Types

- **Generic Models**: Work across multiple games of the same genre
- **Game-Specific Models**: Optimized for specific games
- **Training Models**: Used for AI training and self-play

## Installing Models

Models can be placed in the following locations:

1. This directory (`Resources/Models/`) - Application bundle models
2. `~/Library/Application Support/FBNeo/Models/` - User models directory
3. Game-specific directory: `~/Library/Application Support/FBNeo/Models/<game_id>/`

## Model Structure

Each model should include:

- CoreML model file (.mlmodel or .mlpackage)
- Info.plist file with metadata (optional but recommended)
- README with usage instructions

## Default Models

The following default models will be provided:

1. **generic_fighting.mlmodel** - Generic model for fighting games
2. **generic_shmup.mlmodel** - Generic model for shoot 'em up games
3. **generic_platform.mlmodel** - Generic model for platform games
4. **generic_puzzle.mlmodel** - Generic model for puzzle games

## Creating Custom Models

Information on creating custom models can be found in the developer documentation. 