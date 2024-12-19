// Copyright 2024 M Reza Dwi Prasetiawan
// License under GNU GPL v3.0
// for more information visit https://www.gnu.org/licenses/gpl-3.0.html
#include <fstream>
#include <iostream>
#include <vkmincomp.hxx>

using namespace std;
using namespace vkmincomp;

// metode public
/* the one and only one constructor to create an instance
 *
 * @param appname The name of your Application Instance
 * @param appvers Version of your Application
 * @param engname The name of your Engine;
 * @param engvers Version of your Engine;
 *
 * apivers use available and the higher version API of the vulkan driver
 */
stdEng::stdEng(const char *appname, uint32_t appvers, const char *engname,
               uint32_t engvers) {
  uint32_t apivers = enumerateInstanceVersion();
  cout << appname << endl;
  cout << appvers << endl;
  cout << engname << endl;
  cout << engvers << endl;
  cout << apivers << endl;
  ApplicationInfo appInfo(appname, appvers, engname, engvers, apivers);
  this->appInfo = appInfo;
  InstanceCreateInfo instInfo({}, &appInfo, 0, {}, 0, {});
  this->instInfo = instInfo;
  Instance inst = createInstance(instInfo);
  this->inst = inst;
}

/* Actually Priorities are array but we only use 1 device
 * @param priority priority for the device
 */
void stdEng::setPriority(float priority) { this->priority = priority; }

/* For inputs stuff
 *
 * @param inputs vector from vector of the input arrays
 * @param size total size of the each vector in byte
 */
void stdEng::setInputs(vector<vector<void *>> inputs, vector<size_t> size) {
  this->inputs = inputs;
  this->insizes = size;
}

// Same as inputs
void stdEng::setOutputs(vector<vector<void *>> outputs, vector<size_t> size) {
  this->outputs = outputs;
  this->outsizes = size;
}

/* Set up binding in the shader
 *
 * Using this library means you must group the set&binding of inputs together
 * before finally set&binding the outputs.
 *
 * @param bindings The size of the bindings is the number of sets, while the
 * elements represent the number of bindings in each set.
 * @param IOSetOffset This is the offset value of the output set relative to the
 * input. For example, if the first outputs is in set 5, the IOSetOffset should
 * be 5.
 * @param IOBindingOffset Similar to the previous one, if the first output is in
 * binding n, the offset should also be n.
 */
void stdEng::setBindings(vector<uint32_t> bindings, uint32_t IOSetOffset,
                         uint32_t IOBindingOffset) {
  this->bindings = bindings;
  this->IOSetOffset = IOSetOffset;
  this->IOBindingOffset = IOBindingOffset;
}

/* The shader used, as far as I know, is SPIR-V, which is usually compiled from
 * HLSL or GLSL.
 *
 * @param filepath The path to the SPIR-V shader file, relative to where this
 * class is used.
 */
void stdEng::setShaderFile(const char *filepath) { this->filepath = filepath; }

/* Set the name of the m ain function or entry point in the shader
 * In shaders, you can have multiple functions, so you need to know where the
 * main function is.
 *
 * @param entryPoint The name of the function as the entry point in the shader.
 */
void stdEng::setEntryPoint(const char *entryPoint) {
  this->entryPoint = entryPoint;
}

/* setWorkgroupSize is the size of the workgroup in the shader
 *
 * @param width The width of the workgroup
 * @param height The height of the workgroup
 * @param depth The depth of the workgroup
 */
void stdEng::setWorkgroupSize(uint32_t width, uint32_t height, uint32_t depth) {
  this->width = width;
  this->height = height;
  this->depth = depth;
}

/* Iam using Fence for mark  if our calculation finish
 *
 * @param time timeout for waiting the fence
 */
void stdEng::setWaitFenceFor(uint64_t time) { this->time = time; }

// get current debug mode
DebugMode stdEng::getDebugMode() { return this->debugMode; }

/* set the debug mode
 *
 * @param debugMode the debug mode that you want to use
 * @see DebugMode
 */
void stdEng::setDebugMode(DebugMode debugMode) { this->debugMode = debugMode; }
// akhir dari metode public

