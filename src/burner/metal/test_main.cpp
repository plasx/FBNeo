#include <stdio.h>

extern "C" {
    bool InitMetal(void* view);
    void ShutdownMetal();
}

int main() {
    printf("FBNeo Metal Test\n");
    
    bool success = InitMetal(nullptr);
    if (success) {
        printf("Metal initialized successfully!\n");
    } else {
        printf("Failed to initialize Metal\n");
    }
    
    ShutdownMetal();
    
    return 0;
} 