# Test Fixtures

This directory contains test fixtures for the FBNeo Metal test suite. Test fixtures are fixed data used in test cases, ensuring that tests run against consistent, predictable inputs.

## ROM Test Fixtures

The test suite uses mock ROMs for testing ROM loading and verification functionality. These are automatically generated during test execution and do not contain any actual copyrighted ROM data.

### Auto-generated Fixtures

- `test_rom.zip` - A minimal mock ROM ZIP created to test basic ROM loading functionality
- Various checksum files for testing verification

## How Fixtures Are Used

The test fixtures are used by:

1. **CPS2 ROM Loader Tests** - To test loading and verification without requiring actual ROM files
2. **Integration Tests** - To test the interaction between Metal renderer and ROM handling
3. **Performance Tests** - To provide consistent input data for benchmarking

## Creating Custom Fixtures

To create additional test fixtures, you can either:

1. Let the tests auto-generate them (preferred method)
2. Manually create them in this directory

**Note:** Do not commit any copyrighted ROM data to this repository. All fixtures should be either mock data or freely redistributable content.

## Directory Structure

```
fixtures/
├── README.md           # This file
├── test_rom.zip        # Auto-generated mock ROM
└── checksums/          # Contains verification data
```

## Automated Fixture Generation

Some tests will auto-generate needed fixtures if they don't exist:

```cpp
// Example from CPS2RomLoaderTest::SetUp
void SetUp() override {
    // Initialize the ROM loader
    CPS2_InitROMLoader();
    
    // Create test directories if needed
    system("mkdir -p tests/metal/fixtures");
    
    // Create a minimal mock ROM ZIP file for testing if it doesn't exist
    struct stat buffer;
    if (stat(TEST_ROM_PATH, &buffer) != 0) {
        CreateMockRomZip();
    }
}
```

This approach ensures tests can run without requiring manual setup while avoiding the inclusion of copyrighted content in the repository. 