// metode private
// Create device from the instance
void stdEng::createDevice() {
  if (!this->inst) {
    cout << "Instance not created yet!" << endl;
    exit(EXIT_FAILURE);
  }
  vector<PhysicalDevice> physdevs = this->inst.enumeratePhysicalDevices();
  if (physdevs.empty()) {
    cout << "No Vulkan driver found!" << endl;
    exit(EXIT_FAILURE);
  }
  this->physdev = PhysicalDevice(physdevs[0]);
  vector<QueueFamilyProperties> qFamProps =
      this->physdev.getQueueFamilyProperties();
  auto qFamProp = find_if(
      qFamProps.begin(), qFamProps.end(), [](QueueFamilyProperties qFamPropsT) {
        return qFamPropsT.queueFlags & QueueFlagBits::eCompute;
      });
  this->queueFamIndex = distance(qFamProps.begin(), qFamProp);
  if (this->queueFamIndex == qFamProps.size()) {
    cout << "No Queue Family found!" << endl;
    exit(EXIT_FAILURE);
  }
  DeviceQueueCreateInfo devQInfo(DeviceQueueCreateFlags(), this->queueFamIndex,
                                 1, &this->priority);
  this->devQInfo = devQInfo;
  DeviceCreateInfo devInfo({}, devQInfo);
  this->devInfo = devInfo;
  Device dev = physdev.createDevice(devInfo);
  this->dev = dev;
}

// creating buffers for input and output
void stdEng::createBuffer() {
  if (this->insizes.empty() || this->outsizes.empty()) {
    cout << "No input or output size has been set!" << endl;
    exit(EXIT_FAILURE);
  }
  for (size_t insize : this->insizes) {
    BufferCreateInfo inBuffInfo(BufferCreateFlags(), insize,
                                BufferUsageFlagBits::eStorageBuffer,
                                SharingMode::eExclusive);
    this->inBuffInfos.push_back(inBuffInfo);
    Buffer inbuff = this->dev.createBuffer(inBuffInfo);
    this->inBuffs.push_back(inbuff);
  }
  for (size_t outsize : this->outsizes) {
    BufferCreateInfo outBuffInfo(BufferCreateFlags(), outsize,
                                 BufferUsageFlagBits::eStorageBuffer,
                                 SharingMode::eExclusive);
    this->outBuffInfos.push_back(outBuffInfo);
    Buffer outbuff = this->dev.createBuffer(outBuffInfo);
    this->outBuffs.push_back(outbuff);
  }
}

// Memory allocations for input and output
void stdEng::allocateMemory() {
  for (Buffer inbuff : this->inBuffs) {
    MemoryRequirements inMemReq = this->dev.getBufferMemoryRequirements(inbuff);
    this->inMemReqs.push_back(inMemReq);
  }
  for (Buffer outbuff : this->outBuffs) {
    MemoryRequirements outMemReq =
        this->dev.getBufferMemoryRequirements(outbuff);
    this->outMemReqs.push_back(outMemReq);
  }
  PhysicalDeviceMemoryProperties heapMemProp =
      this->physdev.getMemoryProperties();
  DeviceSize heapIndex = uint32_t(~0);
  for (uint32_t i = 0; i < heapMemProp.memoryTypeCount; ++i) {
    if ((heapMemProp.memoryTypes[i].propertyFlags &
         MemoryPropertyFlagBits::eHostVisible) &&
        (heapMemProp.memoryTypes[i].propertyFlags &
         MemoryPropertyFlagBits::eHostCoherent)) {
      heapIndex = i;
      break;
    }
  }
  if (heapIndex == uint32_t(~0))
    throw runtime_error("No heap found");
  int inCount = 0, outCount = 0;
  for (MemoryRequirements inMemReq : this->inMemReqs) {
    inCount++;
    MemoryAllocateInfo inMemAllocInfo(inMemReq.size, heapIndex);
    this->inMemAllocInfos.push_back(inMemAllocInfo);
    DeviceMemory inmem = this->dev.allocateMemory(inMemAllocInfo);
    this->inMems.push_back(inmem);
  }
  for (MemoryRequirements outMemReq : this->outMemReqs) {
    outCount++;
    MemoryAllocateInfo outMemAllocInfo(outMemReq.size, heapIndex);
    this->outMemAllocInfos.push_back(outMemAllocInfo);
    DeviceMemory outmem = this->dev.allocateMemory(outMemAllocInfo);
    this->outMems.push_back(outmem);
  }
  cout << "inCount and outCount = " << inCount << outCount << endl;
}

