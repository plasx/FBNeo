#include <metal_stdlib>
#include <metal_compute>
#include <metal_matrix>
#include <metal_math>

using namespace metal;

// Structure definitions for tensor operations
struct ConvParams {
    uint inputWidth;
    uint inputHeight;
    uint inputChannels;
    uint outputChannels;
    uint kernelSize;
    uint stride;
    uint padding;
    float activation_alpha;
    float activation_beta;
    int activation_type; // 0=none, 1=relu, 2=leaky_relu, 3=sigmoid
};

struct MatrixParams {
    uint M;      // Rows in output
    uint N;      // Columns in output
    uint K;      // Inner dimension
    bool transpose_a;
    bool transpose_b;
    bool accumulate;
    int activation_type;
    float activation_alpha;
    float activation_beta;
};

struct PoolParams {
    uint inputWidth;
    uint inputHeight;
    uint channels;
    uint kernelSize;
    uint stride;
    uint padding;
    bool isMaxPool;  // true for max pooling, false for average pooling
};

struct SparseMatrixParams {
    uint numRows;
    uint numCols;
    uint numNonZero;
    uint blockSize; // For block-sparse format
    bool useTensorCores;
};

// Activation functions
float apply_activation(float input, int activation_type, float alpha, float beta) {
    switch (activation_type) {
        case 1: // ReLU
            return max(0.0f, input);
        case 2: // Leaky ReLU
            return input > 0.0f ? input : alpha * input;
        case 3: // Sigmoid
            return 1.0f / (1.0f + exp(-input));
        case 4: // Tanh
            return tanh(input);
        case 5: // ELU
            return input > 0.0f ? input : alpha * (exp(input) - 1.0f);
        case 6: // SELU
            return input > 0.0f ? beta * input : beta * alpha * (exp(input) - 1.0f);
        default: // No activation
            return input;
    }
}

// Matrix multiplication using tensor cores where available
kernel void matrix_multiply(device const float* a [[buffer(0)]],
                           device const float* b [[buffer(1)]],
                           device float* c [[buffer(2)]],
                           constant MatrixParams& params [[buffer(3)]],
                           uint2 gid [[thread_position_in_grid]],
                           uint2 gridDim [[threads_per_grid]]) {
    // Make sure we're within bounds
    if (gid.x >= params.N || gid.y >= params.M)
        return;
    
    uint m = gid.y;
    uint n = gid.x;
    
    // Calculate starting positions for matrices
    uint aRow = params.transpose_a ? m * params.K : m;
    uint aCol = params.transpose_a ? 0 : 0;
    uint aRowStride = params.transpose_a ? 1 : params.K;
    uint aColStride = params.transpose_a ? params.M : 1;
    
    uint bRow = params.transpose_b ? 0 : 0;
    uint bCol = params.transpose_b ? n : n;
    uint bRowStride = params.transpose_b ? params.N : 1;
    uint bColStride = params.transpose_b ? 1 : params.K;
    
    // Matrix multiplication
    float sum = 0.0f;
    for (uint k = 0; k < params.K; k++) {
        uint aIndex = aRow * aRowStride + (aCol + k) * aColStride;
        uint bIndex = (bRow + k) * bRowStride + bCol * bColStride;
        sum += a[aIndex] * b[bIndex];
    }
    
    // Apply activation
    sum = apply_activation(sum, params.activation_type, params.activation_alpha, params.activation_beta);
    
    // Store result
    uint cIndex = m * params.N + n;
    if (params.accumulate) {
        c[cIndex] += sum;
    } else {
        c[cIndex] = sum;
    }
}

