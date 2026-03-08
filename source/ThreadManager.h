#pragma once

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <memory>
#include <atomic>

class ThreadManager {
public:
    explicit ThreadManager(size_t numThreads = 4);
    ~ThreadManager();
    
    void submitShipTask(std::function<void()> task);
    void submitEffectTask(std::function<void()> task);
    
    void waitForShipsComplete();
    void waitForEffectsComplete();
    void waitForAllComplete();
    
    void shutdown();
    
    size_t GetNumThreads() const { return workers.size(); }

private:
    void workerLoop(std::queue<std::function<void()>>& taskQueue, std::mutex& queueMutex,
                    std::condition_variable& condition, std::atomic<bool>& isActive);
    
    std::vector<std::thread> workers;
    
    // Ship processing threads
    std::queue<std::function<void()>> shipTaskQueue;
    std::mutex shipQueueMutex;
    std::condition_variable shipCondition;
    std::atomic<int> activeShipTasks{0};
    std::condition_variable shipTaskComplete;
    
    // Effects processing threads  
    std::queue<std::function<void()>> effectTaskQueue;
    std::mutex effectQueueMutex;
    std::condition_variable effectCondition;
    std::atomic<int> activeEffectTasks{0};
    std::condition_variable effectTaskComplete;
    
    std::atomic<bool> shutdown_flag{false};
};
