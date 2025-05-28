#pragma once

// This is a stub header for torch/script.h
// It's used for conditional compilation when LibTorch is not available

namespace torch {
    namespace jit {
        class Module {
        public:
            Module() {}
            ~Module() {}
            
            void eval() {}
            
            template<typename... Args>
            void forward(Args... args) {}
            
            void save(const std::string& path) {}
        };
        
        namespace script {
            class Module : public jit::Module {
            public:
                static Module load(const std::string& path) { return Module(); }
            };
        }
    }
} 