// Optimized matrix multiplication using matrix types when available (M1/M2/M3 chips)
kernel void matrix_multiply_optimized(device const half* a [[buffer(0)]],
                                     device const half* b [[buffer(1)]],
                                     device half* c [[buffer(2)]],
                                     constant MatrixParams& params [[buffer(3)]],
                                     uint2 gid [[thread_position_in_grid]]) {
    constexpr int TILE_SIZE = 8;
    
    // Calculate output coordinates
    uint row = gid.y * TILE_SIZE;
    uint col = gid.x * TILE_SIZE;
    
    // Ensure we're in bounds
    if (row >= params.M || col >= params.N)
        return;
    
    // Limit tile size to matrix boundaries
    uint rowLimit = min(row + TILE_SIZE, params.M);
    uint colLimit = min(col + TILE_SIZE, params.N);
    uint tileRows = rowLimit - row;
    uint tileCols = colLimit - col;
    
    // Create matrix tiles for accelerated computation
    half tileA[TILE_SIZE][TILE_SIZE];
    half tileB[TILE_SIZE][TILE_SIZE];
    half tileC[TILE_SIZE][TILE_SIZE] = {0.0h};
    
    // Process K dimension in blocks for better cache usage
    for (uint kBlock = 0; kBlock < (params.K + TILE_SIZE - 1) / TILE_SIZE; kBlock++) {
        // Load tiles from global memory
        for (uint i = 0; i < tileRows; i++) {
            for (uint j = 0; j < TILE_SIZE; j++) {
                uint k = kBlock * TILE_SIZE + j;
                if (k < params.K) {
                    uint aIndex = params.transpose_a ? 
                        (k * params.M + (row + i)) : 
                        ((row + i) * params.K + k);
                    tileA[i][j] = a[aIndex];
                } else {
                    tileA[i][j] = 0.0h;
                }
            }
        }
        
        for (uint i = 0; i < TILE_SIZE; i++) {
            for (uint j = 0; j < tileCols; j++) {
                uint k = kBlock * TILE_SIZE + i;
                if (k < params.K) {
                    uint bIndex = params.transpose_b ? 
                        ((col + j) * params.K + k) : 
                        (k * params.N + (col + j));
                    tileB[i][j] = b[bIndex];
                } else {
                    tileB[i][j] = 0.0h;
                }
            }
        }
        
        // Matrix multiplication within tile
        for (uint i = 0; i < tileRows; i++) {
            for (uint j = 0; j < tileCols; j++) {
                half sum = 0.0h;
                for (uint k = 0; k < TILE_SIZE; k++) {
                    sum += tileA[i][k] * tileB[k][j];
                }
                tileC[i][j] += sum;
            }
        }
    }
    
    // Write results to global memory
    for (uint i = 0; i < tileRows; i++) {
        for (uint j = 0; j < tileCols; j++) {
            uint index = (row + i) * params.N + (col + j);
            half result = tileC[i][j];
            
            // Apply activation
            result = apply_activation(result, params.activation_type, params.activation_alpha, params.activation_beta);
            
            // Store result
            if (params.accumulate) {
                c[index] += result;
            } else {
                c[index] = result;
            }
        }
    }
}

// Convolution kernel with optimizations for 3x3 filters (common in neural networks)
kernel void conv2d(device const float* input [[buffer(0)]],
                  device const float* weights [[buffer(1)]],
                  device const float* bias [[buffer(2)]],
                  device float* output [[buffer(3)]],
                  constant ConvParams& params [[buffer(4)]],
                  uint3 gid [[thread_position_in_grid]]) {
    
    // Calculate output position
    uint x = gid.x;
    uint y = gid.y;
    uint z = gid.z;
    
    // Ensure we're within bounds
    if (x >= (params.inputWidth + 2 * params.padding - params.kernelSize) / params.stride + 1 ||
        y >= (params.inputHeight + 2 * params.padding - params.kernelSize) / params.stride + 1 ||
        z >= params.outputChannels) {
        return;
    }
    
    // Initialize sum with bias value
    float sum = bias[z];
    
    // Convolution operation
    for (uint c = 0; c < params.inputChannels; c++) {
        for (uint ky = 0; ky < params.kernelSize; ky++) {
            for (uint kx = 0; kx < params.kernelSize; kx++) {
                // Calculate input coordinates with padding
                int ix = int(x * params.stride) + int(kx) - int(params.padding);
                int iy = int(y * params.stride) + int(ky) - int(params.padding);
                
                // Check if input pixel is within bounds
                if (ix >= 0 && ix < int(params.inputWidth) && 
                    iy >= 0 && iy < int(params.inputHeight)) {
                    
                    // Calculate input index
                    uint inputIdx = (c * params.inputHeight + iy) * params.inputWidth + ix;
                    
                    // Calculate weight index
                    uint weightIdx = ((z * params.inputChannels + c) * params.kernelSize + ky) * params.kernelSize + kx;
                    
                    // Accumulate weighted input
                    sum += input[inputIdx] * weights[weightIdx];
                }
            }
        }
    }
    
    // Apply activation function
    sum = apply_activation(sum, params.activation_type, params.activation_alpha, params.activation_beta);
    
    // Write output
    uint outputIdx = (z * ((params.inputHeight + 2 * params.padding - params.kernelSize) / params.stride + 1) + y) * 
                    ((params.inputWidth + 2 * params.padding - params.kernelSize) / params.stride + 1) + x;
    output[outputIdx] = sum;
}

