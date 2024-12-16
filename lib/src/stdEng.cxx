#include "vkmincomp.hxx"
#include <cstdint>
#include <fstream>
#include <iostream>

using namespace std;
using namespace vkmincomp;

// metode public
/* the one and only one constructor to create an instance
 *
 * @param appname The name of your Application Instance
 * @param appvers Version of your Application
 * @param engname The name of your Engine;
 * @param engvers Version of your Application
 *
 * apivers use available and the higher version API of the vulkan driver
 */
stdEng::stdEng(char* appname,uint32_t appvers,char* engname,uint32_t engvers) {
  uint32_t apivers=enumerateInstanceVersion();
  ApplicationInfo appInfo(appname,appvers,engname,engvers, apivers);
  InstanceCreateInfo instInfo({},&appInfo,0,{}, 0, {});
  this->inst=new Instance(createInstance(instInfo));
}

/* Actually Priorities are array but we only use 1 device
 * @param priority priority for the device
 */
void stdEng::setPriorities(float priority) {
  this->priorities=priority;
}

/* For inputs stuff
 * 
 * @param inputs vector from vector of the input arrays
 * @param size total size of the each vector in byte
 */
void stdEng::setInputs(vector<vector<void*>> inputs,vector<size_t> size) {
  this->inputs=inputs;
  this->insizes=size;
}

// Same as inputs
void stdEng::setOutputs(vector<vector<void*>> outputs,vector<size_t> size) {
  this->outputs=outputs;
  this->outsizes=size;
}

 /* Set up binding in the shader
 *
 * Using this library means you must group the set&binding of inputs together
 * before finally set&binding the outputs.
 *
 * @param bindings The size of the bindings is the number of sets, while the elements represent
 *   the number of bindings in each set.
 * @param IOSetOffset This is the offset value of the output set relative to the input.
 *   For example, if the first outputs is in set 5, the IOSetOffset should be 5.
 * @param IOBindingOffset Similar to the previous one, if the first output is in binding
 *   n, the offset should also be n.
 */
void stdEng::setBinding(vector<uint32_t> bindings,uint32_t IOSetOffset,uint32_t IOBindingOffset) {
  this->bindings=bindings;
  this->IOSetOffset=IOSetOffset;
  this->IOBindingOffset=IOBindingOffset;
}

/* The shader used, as far as I know, is SPIR-V, which is usually compiled from HLSL or GLSL.
 *
 * @param filepath The path to the SPIR-V shader file, relative to where this class is used.
 */
void stdEng::setShaderFile(char* filepath) {
  this->filepath=filepath;
}

/* Set the name of the main function or entry point in the shader
 * In shaders, you can have multiple functions, so you need to know where the main function is.
 *
 * @param entryPoint The name of the function as the entry point in the shader.
 */
void stdEng::setEntryPoint(char* entryPoint){
  this->entryPoint=entryPoint;
}

/* Iam using Fence for mark  if our calculation finish
 *
 * @param time timeout for waiting the fence
 */
void stdEng::setWaitFenceFor(uint64_t time){
  this->time=time;
}
//akhir dari metode public


//metode private
/* Create device
 */
void stdEng::createDevice() {
  vector<PhysicalDevice> physdevs=this->inst->enumeratePhysicalDevices();
  if (physdevs.empty()) {
    cout << "No Vulkan driver found!" << endl;
    exit(EXIT_FAILURE);
  }
  this->physdev=new PhysicalDevice(physdevs[0]);
  vector<QueueFamilyProperties> qFamProps=this->physdev->getQueueFamilyProperties();
  auto qFamProp=find_if(qFamProps.begin(),qFamProps.end(),[](QueueFamilyProperties qFamPropsT){
    return qFamPropsT.queueFlags & QueueFlagBits::eCompute;
    });
  this->queueFamIndex=distance(qFamProps.begin(),qFamProp);
  DeviceQueueCreateInfo devQInfo(DeviceQueueCreateFlags(),this->queueFamIndex,1,&this->priorities);
  this->devQInfo=&devQInfo;
  DeviceCreateInfo devInfo({},devQInfo);
  this->devInfo=&devInfo;
  Device dev=physdev->createDevice(devInfo);
  this->dev =&dev;
}

