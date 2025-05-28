# ROM Setup for FBNeo Metal (Marvel vs Capcom)

This document explains how to set up the ROMs for Marvel vs Capcom in the FBNeo Metal emulator.

## ROM Requirements

You will need the Marvel vs Capcom ROM file (`mvsc.zip`). This ROM should contain all the necessary files for the game to run.

## Setup Instructions

1. **Create a ROM directory**: The emulator expects ROMs to be in the `roms` directory. If this directory doesn't exist, create it:
   ```
   mkdir -p roms
   ```

2. **Copy the ROM file**: Copy the Marvel vs Capcom ROM file (`mvsc.zip`) to the `roms` directory:
   ```
   cp /path/to/your/mvsc.zip roms/
   ```

3. **ROM path**: When running the emulator, you need to specify the path to the ROM file:
   ```
   ./fbneo_metal_core roms/mvsc.zip
   ```

## ROM Verification

FBNeo performs CRC checks on ROMs to ensure they are correct. The emulator will report any issues with the ROMs when you try to run the game.

### Expected CRC Values

For reference, these are the expected CRC values for some of the main ROM files in `mvsc.zip`:

- `mvc.10` (Main graphics ROM): CRC32 should be `0x2d4881cb`
- `mvc.11` (Main graphics ROM): CRC32 should be `0x4b0b6d3e`
- `mvc.12` (Main graphics ROM): CRC32 should be `0x91a62ac8`
- `mvce.03` (Program ROM - Euro): CRC32 should be `0x7158bf45`
- `mvce.04` (Program ROM - Euro): CRC32 should be `0x12acd064`
- `mvc.01` (QSound data): CRC32 should be `0x41629e95`
- `mvc.02` (QSound data): CRC32 should be `0x963abf6b`

## Troubleshooting

- **ROM not found**: Make sure you've placed the ROM file in the correct location (`roms/mvsc.zip`).
- **CRC mismatch**: If you see a CRC mismatch error, it means your ROM file may be corrupted or from a different version. Try obtaining a verified ROM file.
- **Missing files**: If the emulator reports missing files, ensure you're using a complete ROM set.

## Support

If you encounter issues with ROM loading, please check the console output for specific error messages. The emulator provides detailed information about what might be wrong with your ROM setup. 