// Optimized 3x3 convolution for neural networks
kernel void conv2d_3x3_optimized(device const float* input [[buffer(0)]],
                              device const float* weights [[buffer(1)]],
                              device const float* bias [[buffer(2)]],
                              device float* output [[buffer(3)]],
                              constant ConvParams& params [[buffer(4)]],
                              uint3 gid [[thread_position_in_grid]]) {
    
    // Only works for 3x3 kernels
    if (params.kernelSize != 3) {
        return;
    }
    
    // Calculate output position
    uint x = gid.x;
    uint y = gid.y;
    uint z = gid.z;
    
    // Output dimensions
    uint outWidth = (params.inputWidth + 2 * params.padding - params.kernelSize) / params.stride + 1;
    uint outHeight = (params.inputHeight + 2 * params.padding - params.kernelSize) / params.stride + 1;
    
    // Ensure we're within bounds
    if (x >= outWidth || y >= outHeight || z >= params.outputChannels) {
        return;
    }
    
    // Initialize sum with bias value
    float sum = bias[z];
    
    // Use local memory for weights to improve cache coherence
    threadgroup float localWeights[9];
    
    // Convolution operation
    for (uint c = 0; c < params.inputChannels; c++) {
        // Load weights into local memory
        for (uint i = 0; i < 9; i++) {
            uint weightIdx = (z * params.inputChannels + c) * 9 + i;
            localWeights[i] = weights[weightIdx];
        }
        
        // Calculate input start position with padding
        int iy0 = int(y * params.stride) - int(params.padding);
        int ix0 = int(x * params.stride) - int(params.padding);
        
        // Process 3x3 convolution window
        for (uint ky = 0; ky < 3; ky++) {
            int iy = iy0 + int(ky);
            if (iy >= 0 && iy < int(params.inputHeight)) {
                for (uint kx = 0; kx < 3; kx++) {
                    int ix = ix0 + int(kx);
                    if (ix >= 0 && ix < int(params.inputWidth)) {
                        // Calculate input index
                        uint inputIdx = (c * params.inputHeight + iy) * params.inputWidth + ix;
                        // Get weight from local memory
                        float weight = localWeights[ky * 3 + kx];
                        // Accumulate weighted input
                        sum += input[inputIdx] * weight;
                    }
                }
            }
        }
    }
    
    // Apply activation function
    sum = apply_activation(sum, params.activation_type, params.activation_alpha, params.activation_beta);
    
    // Write output
    uint outputIdx = (z * outHeight + y) * outWidth + x;
    output[outputIdx] = sum;
}

// Pooling operation (max or average)
kernel void pooling(device const float* input [[buffer(0)]],
                   device float* output [[buffer(1)]],
                   constant PoolParams& params [[buffer(2)]],
                   uint3 gid [[thread_position_in_grid]]) {
    
    // Calculate output position
    uint x = gid.x;
    uint y = gid.y;
    uint c = gid.z;
    
    // Output dimensions
    uint outWidth = (params.inputWidth + 2 * params.padding - params.kernelSize) / params.stride + 1;
    uint outHeight = (params.inputHeight + 2 * params.padding - params.kernelSize) / params.stride + 1;
    
    // Ensure we're within bounds
    if (x >= outWidth || y >= outHeight || c >= params.channels) {
        return;
    }
    
    // Initialize result
    float result = params.isMaxPool ? -INFINITY : 0.0f;
    uint count = 0;
    
    // Pooling operation
    for (uint ky = 0; ky < params.kernelSize; ky++) {
        // Calculate input row with padding
        int iy = int(y * params.stride) + int(ky) - int(params.padding);
        if (iy < 0 || iy >= int(params.inputHeight))
            continue;
        
        for (uint kx = 0; kx < params.kernelSize; kx++) {
            // Calculate input column with padding
            int ix = int(x * params.stride) + int(kx) - int(params.padding);
            if (ix < 0 || ix >= int(params.inputWidth))
                continue;
            
            // Calculate input index
            uint inputIdx = (c * params.inputHeight + iy) * params.inputWidth + ix;
            float val = input[inputIdx];
            
            if (params.isMaxPool) {
                // Max pooling
                result = max(result, val);
            } else {
                // Average pooling
                result += val;
                count++;
            }
        }
    }
    
    // Finalize average pooling
    if (!params.isMaxPool && count > 0) {
        result /= float(count);
    }
    
    // Write output
    uint outputIdx = (c * outHeight + y) * outWidth + x;
    output[outputIdx] = result;
}

