#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "dxvk_gpu_query.h"
#include "dxvk_device.h"

namespace dxvk {

  /**
   * \brief Async query collection request
   * 
   * Represents a query that needs to be collected asynchronously.
   */
  struct DxvkAsyncQueryRequest {
    Rc<DxvkGpuQuery>    query;
    VkQueryType         type;
    DxvkQueryData*      targetData;
    std::atomic<DxvkGpuQueryStatus>* status;
  };

  /**
   * \brief Async query collector
   * 
   * Collects query results asynchronously using VK_QUERY_RESULT_NO_WAIT
   * to avoid blocking the main thread on GPU operations.
   */
  class DxvkAsyncQueryCollector {
  public:
    
    DxvkAsyncQueryCollector(DxvkDevice* device)
    : m_device(device),
      m_running(false) { }
    
    ~DxvkAsyncQueryCollector() {
      stop();
    }
    
    /**
     * \brief Start the async collection thread
     */
    void start() {
      if (m_running.exchange(true))
        return;
      
      m_thread = std::thread([this] { collectLoop(); });
    }
    
    /**
     * \brief Stop the async collection thread
     */
    void stop() {
      if (!m_running.exchange(false))
        return;
      
      m_condition.notify_all();
      
      if (m_thread.joinable())
        m_thread.join();
    }
    
    /**
     * \brief Submit a query for async collection
     * 
     * \param [in] query The GPU query to collect
     * \param [in] type Query type
     * \param [in] targetData Target data buffer
     * \param [in] status Status flag to update
     */
    void submitQuery(
            const Rc<DxvkGpuQuery>&           query,
            VkQueryType                       type,
            DxvkQueryData*                    targetData,
            std::atomic<DxvkGpuQueryStatus>*  status) {
      std::unique_lock<std::mutex> lock(m_mutex);
      
      m_pendingQueries.push(DxvkAsyncQueryRequest {
        query, type, targetData, status
      });
      
      m_condition.notify_one();
    }
    
    /**
     * \brief Check if async collection is enabled
     * 
     * \returns True if async collection is active
     */
    bool isEnabled() const {
      return m_running.load();
    }
    
  private:
    
    DxvkDevice*                         m_device;
    std::atomic<bool>                   m_running;
    std::thread                         m_thread;
    
    std::mutex                          m_mutex;
    std::condition_variable             m_condition;
    std::queue<DxvkAsyncQueryRequest>   m_pendingQueries;
    std::vector<DxvkAsyncQueryRequest>  m_activeQueries;
    
    /**
     * \brief Main collection loop
     * 
     * Runs in a separate thread to collect query results.
     */
    void collectLoop() {
      const auto vk = m_device->vkd();
      
      while (m_running.load()) {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // Wait for queries or shutdown signal
        m_condition.wait(lock, [this] {
          return !m_pendingQueries.empty() || !m_running.load();
        });
        
        // Move pending queries to active list
        while (!m_pendingQueries.empty()) {
          m_activeQueries.push_back(std::move(m_pendingQueries.front()));
          m_pendingQueries.pop();
        }
        
        lock.unlock();
        
        // Process active queries
        processActiveQueries(vk);
      }
    }
    
    /**
     * \brief Process active queries
     * 
     * Attempts to collect results for all active queries
     * using VK_QUERY_RESULT_NO_WAIT flag.
     */
    void processActiveQueries(const Rc<vk::DeviceFn>& vk) {
      auto it = m_activeQueries.begin();
      
      while (it != m_activeQueries.end()) {
        DxvkQueryData tmpData = { };
        
        // Get query handle
        std::pair<VkQueryPool, uint32_t> handle = it->query->getQuery();
        
        // Try to get results without waiting
        VkResult result = vk->vkGetQueryPoolResults(
          vk->device(), handle.first, handle.second, 1,
          sizeof(DxvkQueryData), &tmpData,
          sizeof(DxvkQueryData), 
          VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
        
        if (result == VK_SUCCESS) {
          // Successfully retrieved results
          mergeQueryData(it->type, it->targetData, &tmpData);
          
          // Update status
          if (it->status)
            it->status->store(DxvkGpuQueryStatus::Available);
          
          // Remove from active list
          it = m_activeQueries.erase(it);
        } else if (result == VK_NOT_READY) {
          // Query still pending, keep in active list
          ++it;
        } else {
          // Query failed
          if (it->status)
            it->status->store(DxvkGpuQueryStatus::Failed);
          
          // Remove from active list
          it = m_activeQueries.erase(it);
        }
        
        // Yield periodically to avoid monopolizing CPU
        if ((it - m_activeQueries.begin()) % 16 == 0) {
          std::this_thread::yield();
        }
      }
      
      // Sleep briefly if no queries were ready
      if (!m_activeQueries.empty()) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
    }
    
    /**
     * \brief Merge query data
     * 
     * Merges retrieved query data into the target buffer.
     */
    void mergeQueryData(
            VkQueryType         type,
            DxvkQueryData*      target,
      const DxvkQueryData*      source) {
      switch (type) {
        case VK_QUERY_TYPE_OCCLUSION:
          target->occlusion.samplesPassed += source->occlusion.samplesPassed;
          break;
          
        case VK_QUERY_TYPE_TIMESTAMP:
          target->timestamp.time = source->timestamp.time;
          break;
          
        case VK_QUERY_TYPE_PIPELINE_STATISTICS:
          target->statistic.iaVertices       += source->statistic.iaVertices;
          target->statistic.iaPrimitives     += source->statistic.iaPrimitives;
          target->statistic.vsInvocations    += source->statistic.vsInvocations;
          target->statistic.gsInvocations    += source->statistic.gsInvocations;
          target->statistic.gsPrimitives     += source->statistic.gsPrimitives;
          target->statistic.clipInvocations  += source->statistic.clipInvocations;
          target->statistic.clipPrimitives   += source->statistic.clipPrimitives;
          target->statistic.fsInvocations    += source->statistic.fsInvocations;
          target->statistic.tcsPatches       += source->statistic.tcsPatches;
          target->statistic.tesInvocations   += source->statistic.tesInvocations;
          target->statistic.csInvocations    += source->statistic.csInvocations;
          break;
          
        case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
          target->xfbStream.primitivesWritten += source->xfbStream.primitivesWritten;
          target->xfbStream.primitivesNeeded  += source->xfbStream.primitivesNeeded;
          break;
          
        default:
          Logger::err(str::format("DXVK: Unhandled query type in async collector: ", type));
          break;
      }
    }
  };

}