#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <random>
#include <thread>
#include <mutex>
#include <atomic>

// Forward declarations for Metal types
#ifdef __OBJC__
@protocol MTLDevice;
@protocol MTLCommandQueue;
@protocol MTLLibrary;
@protocol MTLComputePipelineState;
@protocol MTLBuffer;
#else
typedef void* id;
#endif

namespace fbneo {
namespace ai {

/**
 * @brief Class representing a hyperparameter configuration
 */
struct HyperparameterConfig {
    std::unordered_map<std::string, float> params;
    float performance;
    int episodes;
    
    HyperparameterConfig() : performance(0.0f), episodes(0) {}
    
    explicit HyperparameterConfig(const std::unordered_map<std::string, float>& p) 
        : params(p), performance(0.0f), episodes(0) {}
};

/**
 * @class HyperparameterTuner
 * @brief Implements hyperparameter optimization for reinforcement learning algorithms
 * 
 * This class provides a Metal-accelerated implementation of evolutionary hyperparameter
 * tuning for optimizing the performance of reinforcement learning algorithms.
 */
class HyperparameterTuner {
public:
    /**
     * @brief Type definition for evaluation function
     * 
     * The evaluation function takes a set of hyperparameters and returns a reward value.
     * Higher reward values indicate better hyperparameter settings.
     */
    using EvaluationFunction = std::function<float(const std::unordered_map<std::string, float>&)>;
    
    /**
     * @brief Constructor
     */
    HyperparameterTuner();
    
    /**
     * @brief Destructor
     */
    ~HyperparameterTuner();
    
    /**
     * @brief Add a hyperparameter to be tuned
     * @param name Name of the hyperparameter
     * @param minValue Minimum value for the hyperparameter
     * @param maxValue Maximum value for the hyperparameter
     */
    void addHyperparameter(const std::string& name, float minValue, float maxValue);
    
    /**
     * @brief Set the function used to evaluate hyperparameter sets
     * @param evalFunc Evaluation function
     */
    void setEvaluationFunction(EvaluationFunction evalFunc);
    
    /**
     * @brief Start the hyperparameter tuning process
     * @param numGenerations Number of generations to run
     * @param populationSize Size of the population in each generation
     */
    void startTuning(int numGenerations = 20, int populationSize = 50);
    
    /**
     * @brief Stop the hyperparameter tuning process
     */
    void stopTuning();
    
    /**
     * @brief Get the best hyperparameters found so far
     * @return Map of hyperparameter names to values
     */
    std::unordered_map<std::string, float> getBestHyperparameters() const;
    
    /**
     * @brief Get the best reward value found so far
     * @return Best reward value
     */
    float getBestReward() const;
    
    /**
     * @brief Save the best hyperparameters to a file
     * @param filename Path to the file
     * @return True if successful, false otherwise
     */
    bool saveToFile(const std::string& filename) const;
    
    /**
     * @brief Load hyperparameters from a file
     * @param filename Path to the file
     * @return True if successful, false otherwise
     */
    bool loadFromFile(const std::string& filename);
    
private:
    // Hyperparameter definition
    struct HyperparameterDefinition {
        std::string name;
        float minValue;
        float maxValue;
    };
    
    // Hyperparameter definitions
    std::vector<HyperparameterDefinition> hyperparameters;
    
    // Population for CPU-based tuning
    std::vector<std::vector<float>> population;
    
    // Best values found so far
    std::unordered_map<std::string, float> bestHyperparameterValues;
    float bestReward;
    
    // Evaluation function
    EvaluationFunction evaluationFunction;
    
    // Tuning thread
    std::thread tuningThread;
    std::atomic<bool> isRunning;
    
    // Random number generator
    std::mt19937 rng;
    
    // Mutex for thread safety
    std::mutex parametersMutex;
    
    // Tuning parameters
    float initialExplorationRate = 1.0f;
    
