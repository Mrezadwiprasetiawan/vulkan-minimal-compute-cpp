#ifndef _VKMINCOMP_H
#define _VKMINCOMP_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>  
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

using namespace std;
using namespace vk;

namespace vkmincomp {

  enum DebugMode{
    VERBOSE,
    STANDARD,
    NO
  };
  
  class stdEng {
    private:
      DebugMode debugMode=DebugMode::NO;
      uint32_t width,height,depth;

      ApplicationInfo* appInfo;
      InstanceCreateInfo* instInfo;
      Instance* inst;
      PhysicalDevice* physdev;
      uint32_t queueFamIndex;
      DeviceCreateInfo* devInfo;
      Device* dev;
      vector<BufferCreateInfo*> inBuffInfos, outBuffInfos;
      vector<Buffer*> inBuffs, outBuffs;
      vector<MemoryRequirements*> inMemReqs, outMemReqs;
      vector<MemoryAllocateInfo*> inMemAllocInfos, outMemAllocInfos;
      vector<DeviceMemory*> inMems, outMems;
      vector<DescriptorSetLayoutBinding*> descSetLayBinds;
      uint32_t sumBind;
      DescriptorSetLayoutCreateInfo* descSetLayInfo;
      DescriptorSetLayout* descSetLay;
      ShaderModuleCreateInfo* shadModInfo;
      ShaderModule* shadMod;
      PipelineShaderStageCreateInfo* pipeShadStagInfo;
      PipelineLayoutCreateInfo* pipeLayInfo;
      PipelineLayout* pipeLay;
      ComputePipelineCreateInfo* compPipeInfo;
      Pipeline* pipe;
      DescriptorPoolSize* descPoolSize;
      DescriptorPoolCreateInfo* descPoolInfo;
      DescriptorPool* descPool;
      DescriptorSetAllocateInfo* descSetAllocInfo;
      vector<DescriptorSet*> descSets;
      vector<WriteDescriptorSet*> writedescsSet;
      vector<DescriptorBufferInfo*> descBuffInfos;
      vector<WriteDescriptorSet*> writeDescSets;
      CommandPoolCreateInfo* cmdPoolInfo;
      CommandPool* cmdPool;
      CommandBufferAllocateInfo* cmdBuffAllocInfo;
      vector<CommandBuffer*> cmdBuffs;
      Queue* queue;
      Fence* fence;
      SubmitInfo* submitInfo;
      Result* waitFenceRes;


      float priorities;
      vector<vector<void*>> inputs;
      vector<uint32_t> inBinding;
      vector<size_t> insizes;
      vector<vector<void*>> outputs;
      vector<size_t> outsizes;
      vector<uint32_t> bindings;
      uint32_t IOSetOffset,IOBindingOffset;
      char* filepath;
      char* entryPoint;
      uint64_t time;

      void createDevice();
      void createBuffer();
      void allocateMemory();
      void fillInputs();
      void loadShader();
      void createDescriptorSetLayout();
      void createPipelineLayout();
      void createPipeline();
      void createDescriptorPool();
      void allocateDescriptorSet();
      void createCommandBuffer();
      void sendCommand();
      void waitFence();


    public:
      stdEng(char* appname, uint32_t appvers, char* engname, uint32_t engvers);

      void setDebugMode(DebugMode debugMode);
      DebugMode getDebugMode();
      void setWorkgroupSize(uint32_t width,uint32_t height,uint32_t depth);

      void setPriorities(float priorities);
      void setInputs(vector<vector<void*>> inputs, vector<size_t> size);
      void setOutputs(vector<vector<void*>> outputs, vector<size_t> size);
      void setBinding(vector<uint32_t> bindings,uint32_t IOSetOffset,uint32_t IOBindingOffset);
      void setShaderFile(char* filepath);
      void setEntryPoint(char* entryPoint);
      void setWaitFenceFor(uint64_t time);

      void run();

      ~stdEng();
  };

}

#endif // _VKMINCOMP_H