// Sparse matrix-vector multiplication for better efficiency
kernel void sparse_matrix_vector_multiply(
    device const float* values [[buffer(0)]],         // Non-zero values
    device const uint* rowPtrs [[buffer(1)]],         // Row pointers (CSR format)
    device const uint* colIndices [[buffer(2)]],      // Column indices
    device const float* vector [[buffer(3)]],         // Dense vector
    device float* result [[buffer(4)]],               // Output vector
    constant SparseMatrixParams& params [[buffer(5)]],
    uint gid [[thread_position_in_grid]])
{
    // Ensure we're within bounds
    if (gid >= params.numRows)
        return;
    
    float sum = 0.0f;
    
    // Get start and end positions for this row
    uint start = rowPtrs[gid];
    uint end = rowPtrs[gid + 1];
    
    // Process non-zero elements
    for (uint i = start; i < end; i++) {
        uint col = colIndices[i];
        sum += values[i] * vector[col];
    }
    
    // Store result
    result[gid] = sum;
}

// Block-sparse matrix multiply for neural networks with structured sparsity
kernel void block_sparse_matrix_multiply(
    device const float* blocks [[buffer(0)]],         // Non-zero blocks
    device const uint* blockRowPtrs [[buffer(1)]],    // Row pointers for blocks
    device const uint* blockColIndices [[buffer(2)]], // Column indices for blocks
    device const float* input [[buffer(3)]],          // Input matrix
    device float* output [[buffer(4)]],               // Output matrix
    constant SparseMatrixParams& params [[buffer(5)]],
    uint gid [[thread_position_in_grid]])
{
    // Block size (e.g., 4x4, 8x8, etc.)
    uint blockSize = params.blockSize;
    
    // Calculate row block
    uint rowBlock = gid / (params.numCols / blockSize);
    // Calculate column within output
    uint outCol = gid % (params.numCols / blockSize);
    
    // Ensure we're within bounds
    if (rowBlock >= params.numRows / blockSize)
        return;
    
    // Get start and end positions for this block row
    uint start = blockRowPtrs[rowBlock];
    uint end = blockRowPtrs[rowBlock + 1];
    
    // Initialize output block
    for (uint i = 0; i < blockSize; i++) {
        for (uint j = 0; j < blockSize; j++) {
            output[(rowBlock * blockSize + i) * params.numCols + outCol * blockSize + j] = 0.0f;
        }
    }
    
    // Process non-zero blocks
    for (uint i = start; i < end; i++) {
        uint colBlock = blockColIndices[i];
        
        // Block matrix multiplication
        for (uint bi = 0; bi < blockSize; bi++) {
            for (uint bj = 0; bj < blockSize; bj++) {
                for (uint bk = 0; bk < blockSize; bk++) {
                    uint outIdx = (rowBlock * blockSize + bi) * params.numCols + outCol * blockSize + bj;
                    uint blockIdx = (i * blockSize * blockSize) + (bi * blockSize) + bk;
                    uint inIdx = (colBlock * blockSize + bk) * params.numCols + outCol * blockSize + bj;
                    
                    output[outIdx] += blocks[blockIdx] * input[inIdx];
                }
            }
        }
    }
}

// Fast 1x1 convolution implementation (equivalent to matrix multiply with reshape)
kernel void conv2d_1x1(device const float* input [[buffer(0)]],
                      device const float* weights [[buffer(1)]],
                      device const float* bias [[buffer(2)]],
                      device float* output [[buffer(3)]],
                      constant ConvParams& params [[buffer(4)]],
                      uint3 gid [[thread_position_in_grid]]) {
    
    // Only works for 1x1 kernels
    if (params.kernelSize != 1 || params.stride != 1 || params.padding != 0) {
        return;
    }
    
    // Calculate output position
    uint x = gid.x;
    uint y = gid.y;
    uint z = gid.z;
    
    // Ensure we're within bounds
    if (x >= params.inputWidth || y >= params.inputHeight || z >= params.outputChannels) {
        return;
    }
    
    // Initialize sum with bias value
    float sum = bias[z];
    
    // Spatial location in flattened form
    uint spatialIdx = y * params.inputWidth + x;
    
    // 1x1 convolution is equivalent to matrix multiplication
    for (uint c = 0; c < params.inputChannels; c++) {
        uint inputIdx = c * params.inputHeight * params.inputWidth + spatialIdx;
        uint weightIdx = z * params.inputChannels + c;
        sum += input[inputIdx] * weights[weightIdx];
    }
    
    // Apply activation function
    sum = apply_activation(sum, params.activation_type, params.activation_alpha, params.activation_beta);
    
    // Write output
    uint outputIdx = z * params.inputHeight * params.inputWidth + spatialIdx;
    output[outputIdx] = sum;
}