// fill the memories with our inputs data in byte
void stdEng::fillInputs() {
  if (this->inMems.empty() || this->inputs.empty()) {
    if (this->inMems.empty())
      cout << "No input memory has been set!" << endl;
    if (this->inputs.empty())
      cout << "No input data has been set!" << endl;
    exit(EXIT_FAILURE);
  }
  for (size_t i = 0; i < this->inputs.size(); ++i) {
    void *inPtr =
        this->dev.mapMemory(this->inMems.at(i), 0, this->insizes.at(i));
    memcpy(inPtr, this->inputs.at(i).data(), this->insizes.at(i));
    this->dev.unmapMemory(this->inMems.at(i));
  }
}

// load SPIR-V shader
void stdEng::loadShader() {
  vector<char> shaderRaw;
  ifstream shaderFile(this->filepath, ios::ate | ios::binary);
  if (!shaderFile)
    throw runtime_error("Failed to open shader file");
  size_t shaderSize = shaderFile.tellg();
  shaderFile.seekg(0);
  shaderRaw.resize(shaderSize);
  shaderFile.read(shaderRaw.data(), shaderSize);
  shaderFile.close();

  ShaderModuleCreateInfo shadModInfo(
      ShaderModuleCreateFlags(), shaderSize,
      reinterpret_cast<const uint32_t *>(shaderRaw.data()));
  ShaderModule shadMod = this->dev.createShaderModule(shadModInfo);
  this->shadModInfo = shadModInfo;
  this->shadMod = shadMod;
}

// Creating DescriptorSetLayout for binding to the Shader
void stdEng::createDescriptorSetLayout() {
  uint32_t sumBind = 0;
  if (this->bindings.empty()) {
    cout << "No binding has been set!" << endl;
    exit(EXIT_FAILURE);
  }
  for (uint32_t setI = 0; setI < this->bindings.size(); ++setI) {
    sumBind += this->bindings.at(setI);
    for (uint32_t bindI = 0; bindI < this->bindings.at(setI); ++bindI) {
      DescriptorSetLayoutBinding descSetLayBind(
          bindI, DescriptorType::eStorageBuffer, setI,
          ShaderStageFlagBits::eCompute);
      this->descSetLayBinds.push_back(descSetLayBind);
    }
  }
  // save the amount of the binding which used later in DescriptorPool
  this->sumBind = sumBind;
  vector<DescriptorSetLayoutBinding> descSetLayBinds;
  for (DescriptorSetLayoutBinding descSetLayBind : this->descSetLayBinds)
    descSetLayBinds.push_back(descSetLayBind);
  DescriptorSetLayoutCreateInfo descSetLayInfo(DescriptorSetLayoutCreateFlags(),
                                               descSetLayBinds);
  this->descSetLayInfo = descSetLayInfo;
  DescriptorSetLayout descSetLay =
      this->dev.createDescriptorSetLayout(descSetLayInfo);
  this->descSetLay = descSetLay;
}

// Create Pipeline Layout for binding to the Pipeline
void stdEng::createPipelineLayout() {
  PipelineLayoutCreateInfo pipeLayInfo(PipelineLayoutCreateFlags(),
                                       this->descSetLay);
  PipelineLayout pipeLay = this->dev.createPipelineLayout(pipeLayInfo);
  this->pipeLayInfo = pipeLayInfo;
  this->pipeLay = pipeLay;
}