    // Metal acceleration
    id metalDevice = nullptr;
    id metalCommandQueue = nullptr;
    id metalLibrary = nullptr;
    id evalPipeline = nullptr;
    id genPipeline = nullptr;
    id hyperparametersBuffer = nullptr;
    id bestHyperparametersBuffer = nullptr;
    id rewardsBuffer = nullptr;
    id scoresBuffer = nullptr;
    id configBuffer = nullptr;
    id seedsBuffer = nullptr;
    
    // Metal helpers
    void initializeMetalResources();
    void cleanupMetalResources();
    void initializeMetalBuffers(int populationSize);
    
    // Tuning helpers
    void tuningLoop(int numGenerations, int populationSize);
    void generateRandomPopulation(int populationSize);
    void generatePopulationWithMetal(int populationSize);
};

/**
 * @brief Random search hyperparameter tuning
 */
class RandomSearchTuner : public HyperparameterTuner {
public:
    RandomSearchTuner();
    ~RandomSearchTuner() override;
    
    // Get next configuration to try (random within ranges)
    std::unordered_map<std::string, float> getNextConfiguration() override;
};

/**
 * @brief Grid search hyperparameter tuning
 */
class GridSearchTuner : public HyperparameterTuner {
public:
    GridSearchTuner(int pointsPerDimension = 5);
    ~GridSearchTuner() override;
    
    // Initialize with grid points for each parameter
    void initialize(const std::unordered_map<std::string, std::pair<float, float>>& paramRanges) override;
    
    // Get next configuration to try
    std::unordered_map<std::string, float> getNextConfiguration() override;
    
private:
    int pointsPerDimension;
    std::unordered_map<std::string, std::vector<float>> gridPoints;
    std::vector<int> currentIndices;
    bool hasMoreConfigurations;
    
    // Generate grid points for a parameter
    std::vector<float> generateGridPoints(float min, float max, int numPoints);
};

/**
 * @brief Bayesian optimization for hyperparameter tuning
 */
class BayesianOptimizationTuner : public HyperparameterTuner {
public:
    BayesianOptimizationTuner();
    ~BayesianOptimizationTuner() override;
    
    // Initialize the Gaussian Process model
    void initialize(const std::unordered_map<std::string, std::pair<float, float>>& paramRanges) override;
    
    // Get next configuration to try using acquisition function
    std::unordered_map<std::string, float> getNextConfiguration() override;
    
    // Update the GP model with new result
    void updateResult(const std::unordered_map<std::string, float>& params, float performance) override;
    
private:
    // Simple GP implementation (would be more complex in real system)
    struct GPModel {
        std::vector<std::vector<float>> X; // Parameter vectors
        std::vector<float> y;              // Performance values
        
        // GP hyperparameters
        float lengthScale;
        float signalVariance;
        float noiseVariance;
        
        // Calculate kernel between two parameter vectors
        float kernel(const std::vector<float>& x1, const std::vector<float>& x2) const;
        
        // Calculate mean and variance at a point
        std::pair<float, float> predict(const std::vector<float>& x) const;
        
        // Calculate acquisition function (Expected Improvement)
        float expectedImprovement(const std::vector<float>& x) const;
    };
    
    GPModel model;
    
    // Convert between parameter map and vector representation
    std::vector<float> mapToVector(const std::unordered_map<std::string, float>& params) const;
    std::unordered_map<std::string, float> vectorToMap(const std::vector<float>& vec) const;
    
    // Find configuration with maximum acquisition function
    std::vector<float> optimizeAcquisition() const;
};

/**
 * @brief Hyperparameter tuning factory
 */
class HyperparameterTunerFactory {
public:
    // Create a hyperparameter tuner of the specified type
    static std::unique_ptr<HyperparameterTuner> create(const std::string& type);
    
    // Get available tuner types
    static std::vector<std::string> getAvailableTuners();
};

} // namespace ai
} // namespace fbneo 