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

  struct IObinding{
    vector<uint32_t> Sets;
    vector<uint32_t> Bindings;
  };
  
  class stdEng {
    private:
      DebugMode debugMode=DebugMode::NO;

      Instance* inst;
      PhysicalDevice* physdev;
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

      float priorities;
      vector<vector<void*>> inputs;
      vector<size_t> insizes;
      vector<vector<void*>> outputs;
      vector<size_t> outsizes;
      vector<uint32_t> bindings;
      char* filepath;
      char* entryPoint;

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


    public:
      stdEng(char* appname, uint32_t appvers, char* engname, uint32_t engvers);

      void setDebugMode(DebugMode debugMode);
      DebugMode getDebugMode();

      void setPriorities(float priorities);
      void setInputs(vector<vector<void*>> inputs, vector<size_t> size);
      void setOutputs(vector<vector<void*>> outputs, vector<size_t> size);
      void setBinding(vector<uint32_t> bindings);
      void setShaderFile(char* filepath);
      void setEntryPoint(char* entryPoint);

      void run();

      ~stdEng();
  };

}

#endif // _VKMINCOMP_H