// Create Pipeline for the Shader
void stdEng::createPipeline() {
  PipelineShaderStageCreateInfo pipeShadStagInfo(
      PipelineShaderStageCreateFlags(), ShaderStageFlagBits::eCompute,
      (this->shadMod), this->entryPoint);
  this->pipeShadStagInfo = pipeShadStagInfo;
  PipelineCache pipeCache =
      this->dev.createPipelineCache(PipelineCacheCreateInfo());
  ComputePipelineCreateInfo compPipeInfo(PipelineCreateFlags(),
                                         pipeShadStagInfo, this->pipeLay);
  this->compPipeInfo = compPipeInfo;
  ResultValue res = this->dev.createComputePipeline(pipeCache, compPipeInfo);
  if (res.result != Result::eSuccess)
    throw runtime_error("Failed to create pipeline");
  Pipeline pipe =
      this->dev.createComputePipeline(pipeCache, compPipeInfo).value;
  this->pipe = pipe;
}

// Create Descriptor Pool for binding to the DescriptorSet
void stdEng::createDescriptorPool() {
  /* The second parameter is the number of descriptors per type. Since there is
   * only one type here, this is the total number of bindings.
   */
  DescriptorPoolSize descPoolSize(DescriptorType::eStorageBuffer,
                                  this->sumBind);
  this->descPoolSize = descPoolSize;
  DescriptorPoolCreateInfo descPoolInfo(DescriptorPoolCreateFlags(),
                                        this->bindings.size(), descPoolSize);
  this->descPoolInfo = descPoolInfo;
  DescriptorPool descPool = this->dev.createDescriptorPool(descPoolInfo);
  this->descPool = descPool;
}

// DescriptorSet allocation for binding to the DescriptorSetLayout
void stdEng::allocateDescriptorSet() {
  DescriptorSetAllocateInfo descSetAllocInfo(
      this->descPool, this->bindings.size(), &this->descSetLay);
  this->descSetAllocInfo = descSetAllocInfo;
  this->descSets = this->dev.allocateDescriptorSets(descSetAllocInfo);
  vector<DescriptorBufferInfo> descBuffInfos;
  for (uint32_t i = 0; i < this->insizes.size(); ++i) {
    DescriptorBufferInfo descbuffinfo(this->inBuffs.at(i), 0,
                                      this->inBuffInfos.at(i).size);
    descBuffInfos.push_back(descbuffinfo);
  }
  for (uint32_t i = 0; i < this->outsizes.size(); ++i) {
    DescriptorBufferInfo descbuffinfo(this->outBuffs.at(i), 0,
                                      this->outBuffInfos.at(i).size);
    descBuffInfos.push_back(descbuffinfo);
  }
  vector<WriteDescriptorSet> writeDescSets;
  uint32_t p = 0;
  if (IOBindingOffset != 0) {
    for (uint32_t i = 0; i <= IOSetOffset; ++i) {
      if (i != IOSetOffset) {
        for (uint32_t j = 0; j < bindings.at(i); ++j) {
          writeDescSets.push_back({descSets.at(i), j, 0, i,
                                   DescriptorType::eStorageBuffer, nullptr,
                                   &descBuffInfos.at(p)});
          p++;
        }
      }
    }
  } else {
    for (uint32_t i = 0; i < IOSetOffset; ++i) {
      for (uint32_t j = 0; j < bindings.at(i); ++j) {
        writeDescSets.push_back({descSets.at(i), j, 0, i,
                                 DescriptorType::eStorageBuffer, nullptr,
                                 &descBuffInfos.at(p)});
        p++;
      }
    }
  }
  this->dev.updateDescriptorSets(writeDescSets, nullptr);
}

// Commamd Buffer Creation for sending the command
void stdEng::createCommandBuffer() {
  CommandPoolCreateInfo cmdPoolInfo(CommandPoolCreateFlags(),
                                    this->queueFamIndex);
  this->cmdPoolInfo = cmdPoolInfo;
  CommandPool cmdPool = this->dev.createCommandPool(cmdPoolInfo);
  this->cmdPool = cmdPool;
  CommandBufferAllocateInfo cmdBuffAllocInfo(cmdPool,
                                             CommandBufferLevel::ePrimary, 1);
  this->cmdBuffAllocInfo = cmdBuffAllocInfo;
  vector<CommandBuffer> cmdBuffs =
      this->dev.allocateCommandBuffers(cmdBuffAllocInfo);
  for (CommandBuffer cmdbuff : cmdBuffs)
    this->cmdBuffs.push_back(cmdbuff);
}