// Implementation of depth-wise convolution (used in efficient networks like MobileNet)
kernel void depthwise_conv2d(device const float* input [[buffer(0)]],
                           device const float* weights [[buffer(1)]],
                           device const float* bias [[buffer(2)]],
                           device float* output [[buffer(3)]],
                           constant ConvParams& params [[buffer(4)]],
                           uint3 gid [[thread_position_in_grid]]) {
    
    // Calculate output position
    uint x = gid.x;
    uint y = gid.y;
    uint c = gid.z; // Channel is same for input and output
    
    // Output dimensions
    uint outWidth = (params.inputWidth + 2 * params.padding - params.kernelSize) / params.stride + 1;
    uint outHeight = (params.inputHeight + 2 * params.padding - params.kernelSize) / params.stride + 1;
    
    // Ensure we're within bounds
    if (x >= outWidth || y >= outHeight || c >= params.inputChannels) {
        return;
    }
    
    // Initialize sum with bias value
    float sum = bias[c];
    
    // Depthwise convolution (each input channel is convolved with its own filter)
    for (uint ky = 0; ky < params.kernelSize; ky++) {
        for (uint kx = 0; kx < params.kernelSize; kx++) {
            // Calculate input coordinates with padding
            int ix = int(x * params.stride) + int(kx) - int(params.padding);
            int iy = int(y * params.stride) + int(ky) - int(params.padding);
            
            // Check if input pixel is within bounds
            if (ix >= 0 && ix < int(params.inputWidth) && 
                iy >= 0 && iy < int(params.inputHeight)) {
                
                // Calculate input index
                uint inputIdx = (c * params.inputHeight + iy) * params.inputWidth + ix;
                
                // Calculate weight index (for depthwise, weights are [channel, ky, kx])
                uint weightIdx = (c * params.kernelSize + ky) * params.kernelSize + kx;
                
                // Accumulate weighted input
                sum += input[inputIdx] * weights[weightIdx];
            }
        }
    }
    
    // Apply activation function
    sum = apply_activation(sum, params.activation_type, params.activation_alpha, params.activation_beta);
    
    // Write output
    uint outputIdx = (c * outHeight + y) * outWidth + x;
    output[outputIdx] = sum;
}

// Tensor addition/subtraction with broadcasting support
kernel void tensor_add_sub(device const float* a [[buffer(0)]],
                          device const float* b [[buffer(1)]],
                          device float* output [[buffer(2)]],
                          constant uint* dims [[buffer(3)]],
                          constant uint* strides_a [[buffer(4)]],
                          constant uint* strides_b [[buffer(5)]],
                          constant int& subtract [[buffer(6)]],
                          constant int& activation_type [[buffer(7)]],
                          constant float& alpha [[buffer(8)]],
                          constant float& beta [[buffer(9)]],
                          uint3 gid [[thread_position_in_grid]]) {
    
    uint rank = 4; // 4D tensor
    uint idx[4] = {gid.z / dims[2], gid.z % dims[2], gid.y, gid.x};
    
    // Check if we're within bounds
    if (idx[0] >= dims[0] || idx[1] >= dims[1] || idx[2] >= dims[2] || idx[3] >= dims[3]) {
        return;
    }
    
    // Calculate linear indices with strides (for broadcasting support)
    uint idx_a = 0;
    uint idx_b = 0;
    
    for (uint i = 0; i < rank; i++) {
        idx_a += idx[i] * strides_a[i];
        idx_b += idx[i] * strides_b[i];
    }
    
    // Perform addition or subtraction
    float result;
    if (subtract) {
        result = a[idx_a] - b[idx_b];
    } else {
        result = a[idx_a] + b[idx_b];
    }
    
    // Apply activation function
    result = apply_activation(result, activation_type, alpha, beta);
    
    // Calculate output index
    uint out_idx = ((idx[0] * dims[1] + idx[1]) * dims[2] + idx[2]) * dims[3] + idx[3];
    
    // Write output
    output[out_idx] = result;
} 