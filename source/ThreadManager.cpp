#include "ThreadManager.h"

ThreadManager::ThreadManager(size_t numThreads)
{
    // Create dedicated threads for ships and effects
    for(size_t i = 0; i < numThreads / 2; ++i)
    {
        workers.emplace_back([this]() { 
            workerLoop(shipTaskQueue, shipQueueMutex, shipCondition, shutdown_flag); 
        });
    }
    
    for(size_t i = 0; i < numThreads / 2; ++i)
    {
        workers.emplace_back([this]() { 
            workerLoop(effectTaskQueue, effectQueueMutex, effectCondition, shutdown_flag); 
        });
    }
}

ThreadManager::~ThreadManager()
{
    shutdown();
}

void ThreadManager::submitShipTask(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(shipQueueMutex);
        shipTaskQueue.push(task);
        ++activeShipTasks;
    }
    shipCondition.notify_one();
}

void ThreadManager::submitEffectTask(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(effectQueueMutex);
        effectTaskQueue.push(task);
        ++activeEffectTasks;
    }
    effectCondition.notify_one();
}

void ThreadManager::waitForShipsComplete()
{
    std::unique_lock<std::mutex> lock(shipQueueMutex);
    shipTaskComplete.wait(lock, [this]() { return activeShipTasks == 0 && shipTaskQueue.empty(); });
}

void ThreadManager::waitForEffectsComplete()
{
    std::unique_lock<std::mutex> lock(effectQueueMutex);
    effectTaskComplete.wait(lock, [this]() { return activeEffectTasks == 0 && effectTaskQueue.empty(); });
}

void ThreadManager::waitForAllComplete()
{
    waitForShipsComplete();
    waitForEffectsComplete();
}

void ThreadManager::shutdown()
{
    shutdown_flag = true;
    shipCondition.notify_all();
    effectCondition.notify_all();
    
    for(auto& thread : workers)
    {
        if(thread.joinable())
            thread.join();
    }
}

void ThreadManager::workerLoop(std::queue<std::function<void()>>& taskQueue, 
                               std::mutex& queueMutex,
                               std::condition_variable& condition,
                               std::atomic<bool>& isActive)
{
    while(!isActive)
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        condition.wait(lock, [&taskQueue, &isActive]() { 
            return !taskQueue.empty() || isActive; 
        });
        
        if(!taskQueue.empty())
        {
            auto task = taskQueue.front();
            taskQueue.pop();
            lock.unlock();
            
            task();
            
            if(&queueMutex == &shipQueueMutex)
                --activeShipTasks;
            else
                --activeEffectTasks;
        }
    }
}
