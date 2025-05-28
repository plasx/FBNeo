#pragma once

// This header provides compatibility for conditional LibTorch compilation
#ifdef USE_LIBTORCH
#include <torch/script.h>
#include <torch/torch.h>
#else
// Forward declarations
namespace torch {
    namespace jit {
        class Module;
        namespace script {
            class Module;
        }
    }
}
#endif