// Send Command with CommandBuffer to the GPU
void stdEng::sendCommand() {
  CommandBufferBeginInfo cmdBuffBeginInfo(
      CommandBufferUsageFlagBits::eOneTimeSubmit);
  this->cmdBuffBeginInfo = cmdBuffBeginInfo;
  CommandBuffer cmdBuff = this->cmdBuffs.front();

  cmdBuff.begin(cmdBuffBeginInfo);
  cmdBuff.bindPipeline(PipelineBindPoint::eCompute, this->pipe);
  cmdBuff.bindDescriptorSets(PipelineBindPoint::eCompute, this->pipeLay, 0,
                             this->descSets, {});
  cmdBuff.dispatch(this->width, this->height, this->depth);
  cmdBuff.end();
}

// wait gpu proccess with Fence to mark if finish or not
void stdEng::waitFence() {
  this->queue = this->dev.getQueue(this->queueFamIndex, 0);
  this->fence = this->dev.createFence(FenceCreateInfo());
  SubmitInfo submitInfo(0, nullptr, nullptr, 1, this->cmdBuffs.data());
  this->submitInfo = submitInfo;
  queue.submit({submitInfo}, fence);
  Result waitFenceRes = this->dev.waitForFences({fence}, true, this->time);
  this->waitFenceRes = waitFenceRes;
}