//crrating buffers
void stdEng::createBuffer() {
  for (size_t insize:this->insizes) {
    BufferCreateInfo inBuffInfo(
      BufferCreateFlags(),
      insize,
      BufferUsageFlagBits::eStorageBuffer,
      SharingMode::eExclusive
    );
    this->inBuffInfos.push_back(&inBuffInfo);
    Buffer inbuff=this->dev->createBuffer(inBuffInfo);
    this->inBuffs.push_back(&inbuff);
  }
  for (size_t outsize:this->outsizes) {
    BufferCreateInfo outBuffInfo(
      BufferCreateFlags(),
      outsize,
      BufferUsageFlagBits::eStorageBuffer,
      SharingMode::eExclusive
    );
    this->outBuffInfos.push_back(&outBuffInfo);
    Buffer outbuff=this->dev->createBuffer(outBuffInfo);
    this->outBuffs.push_back(&outbuff);
  }
}


//Memory allocations
void stdEng::allocateMemory() {
  for (Buffer* inbuff:this->inBuffs) {
    MemoryRequirements inMemReq=this->dev->getBufferMemoryRequirements(*inbuff);
    this->inMemReqs.push_back(&inMemReq);
  }
  for (Buffer* outbuff:this->outBuffs) {
    MemoryRequirements outMemReq=this->dev->getBufferMemoryRequirements(*outbuff);
    this->outMemReqs.push_back(&outMemReq);
  }
  PhysicalDeviceMemoryProperties heapMemProp=this->physdev->getMemoryProperties();
  DeviceSize heapSize=uint32_t(~0);
  uint32_t heapIndex=uint32_t(~0);
  for (uint32_t i=0;i < heapMemProp.memoryTypeCount; ++i) {
    if ((heapMemProp.memoryTypes[i].propertyFlags & MemoryPropertyFlagBits::eHostVisible) &&
        (heapMemProp.memoryTypes[i].propertyFlags & MemoryPropertyFlagBits::eHostCoherent)) {
      heapSize=heapMemProp.memoryHeaps[i].size;
      heapIndex=i;
      break;
    }
  }
  if (heapIndex==uint32_t(~0)) {
    throw runtime_error("No heap found");
  }
  for (MemoryRequirements* inMemReq:this->inMemReqs) {
    MemoryAllocateInfo inMemAllocInfo(inMemReq->size,heapIndex);
    this->inMemAllocInfos.push_back(&inMemAllocInfo);
    DeviceMemory inmem=this->dev->allocateMemory(inMemAllocInfo);
    this->inMems.push_back(&inmem);
  }
  for (MemoryRequirements* outMemReq:this->outMemReqs) {
    MemoryAllocateInfo outMemAllocInfo(outMemReq->size,heapIndex);
    this->outMemAllocInfos.push_back(&outMemAllocInfo);
    DeviceMemory outmem=this->dev->allocateMemory(outMemAllocInfo);
    this->outMems.push_back(&outmem);
  }
}


//fill the memories with our inputs data
void stdEng::fillInputs(){
  for (size_t i=0;i<this->inputs.size();++i){
    void* inPtr=this->dev->mapMemory(*this->inMems.at(i),0,this->insizes.at(i));
    memcpy(inPtr,this->inputs.at(i).data(),this->insizes.at(i));
    this->dev->unmapMemory(*this->inMems.at(i));
  }
}

// load SPIR-V shader
void stdEng::loadShader(){
  vector<char> shaderRaw;
  ifstream shaderFile(this->filepath,ios::ate|ios::binary);
  if (!shaderFile){
    throw runtime_error("Failed to open shader file");
  }
  size_t shaderSize=shaderFile.tellg();
  shaderFile.seekg(0);
  shaderRaw.resize(shaderSize);
  shaderFile.read(shaderRaw.data(),shaderSize);
  shaderFile.close();

  ShaderModuleCreateInfo shadModInfo(ShaderModuleCreateFlags(),shaderSize,
    reinterpret_cast<const uint32_t*>(shaderRaw.data()));
  ShaderModule shadMod=this->dev->createShaderModule(shadModInfo);
  this->shadModInfo=&shadModInfo;
  this->shadMod=&shadMod;
}


