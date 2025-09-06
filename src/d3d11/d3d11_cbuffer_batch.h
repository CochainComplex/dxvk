#pragma once

#include "d3d11_buffer.h"
#include "d3d11_include.h"
#include "../dxbc/dxbc_util.h"
#include "../dxvk/dxvk_context.h"

namespace dxvk {

  /**
   * \brief Constant buffer batch entry
   * 
   * Represents a single constant buffer update
   * that will be batched with others.
   */
  struct D3D11ConstantBufferBatchEntry {
    DxbcProgramType   shaderStage;
    uint32_t          slot;
    D3D11Buffer*      buffer;
    uint32_t          offset;
    uint32_t          length;
  };

  /**
   * \brief Constant buffer batch manager
   * 
   * Batches multiple constant buffer updates into
   * a single command submission for better performance.
   */
  class D3D11ConstantBufferBatchManager {
    static constexpr uint32_t MaxBatchSize = 16;
    
  public:
    
    D3D11ConstantBufferBatchManager() { }
    
    /**
     * \brief Add a constant buffer update to the batch
     * 
     * \param [in] stage Shader stage
     * \param [in] slot Binding slot
     * \param [in] buffer Buffer to bind
     * \param [in] offset Offset in constants
     * \param [in] length Length in constants
     * \returns True if buffer was added to batch, false if batch was flushed
     */
    bool addBuffer(
            DxbcProgramType         stage,
            uint32_t                slot,
            D3D11Buffer*            buffer,
            uint32_t                offset,
            uint32_t                length) {
      // Check if we can batch this update
      if (m_batchCount >= MaxBatchSize) {
        return false;
      }
      
      // Add to batch
      m_batch[m_batchCount++] = D3D11ConstantBufferBatchEntry {
        stage, slot, buffer, offset, length
      };
      
      return true;
    }
    
    /**
     * \brief Check if batch has pending updates
     * 
     * \returns True if there are pending updates
     */
    bool hasPendingUpdates() const {
      return m_batchCount > 0;
    }
    
    /**
     * \brief Get the number of pending updates
     * 
     * \returns Number of pending updates
     */
    uint32_t getPendingCount() const {
      return m_batchCount;
    }
    
    /**
     * \brief Flush all pending updates
     * 
     * Applies all batched constant buffer updates
     * in a single command submission.
     * 
     * \param [in] emitCs Function to emit command stream
     */
    template<typename EmitFunc>
    void flush(EmitFunc&& emitCs) {
      if (m_batchCount == 0)
        return;
      
      // Capture all batch data for the lambda
      uint32_t count = m_batchCount;
      std::array<D3D11ConstantBufferBatchEntry, MaxBatchSize> batchCopy = m_batch;
      
      // Emit batched command
      emitCs([count, batchCopy] (DxvkContext* ctx) {
        for (uint32_t i = 0; i < count; i++) {
          const auto& entry = batchCopy[i];
          
          if (entry.buffer) {
            // Compute the actual slot ID based on shader stage
            uint32_t slotId = computeConstantBufferBinding(entry.shaderStage, entry.slot);
            VkShaderStageFlags stage = GetShaderStage(entry.shaderStage);
            
            // Bind the buffer slice
            DxvkBufferSlice bufferSlice = entry.buffer->GetBufferSlice(
              16 * entry.offset, 16 * entry.length);
            
            ctx->bindUniformBuffer(stage, slotId, std::move(bufferSlice));
          }
        }
      });
      
      // Clear the batch
      m_batchCount = 0;
    }
    
    /**
     * \brief Clear the batch without flushing
     * 
     * Discards all pending updates.
     */
    void clear() {
      m_batchCount = 0;
    }
    
  private:
    
    uint32_t m_batchCount = 0;
    std::array<D3D11ConstantBufferBatchEntry, MaxBatchSize> m_batch;
    
    /**
     * \brief Compute constant buffer binding slot
     * 
     * Computes the actual binding slot ID based on shader stage.
     */
    static uint32_t computeConstantBufferBinding(
            DxbcProgramType         stage,
            uint32_t                slot) {
      // This mimics the original computeConstantBufferBinding logic
      const uint32_t stageOffset = 16 * uint32_t(stage);
      return stageOffset + slot;
    }
    
    /**
     * \brief Get shader stage flags
     * 
     * Converts program type to Vulkan shader stage flags.
     */
    static VkShaderStageFlags GetShaderStage(DxbcProgramType type) {
      switch (type) {
        case DxbcProgramType::VertexShader:   return VK_SHADER_STAGE_VERTEX_BIT;
        case DxbcProgramType::HullShader:     return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case DxbcProgramType::DomainShader:   return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case DxbcProgramType::GeometryShader: return VK_SHADER_STAGE_GEOMETRY_BIT;
        case DxbcProgramType::PixelShader:    return VK_SHADER_STAGE_FRAGMENT_BIT;
        case DxbcProgramType::ComputeShader:  return VK_SHADER_STAGE_COMPUTE_BIT;
        default: return 0;
      }
    }
  };
  
  /**
   * \brief Optimized constant buffer batch mode
   * 
   * When enabled, constant buffer updates are batched
   * together to reduce command submission overhead.
   */
  enum class D3D11ConstantBufferBatchMode : uint32_t {
    Disabled = 0,   ///< No batching, immediate submission
    Enabled  = 1,   ///< Batch updates for better performance
    Adaptive = 2,   ///< Adaptively enable based on workload
  };

}