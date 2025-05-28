#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>
#include <array>
#include <algorithm>
#include <functional>

// Include FBNeo headers (paths may need adjustment)
#include "../src/burner/burner.h"
#include "../src/intf/input/inp_interface.h"
#include "../src/burner/gami.h"

// Checksum function to validate game state
std::string calculate_game_state_hash() {
    // Create a buffer to hold RAM for hashing
    std::vector<uint8_t> buffer;
    
    // Add CPU RAM to buffer
    for (INT32 i = 0; i < nCPUCount; i++) {
        if (CPU[i].mem && CPU[i].memlen > 0) {
            buffer.insert(buffer.end(), CPU[i].mem, CPU[i].mem + CPU[i].memlen);
        }
    }
    
    // Add video RAM if available
    // This is just an example - adjust according to the actual structures in FBNeo
    if (VideoBuffer && nGameWidth * nGameHeight > 0) {
        buffer.insert(buffer.end(), 
                     (uint8_t*)VideoBuffer, 
                     (uint8_t*)VideoBuffer + (nGameWidth * nGameHeight * nBurnBpp / 8));
    }
    
    // Simple hash function
    uint32_t hash = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        hash = hash * 31 + buffer[i];
    }
    
    char hash_str[16];
    snprintf(hash_str, sizeof(hash_str), "%08x", hash);
    return std::string(hash_str);
}

// Run a replay and gather state hashes at each frame
std::vector<std::string> run_replay_and_hash(const char* rom_name, const char* replay_file, int max_frames = 1000) {
    std::vector<std::string> frame_hashes;
    
    // Initialize FBNeo
    BurnLibInit();
    
    // Load the ROM
    if (BurnDrvSelect(BurnDrvGetIndexByName(rom_name)) != 0) {
        std::cerr << "Failed to load ROM: " << rom_name << std::endl;
        return frame_hashes;
    }
    
    // Init game
    if (BurnDrvInit() != 0) {
        std::cerr << "Failed to initialize game" << std::endl;
        return frame_hashes;
    }
    
    // Load the replay file
    if (RecordLoadStart(replay_file) != 0) {
        std::cerr << "Failed to load replay: " << replay_file << std::endl;
        BurnDrvExit();
        return frame_hashes;
    }
    
    // Run the game frame by frame and collect hashes
    for (int frame = 0; frame < max_frames; frame++) {
        // Process inputs from replay
        RecordFrame();
        
        // Run one frame
        BurnDrvFrame();
        
        // Calculate hash of game state
        frame_hashes.push_back(calculate_game_state_hash());
        
        // Check if replay is finished
        if (RecordStatus() == 0) {
            break;
        }
    }
    
    // Cleanup
    RecordExit();
    BurnDrvExit();
    BurnLibExit();
    
    return frame_hashes;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <rom_name> <replay_file1> <replay_file2> [max_frames]" << std::endl;
        return 1;
    }
    
    const char* rom_name = argv[1];
    const char* replay_file1 = argv[2];
    const char* replay_file2 = argv[3];
    int max_frames = (argc > 4) ? atoi(argv[4]) : 1000;
    
    std::cout << "Testing replay determinism for " << rom_name << std::endl;
    std::cout << "Comparing: " << replay_file1 << " and " << replay_file2 << std::endl;
    
    // Run both replays and collect frame hashes
    auto hashes1 = run_replay_and_hash(rom_name, replay_file1, max_frames);
    auto hashes2 = run_replay_and_hash(rom_name, replay_file2, max_frames);
    
    // Compare frame counts
    if (hashes1.size() != hashes2.size()) {
        std::cout << "Replay lengths differ: " 
                  << hashes1.size() << " vs " << hashes2.size() << " frames" << std::endl;
    }
    
    // Find first divergence
    size_t min_frames = std::min(hashes1.size(), hashes2.size());
    size_t diverge_frame = min_frames;
    
    for (size_t i = 0; i < min_frames; i++) {
        if (hashes1[i] != hashes2[i]) {
            diverge_frame = i;
            break;
        }
    }
    
    // Output results
    if (diverge_frame < min_frames) {
        std::cout << "Replays diverge at frame " << diverge_frame << std::endl;
        std::cout << "Hash1: " << hashes1[diverge_frame] << std::endl;
        std::cout << "Hash2: " << hashes2[diverge_frame] << std::endl;
        return 1;
    } else {
        std::cout << "Replays are deterministic for " << min_frames << " frames" << std::endl;
        return 0;
    }
} 