//Creating DescriptorSetLayout for binding to the Shader
void stdEng::createDescriptorSetLayout(){
  uint32_t sumBind=0;
  for(uint32_t setI=0;setI<this->bindings.size();++setI) {
    sumBind += this->bindings.at(setI);
    for (uint32_t bindI=0;bindI<this->bindings.at(setI);++bindI) {
      DescriptorSetLayoutBinding descSetLayBind(bindI,DescriptorType::eStorageBuffer,
        setI,ShaderStageFlagBits::eCompute);
      this->descSetLayBinds.push_back(&descSetLayBind);
    }
  }
  //save the amount of the binding which used later
  this->sumBind=sumBind;
  vector<DescriptorSetLayoutBinding> descSetLayBinds2;
  for (DescriptorSetLayoutBinding* descSetLayBind:this->descSetLayBinds) {
    descSetLayBinds2.push_back(*descSetLayBind);
  }
  DescriptorSetLayoutCreateInfo descSetLayInfo(DescriptorSetLayoutCreateFlags(),descSetLayBinds2);
  this->descSetLayInfo=&descSetLayInfo;
  DescriptorSetLayout descSetLay=this->dev->createDescriptorSetLayout(descSetLayInfo);
  this->descSetLay=&descSetLay;
}

//Create Pipeline Layout
void stdEng::createPipelineLayout() {
  PipelineLayoutCreateInfo pipeLayInfo(PipelineLayoutCreateFlags(),*this->descSetLay);
  PipelineLayout pipeLay=this->dev->createPipelineLayout(pipeLayInfo);
  this->pipeLayInfo=&pipeLayInfo;
  this->pipeLay=&pipeLay;
}

//Create Pipeline
void stdEng::createPipeline(){
  PipelineShaderStageCreateInfo pipeShadStagInfo(PipelineShaderStageCreateFlags(),
      ShaderStageFlagBits::eCompute,*(this->shadMod),this->entryPoint);
  this->pipeShadStagInfo=&pipeShadStagInfo;
  PipelineCache pipeCache=this->dev->createPipelineCache(PipelineCacheCreateInfo());
  ComputePipelineCreateInfo compPipeInfo(PipelineCreateFlags(),pipeShadStagInfo,*(this->pipeLay));
  this->compPipeInfo=&compPipeInfo;
  Pipeline pipe=this->dev->createComputePipeline(pipeCache,compPipeInfo).value;
  this->pipe=&pipe;
}

//Create Descriptor Pool
void stdEng::createDescriptorPool(){
  /* The second parameter is the number of descriptors per type. Since there is only one type
   * here, this is the total number of bindings.
  */
  DescriptorPoolSize descPoolSize(DescriptorType::eStorageBuffer,this->sumBind);
  this->descPoolSize=&descPoolSize;
  DescriptorPoolCreateInfo descPoolInfo(DescriptorPoolCreateFlags(),this->bindings.size(),
      descPoolSize);
  this->descPoolInfo=&descPoolInfo;
  DescriptorPool descPool=this->dev->createDescriptorPool(descPoolInfo);
  this->descPool=&descPool;
}

//DescriptorSet allocation
void stdEng::allocateDescriptorSet(){
  DescriptorSetAllocateInfo descSetAllocInfo(*(this->descPool),this->bindings.size(),
      this->descSetLay);
  this->descSetAllocInfo=&descSetAllocInfo;
  vector<DescriptorSet> descSets=this->dev->allocateDescriptorSets(descSetAllocInfo);
  for(DescriptorSet descSet:descSets) this->descSets.push_back(&descSet);
  vector<DescriptorBufferInfo> descBuffInfos;
  for(uint32_t i=0;i<this->insizes.size();++i){
    DescriptorBufferInfo descbuffinfo(*this->inBuffs.at(i),0,this->inBuffInfos.at(i)->size);
    descBuffInfos.push_back(descbuffinfo);
  }
  for(uint32_t i=0;i<this->outsizes.size();++i){
    DescriptorBufferInfo descbuffinfo(*this->outBuffs.at(i),0,this->outBuffInfos.at(i)->size);
    descBuffInfos.push_back(descbuffinfo);
  }
  vector<WriteDescriptorSet> writeDescSets;
  uint32_t p=0;
  if(IOBindingOffset!=0){
    for(uint32_t i=0;i<=IOSetOffset;++i){
      if(i!=IOSetOffset){
        for(uint32_t j=0;j<bindings.at(i);++j){
          writeDescSets.push_back({descSets.at(i),j,0,i,DescriptorType::eStorageBuffer,nullptr,&descBuffInfos.at(p)});
          p++;
        }
      }
    }
  }
  else{
    for(uint32_t i=0;i<IOSetOffset;++i){
      for(uint32_t j=0;j<bindings.at(i);++j){
        writeDescSets.push_back({descSets.at(i),j,0,i,DescriptorType::eStorageBuffer,nullptr,&descBuffInfos.at(p)});
        ++p;
        }
      }
  }
  this->dev->updateDescriptorSets(writeDescSets,nullptr);
}

