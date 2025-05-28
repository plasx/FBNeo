#include <cstdlib>
#include <cstring>
#include <stdint.h>

// Forward declare the types needed by the mips3 class
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef int INT32;

// This creates a minimal implementation of the mips3 class
// to satisfy the compiler requirements

namespace mips {
    // Create a dummy mips3 class with the same interface as expected by mips3_intf.cpp
    class mips3 {
    public:
        // Constructor/destructor must be defined externally (outside the class)
        // to ensure they're not inlined, so they can be found by the linker
        mips3();
        ~mips3();
        
        // Basic operations - stub implementations
        void reset() {}
        int run(int cycles, bool skip_op = false) { return 0; }
        
        // State
        struct {
            // Control registers - just enough for what mips3_intf.cpp needs
            UINT64 cpr[16][32];  // System control coprocessor registers
            UINT32 pc;           // Program counter
            UINT64 r[32];        // GPRs
        } m_state;
        
        // Previous program counter - needed by Mips3GetPC()
        UINT32 m_prev_pc = 0;
    };
    
    // Define the constructor and destructor outside the class
    // to prevent inlining and ensure they're available to the linker
    mips3::mips3() {
        memset(&m_state, 0, sizeof(m_state));
    }
    
    mips3::~mips3() {
        // Cleanup code if needed
    }

    // Create a global instance that can be accessed from C code
    mips3 g_mips3;
}

// Export C functions that call the mips3 methods
extern "C" {
    void* Mips3GlobalObject() {
        return (void*)&mips::g_mips3;
    }

    void Mips3GlobalReset() {
        mips::g_mips3.reset();
    }

    int Mips3GlobalRun(int cycles, int skip_op) {
        return mips::g_mips3.run(cycles, skip_op != 0);
    }

    // Add any C-linkage stubs here
    void m68k_modify_timeslice(int value) {}
} 