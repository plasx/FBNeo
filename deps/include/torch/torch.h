#pragma once

// This is a stub header for torch/torch.h
// It's used for conditional compilation when LibTorch is not available

namespace torch {
    class Tensor {
    public:
        Tensor() {}
        ~Tensor() {}
        
        size_t numel() const { return 0; }
        template<typename T> T* data_ptr() const { return nullptr; }
        
        int64_t size(int dim) const { return 0; }
        std::vector<int64_t> sizes() const { return std::vector<int64_t>{}; }
    };
    
    class IValue {
    public:
        IValue() {}
        ~IValue() {}
        
        bool isTensor() const { return false; }
        bool isTuple() const { return false; }
        Tensor toTensor() const { return Tensor(); }
    };
    
    class NoGradGuard {
    public:
        NoGradGuard() {}
        ~NoGradGuard() {}
    };
    
    template<typename T>
    Tensor from_blob(T* data, std::vector<int64_t> sizes, int dtype) {
        return Tensor();
    }
    
    Tensor sigmoid(const Tensor& t) {
        return Tensor();
    }
    
    enum DataType {
        kFloat32 = 0
    };
} 