//Commamd Buffer Creation
void stdEng::createCommandBuffer(){
  CommandPoolCreateInfo cmdPoolInfo(CommandPoolCreateFlags(),this->queueFamIndex);
  this->cmdPoolInfo=&cmdPoolInfo;
  CommandPool cmdPool=this->dev->createCommandPool(cmdPoolInfo);
  this->cmdPool=&cmdPool;
  CommandBufferAllocateInfo cmdBuffAllocInfo(cmdPool,CommandBufferLevel::ePrimary,1);
  this->cmdBuffAllocInfo=&cmdBuffAllocInfo;
  vector<CommandBuffer> cmdBuffs=this->dev->allocateCommandBuffers(cmdBuffAllocInfo);
  for(CommandBuffer cmdbuff:cmdBuffs) this->cmdBuffs.push_back(&cmdbuff);
}

//Send Command with CommandBuffer
void stdEng::sendCommand(){
  CommandBufferBeginInfo cmdBuffBeginInfo(CommandBufferUsageFlagBits::eOneTimeSubmit);
  CommandBuffer cmdBuff=*this->cmdBuffs.front();
  cmdBuff.begin(cmdBuffBeginInfo);
  cmdBuff.bindPipeline(PipelineBindPoint::eCompute,*this->pipe);
  vector<DescriptorSet> realDescSets;
  for(DescriptorSet* descset:this->descSets) realDescSets.push_back(*descset);
  cmdBuff.bindDescriptorSets(PipelineBindPoint::eCompute,*this->pipeLay,0,realDescSets,{});
  cmdBuff.dispatch(this->width,this->height,this->depth);
  cmdBuff.end();
}


//wait gpu proccess with Fence to mark if finish
void stdEng::waitFence(){
  Queue queue=this->dev->getQueue(this->queueFamIndex,0);
  this->queue=&queue;
  Fence fence=this->dev->createFence(FenceCreateInfo());
  this->fence=&fence;
  SubmitInfo submitInfo(0,nullptr,nullptr,1,this->cmdBuffs.front());
  this->submitInfo=&submitInfo;
  queue.submit({ submitInfo },fence);
  Result waitFenceRes=this->dev->waitForFences({ fence },true,this->time);
  this->waitFenceRes=&waitFenceRes;
}