// metode public
// the main method to running all previous methods in order
void stdEng::run() {
  if (this->debugMode == DebugMode::VERBOSE) {
    cout << "Instance was created with :" << endl;
    ;
    cout << "\tApplicationInfo" << endl;
    cout << "\t\tApplication Name = " << this->appInfo.pApplicationName << ","
         << endl;
    cout << "\t\tApplication Version = " << this->appInfo.applicationVersion
         << "," << endl;
    cout << "\t\tEngine Name = " << this->appInfo.pEngineName << "," << endl;
    cout << "\t\tEngine Version = " << this->appInfo.engineVersion << ","
         << endl;
    cout << "\t\tApi Version used = " << this->appInfo.apiVersion << ","
         << endl;
    cout << "\tInstanceCreateInfo" << endl;
    cout << "\t\tInstance Flags = " << to_string(this->instInfo.flags) << ","
         << endl;
    cout << "\t\tAmount of active layer = " << this->instInfo.enabledLayerCount
         << "," << endl;
    cout << "\t\tActive layers" << endl;
    for (uint32_t i = 0; i < this->instInfo.enabledLayerCount; ++i)
      cout << "\t\t\t" << this->instInfo.ppEnabledLayerNames[i] << endl;
    cout << "\t\tAmount of active extensions"
         << this->instInfo.enabledExtensionCount << endl;
    cout << "\t\tActive extensions" << endl;
    for (uint32_t i = 0; i < this->instInfo.enabledExtensionCount; ++i)
      cout << "\t\t\t" << this->instInfo.ppEnabledExtensionNames[i] << ","
           << endl;
    cout << "start creating logical device" << endl;
  }

  this->createDevice();

  if (!(this->debugMode == DebugMode::NO)) {
    cout << "logical device created" << endl;
    if (this->debugMode == DebugMode::VERBOSE) {
      cout << "\tCreateInfo" << endl;
      cout << "\t\tDevice Flags" << to_string(this->devInfo.flags) << ","
           << endl;
      cout << "\t\tDevice Queue Flags" << to_string(this->devQInfo.flags) << ","
           << endl;
      cout << "\t\t Queue Family Index" << this->devQInfo.queueFamilyIndex
           << "," << endl;
      cout << "\t\tQueue Count = " << this->devQInfo.queueCount << endl;
      cout << "\\Queues" << endl;
      for (uint32_t i = 0; i < this->devQInfo.queueCount; ++i)
        cout << "\t\t\t" << this->devQInfo.pQueuePriorities[i] << "," << endl;
    }
    cout << "Start creating Buffers" << endl;
  }

  this->createBuffer();

  if (!(this->debugMode == DebugMode::NO)) {
    if (this->debugMode == DebugMode::VERBOSE) {
      cout << "\tInput Buffer Infos" << endl;
      for (BufferCreateInfo buffInfo : this->inBuffInfos) {
        cout << "\t\tBuffer Create Flags = " << to_string(buffInfo.flags)
             << endl;
        cout << "\t\tBuffer Size = " << buffInfo.size << endl;
        cout << "\t\tBuffer Usage = " << to_string(buffInfo.usage) << endl;
        cout << "\t\tBuffer Sharing mode = " << to_string(buffInfo.sharingMode)
             << endl;
        cout << "\t\tQueue Family Index Count = "
             << buffInfo.queueFamilyIndexCount << endl;
        cout << "\t\tQueue Family Indices" << endl;
        for (uint32_t i = 0; i < buffInfo.queueFamilyIndexCount; ++i)
          cout << "\t\t\t" << buffInfo.pQueueFamilyIndices << endl;
        cout << endl;
      }
    }
    cout << "Start allocate memory" << endl;
  }

  this->allocateMemory();

  if (!(this->debugMode == DebugMode::NO)) {
    cout << "Memory allocated!" << endl;
    if (this->debugMode == DebugMode::VERBOSE) {
      cout << "\tInput Memory Requirements" << endl;
      for (MemoryRequirements memReq : this->inMemReqs) {
        cout << "\t\tSize = " << memReq.size << endl;
        cout << "\t\tAlignment = " << memReq.alignment << endl;
        cout << "\t\tMemory Type Bits = " << memReq.memoryTypeBits << endl;
      }
      cout << "\tOutput Memory Requirements" << endl;
      for (MemoryRequirements memReq : this->outMemReqs) {
        cout << "\t\tSize = " << memReq.size << endl;
        cout << "\t\tAlignment = " << memReq.alignment << endl;
        cout << "\t\tMemory Type Bits = " << memReq.memoryTypeBits << endl;
      }
      cout << "\tPhysical Device Memory Properties" << endl;
      cout << "\t\tMemory Type Count = "
           << this->physdev.getMemoryProperties().memoryTypeCount << endl;
      cout << "\t\tMemory Heap Count = "
           << this->physdev.getMemoryProperties().memoryHeapCount << endl;
    }
    cout << "Start filling inputs" << endl;
  }

  this->fillInputs();

  if (!(this->debugMode == DebugMode::NO)) {
    cout << "Filling Inputs memories successfully!" << endl;
    if (this->debugMode == DebugMode::VERBOSE) {
      if (this->inMems.size() != this->inputs.size()) {
        cout << "\tWarning! inputs and input memories vector has differ size"
             << endl;
        cout << "\tThis mean you were wrongly passing inputs size" << endl;
      }
      size_t minSize =
          inMems.size() > inputs.size() ? inMems.size() : inputs.size();
      for (size_t i = 0; i < minSize; ++i) {
        cout << "\tinput memory " << i << "will be filled by" << endl;
        cout << "\tinput " << i << "in byte:" << endl;
        const char *inputBytes = reinterpret_cast<char *>(&inputs.at(i));
        for (size_t j = 0; j < this->insizes.at(i) / this->inputs.at(i).size();
             ++j)
          cout << "\t\t" << static_cast<int>(inputBytes[j]);
        cout << endl;
      }
    }
    cout << "Start creating descriptor set layout!" << endl;
  }

  this->createDescriptorSetLayout();

  if (!(this->debugMode == DebugMode::NO)) {
    cout << "Descriptor Set Layout created!" << endl;
    if (this->debugMode == DebugMode::VERBOSE) {
      cout << "\tTotal Descriptor Set Layout Binding"
           << this->descSetLayBinds.size() << endl;
      cout << "\tExisting Descriptor Set Layout Binding:" << endl;
      for (DescriptorSetLayoutBinding descSetLayBind : descSetLayBinds) {
        cout << "\t\tBinding = " << descSetLayBind.binding << endl;
        cout << "\t\tDescriptor type = "
             << to_string(descSetLayBind.descriptorType) << endl;
        cout << "\t\tDescriptor count = " << descSetLayBind.descriptorCount
             << endl;
        cout << "\t\tDescriptor Stage Flags"
             << to_string(descSetLayBind.stageFlags) << endl;
        cout << "\t\tSamplers=null" << endl; // we dont need
      }
      cout << "\tTotal Binding = " << this->sumBind << endl;
      cout << "\tDescriptor Set Layout Create Info" << endl;
      cout << "\t\tBinding count = " << this->descSetLayInfo.bindingCount
           << endl;
      cout << "\t\tFlags = " << to_string(this->descSetLayInfo.flags) << endl;
    }
    cout << "Start creating Pipeline Layout!" << endl;
  }

  this->createPipelineLayout();

  if (!(this->debugMode == DebugMode::NO)) {
    cout << "Pipeline Layout created!" << endl;
    if (this->debugMode == DebugMode::VERBOSE) {
      cout << "\tFlags = " << to_string(this->pipeLayInfo.flags) << endl;
      cout << "Set Layout Count" << this->pipeLayInfo.setLayoutCount << endl;
      /* pSetLayout alway point to Descriptor Set Layout that's created before
       * push Range Count and pushConstant not implemented yet
       */
    }
    cout << "Start creating pipeline" << endl;
  }

  this->createPipeline();

  if (!(this->debugMode == DebugMode::NO)) {
    cout << "Pipeline created!" << endl;
    if (this->debugMode == DebugMode::VERBOSE) {
      cout << "\tFlags = " << to_string(this->compPipeInfo.flags) << endl;
      cout << "\tCompute Pipeline Shader Stage Create Info:" << endl;
      cout << "\t\tFlags = " << to_string(this->compPipeInfo.stage.flags)
           << endl;
      cout << "\t\tStage = " << to_string(this->compPipeInfo.stage.stage)
           << endl;
      cout << "\t\tShader module from = " << this->filepath << endl;
      // specializationInfo we dont need this i think
      // pipeline layout using previous Pipeline Layout
      // BasePipelineHandle is not used;
      // BasePipelineIndex is also not ued;
    }
    cout << "Start creating Descriptor Pool" << endl;
  }

  this->createDescriptorPool();

  if (!(this->debugMode == DebugMode::NO)) {
    cout << "Descriptor Pool created!" << endl;
    if (this->debugMode == DebugMode::VERBOSE) {
      cout << "\tFlags = " << to_string(this->descPoolInfo.flags) << endl;
      cout << "\tPool Size Count = " << this->descPoolInfo.poolSizeCount
           << endl;
      if (this->descPoolInfo.poolSizeCount)
        cout << "\tPoolsizes" << endl;
      for (uint32_t i = 0; i < this->descPoolInfo.poolSizeCount; ++i) {
        cout << "\t\tPool size" << i << endl;
        cout << "\t\t\tType = "
             << to_string(this->descPoolInfo.pPoolSizes[i].type) << endl;
        cout << "\t\t\tCount = "
             << this->descPoolInfo.pPoolSizes[i].descriptorCount << endl;
        for (uint32_t j = 0;
             j < this->descPoolInfo.pPoolSizes[i].descriptorCount; ++j)
          cout << "\t\t\t\t" << this->descPoolInfo.pPoolSizes[i].descriptorCount
               << endl;
      }
    }
  }
  this->allocateDescriptorSet();

  if (!(this->debugMode == DebugMode::NO)) {
    cout << "Descriptor Set allocated!" << endl;
    if (this->debugMode == DebugMode::VERBOSE) {
      cout << "\tDescriptor Set Allocate Info" << endl;
      cout << "\t\tDescriptor Pool = " << this->descSetAllocInfo.descriptorPool
           << endl;
      cout << "\t\tDescriptor Set Count = "
           << this->descSetAllocInfo.descriptorSetCount << endl;
      cout << "\t\tDescriptor Set Layouts" << endl;
      for (uint32_t i = 0; i < this->descSetAllocInfo.descriptorSetCount; ++i)
        cout << "\t\t\t" << this->descSetAllocInfo.pSetLayouts[i] << endl;
    }
  }
  this->createCommandBuffer();

  if (!(this->debugMode == DebugMode::NO)) {
    cout << "Command Buffer created!" << endl;
    if (this->debugMode == DebugMode::VERBOSE) {
      cout << "\tCommand Pool Info" << endl;
      cout << "\t\tFlags = " << to_string(this->cmdPoolInfo.flags) << endl;
      cout << "\t\tQueue Family Index = " << this->cmdPoolInfo.queueFamilyIndex
           << endl;
      cout << "\tCommand Buffer Allocate Info" << endl;
      cout << "\t\tCommand Pool = " << this->cmdBuffAllocInfo.commandPool
           << endl;
      cout << "\t\tCommand Buffer Level = "
           << to_string(this->cmdBuffAllocInfo.level) << endl;
      cout << "\t\tCommand Buffer Count = "
           << this->cmdBuffAllocInfo.commandBufferCount << endl;
    }
  }
  this->sendCommand();

  if (!(this->debugMode == DebugMode::NO)) {
    cout << "Command sent!" << endl;
    if (this->debugMode == DebugMode::VERBOSE) {
      cout << "\tCommand Buffer Begin Info" << endl;
      cout << "\t\tCommand Buffer Usage = "
           << to_string(this->cmdBuffAllocInfo.level) << endl;
      cout << "\t\tCommand Buffer Count = "
           << this->cmdBuffAllocInfo.commandBufferCount << endl;
    }
  }
  this->waitFence();

  if (!(this->debugMode == DebugMode::NO)) {
    cout << "Fence waited!" << endl;
    if (this->debugMode == DebugMode::VERBOSE) {
      cout << "\tFence waited for = " << this->time << endl;
      cout << "\tFence result = " << to_string(this->waitFenceRes) << endl;
    }
  }
}

// destructor
stdEng::~stdEng() {
  if (!(this->debugMode == DebugMode::NO))
    cout << "Destroying fence" << endl;
  this->dev.destroyFence(this->fence);

  if (!(this->debugMode == DebugMode::NO))
    cout << "Reset CommandPool" << endl;
  this->dev.resetCommandPool(this->cmdPool);

  if (!(this->debugMode == DebugMode::NO))
    cout << "Destroying Descriptor Set Layout" << endl;
  this->dev.destroyDescriptorSetLayout(this->descSetLay);

  if (!(this->debugMode == DebugMode::NO))
    cout << "Destroying PipelineLayout" << endl;
  this->dev.destroyPipelineLayout(this->pipeLay);

  if (!(this->debugMode == DebugMode::NO))
    cout << "Destroying Shader Moduld" << endl;
  this->dev.destroyShaderModule(this->shadMod);

  if (!(this->debugMode == DebugMode::NO))
    cout << "Destroying Descriptor Pool" << endl;
  this->dev.destroyDescriptorPool(this->descPool);

  if (!(this->debugMode == DebugMode::NO))
    cout << "Destroying CommandPool" << endl;
  this->dev.destroyCommandPool(this->cmdPool);

  if (!(this->debugMode == DebugMode::NO))
    cout << "Free Memory input and output" << endl;
  for (DeviceMemory mem : this->inMems)
    this->dev.freeMemory(mem);
  for (DeviceMemory mem : this->outMems)
    this->dev.freeMemory(mem);

  if (!(this->debugMode == DebugMode::NO))
    cout << "Destroying buffer" << endl;
  for (Buffer buff : this->inBuffs)
    this->dev.destroyBuffer(buff);
  for (Buffer buff : this->outBuffs)
    this->dev.destroyBuffer(buff);

  if (!(this->debugMode == DebugMode::NO))
    cout << "Destroying Device" << endl;
  this->dev.destroy();

  if (!(this->debugMode == DebugMode::NO))
    cout << "Destroying Instance" << endl;
  this->inst.destroy();
}