//metode public
//the main method to running all previous methods
void stdEng::run() {
  if(!(this->debugMode==DebugMode::NO)){
    cout<<"Instance Created with:"<<endl;
    if(this->debugMode==DebugMode::VERBOSE){
      cout<<"\tApplicationInfo"<<endl;
      cout<<"\t\tApplication Name="<<this->appInfo->pApplicationName<<","<<endl;
      cout<<"\t\tApplication Version="<<this->appInfo->applicationVersion<<","<<endl;
      cout<<"\t\tEngine Name="<<this->appInfo->pEngineName<<","<<endl;
      cout<<"\t\tEngine Version="<<this->appInfo->engineVersion<<","<<endl;
      cout<<"\t\tApi Version used="<<this->appInfo->apiVersion<<","<<endl;
      cout<<"\tInstanceCreateInfo"<<endl;
      cout<<"\t\tInstance Flags="<<to_string(this->instInfo->flags)<<","<<endl;
      cout<<"\t\tAmount of active layer="<<this->instInfo->enabledLayerCount<<","<<endl;
      cout<<"\t\tActive layers"<<endl;
      for(uint32_t i=0;i<this->instInfo->enabledLayerCount;++i)
        cout<<"\t\t\t"<<this->instInfo->ppEnabledLayerNames[i]<<endl;
      cout<<"\t\tAmount of active extensions"<<this->instInfo->enabledExtensionCount<<endl;
      cout<<"\t\tActive extensions"<<endl;
      for(uint32_t i=0;i<this->instInfo->enabledExtensionCount;++i)
         cout<<"\t\t\t"<<this->instInfo->ppEnabledExtensionNames[i]<<","<<endl;
      cout<<endl;
    }
  }

  if(!(this->debugMode==DebugMode::NO)){
    cout<<"Start Creating logical device with:"<<endl;
    if(this->debugMode==DebugMode::VERBOSE){
      cout<<"\tCreateInfo"<<endl;
      cout<<"\t\tDevice Flags"<<to_string(this->devInfo->flags)<<","<<endl;
      cout<<"\t\tDevice Queue Flags"<<to_string(this->devQInfo->flags)<<","<<endl;
      cout<<"\t\t Queue Family Index"<<this->devQInfo->queueFamilyIndex<<","<<endl;
      cout<<"\t\tQueue Count="<<this->devQInfo->queueCount<<endl;
      cout<<"\\Queues"<<endl;
      for(uint32_t i=0;i<this->devQInfo->queueCount;++i)
        cout<<"\t\t\t"<<this->devQInfo->pQueuePriorities[i]<<","<<endl;
    }
    cout<<endl;
  }
  this->createDevice();

  if(!(this->debugMode==DebugMode::NO)){
    cout<<"logical device created!"<<endl;
    cout<<"Start creating buffers!"<<endl;
    if(this->debugMode==DebugMode::VERBOSE){
      cout<<"\tInput Buffer Infos"<<endl;
      for(BufferCreateInfo* buffInfo:this->inBuffInfos){
        cout<<"\t\tBuffer Create Flags="<<to_string(buffInfo->flags)<<endl;
        cout<<"\t\tBuffer Size="<<buffInfo->size<<endl;
        cout<<"\t\tBuffer Usage="<<to_string(buffInfo->usage)<<endl;
        cout<<"\t\tBuffer Sharing mode="<<to_string(buffInfo->sharingMode)<<endl;
        cout<<"\t\tQueue Family Index Count="<<buffInfo->queueFamilyIndexCount<<endl;
        cout<<"\t\tQueue Family Indices"<<endl;
        for(uint32_t i=0;i<buffInfo->queueFamilyIndexCount;++i)
          cout<<"\t\t\t"<<buffInfo->pQueueFamilyIndices<<endl;
        cout<<endl;
      }
    }
    cout<<endl;
  }
  this->createBuffer();

  if(!(this->debugMode==DebugMode::NO)){
    cout<<"Buffer created succesfully!"<<endl;
    cout<<"Start filling Inputs memories"<<endl;
    if(this->debugMode==DebugMode::VERBOSE){
      if(this->inMems.size()!=this->inputs.size()){
        cout<<"\tWarning! inputs and input memories vector has differ size"<<endl;
        cout<<"\tThis mean you were wrongly pass inputs size"<<endl;
      }
      size_t minSize=inMems.size()>inputs.size()?inMems.size():inputs.size();
      for(size_t i=0;i<minSize;++i){
        cout<<"\tinput memory "<<i<<"will be filled by"<<endl;
        cout<<"\tinput "<<i<<"in byte:"<<endl;
        const char* inputBytes=reinterpret_cast<char*>(&inputs.at(i));
        for(size_t j=0;j<this->insizes.at(i)/this->inputs.at(i).size();++i)
          cout<<"\t\t"<<inputBytes[j];
        cout<<endl;
      }
    }
    cout<<endl;
  }
  this->fillInputs();
  if(!(this->debugMode==DebugMode::NO)){
    if(this->debugMode==DebugMode::VERBOSE){
    }
  }
  this->createDescriptorSetLayout();
  this->createPipelineLayout();
  this->createPipeline();
  this->createDescriptorPool();
  this->allocateDescriptorSet();
  this->createCommandBuffer();
  this->sendCommand();
  this->waitFence();

}

//destructor
stdEng::~stdEng() {
  this->dev->destroyFence(*this->fence);
  this->dev->resetCommandPool(*this->cmdPool);
  this->dev->destroyDescriptorSetLayout(*this->descSetLay);
  this->dev->destroyPipelineLayout(*this->pipeLay);
  this->dev->destroyShaderModule(*this->shadMod);
  this->dev->destroyDescriptorPool(*this->descPool);
  this->dev->destroyCommandPool(*this->cmdPool);
  for(DeviceMemory* mem:this->inMems)this->dev->freeMemory(*mem);
  for(DeviceMemory* mem:this->outMems)this->dev->freeMemory(*mem);
  for(Buffer* buff:this->inBuffs)this->dev->destroyBuffer(*buff);
  for(Buffer* buff:this->outBuffs)this->dev->destroyBuffer(*buff);
  this->dev->destroy();
  this->inst->destroy();
}
