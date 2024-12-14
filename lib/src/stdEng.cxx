#include "vkmincomp.hxx"
#include <cstdint>
#include <fstream>
#include <iostream>

using namespace std;
using namespace vkmincomp;

// metode public
/* constructor utama dan satu satunya yang otomatis membuat instance
 *
 * @param appname Nama Aplikasi yang akan dipakaidi Instancenya
 * @paeam appvers Versi Aplikasinya
 * @param engname Nama Engine yang akan dipakai;
 * @param engvers Versi Enginenya
 *
 * apivers adalah versi api yang akan digunakan, biar otomatis kompatible, make dari versi
 * tertingginya aja deh
 */
stdEng::stdEng(char* appname,uint32_t appvers,char* engname,uint32_t engvers) {
  uint32_t apivers=enumerateInstanceVersion();
  ApplicationInfo appInfo(appname,appvers,engname,engvers, apivers);
  InstanceCreateInfo instInfo({},&appInfo,0,{}, 0, {});
  this->inst=new Instance(createInstance(instInfo));
}

/* ini untuk mengatur prioritas antrian untuk logical device yang akan dibuat
 *
 * Sebenernya seharusnya array, tapi karena queuenya cuma satu jadi ya langsung aja set
 */
void stdEng::setPriorities(float priorities) {
  this->priorities=priorities;
}

/* Untuk mengatur inputnya
 *
 * @param inputs vector yang berisi vector dari inputnya
 *   karena mau dibuat generic, jadi tipe data disini diatur ke void
 * @param size ini vector yang setiap elemennya menghold informasi total ukuran dari masing
 *   ukuran dari setiap input
 *
 * Misalnya, kalo punya dua input berupa vector<int> bulat dan vector<float> desimal, maka
 * vector<size_t> sizenya memiliki elemen bulat.size()*size_of(int) dan
 * desimal.size()*size_of(float)
 */
void stdEng::setInputs(vector<vector<void*>> inputs,vector<size_t> size) {
  this->inputs=inputs;
  this->insizes=size;
}

//sama seperti di input
void stdEng::setOutputs(vector<vector<void*>> outputs,vector<size_t> size) {
  this->outputs=outputs;
  this->outsizes=size;
}

/* Mengatur binding di shader
 *
 * Menggunakan pustaka ini berarti kamu harus mmengelompokan set&binding dari input bersama sama
 * sebelum akhirnya sst&binding output
 *
 * @param bindings size dari bindings adalah jumlah set, sedangkan elemennya adalah jumlah binding
 *   di setiap setnya
 * @param IOSetOffset ini adalah nilai  offset dari set output terhadap input
 *   misalnya, kalo output berada di set ke 5 maka IOSetOffsetnya 5
 * @param IOBindingOffset sama seperti sebelumnya, kalau outputnya berada di binding ke n
 *   maka offsetnya juga harus n
 */
void stdEng::setBinding(vector<uint32_t> bindings,uint32_t IOSetOffset,uint32_t IOBindingOffset) {
  this->bindings=bindings;
  this->IOSetOffset=IOSetOffset;
  this->IOBindingOffset=IOBindingOffset;
}

/* shader yang digunakan setauku sih SPIR-V yang biasanya dikompilasi dari hlsl atau glsl
 *
 * @param filepath path dari file shader SPIR-V berada, relatif terhadap tempat dimana kelas ini
 *   digunakan
 */
void stdEng::setShaderFile(char* filepath) {
  this->filepath=filepath;
}

/* set nama fungsi utama atau tempat masuknya di shader
 *
 * di shader kan bisa dikasih banyak fungsi tuh, jadi hrus tau fungsi utamanya dimana.
 * 
 * @param entryPoint mama dari fungsi sebagai titik masuk di shader
 */
void stdEng::setEntryPoint(char* entryPoint){
  this->entryPoint=entryPoint;
}

/* Ini untuk menentukan maksimum waktu(timeout) untuk menunggu hasil dari pemrosesan gpunya.
 * 
 * @param time timeout menunggu hasil pemrosesan gpu
 */
void stdEng::setWaitFenceFor(uint64_t time){
  this->time=time;
}
//akhir dari metode public


//metode private
//membuat logical device
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

//untuk pembuatan buffer karena inputnya vector jadi butuh loop
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


//untuk alokask memori untuk input dan output
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


//ini untuk mengisi buffer inputnya,yah semuanya butuh loop karena make vector
void stdEng::fillInputs(){
  for (size_t i=0;i<this->inputs.size();++i){
    void* inPtr=this->dev->mapMemory(*this->inMems.at(i),0,this->insizes.at(i));
    memcpy(inPtr,this->inputs.at(i).data(),this->insizes.at(i));
    this->dev->unmapMemory(*this->inMems.at(i));
  }
}

//untuk memuat shader dari file
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


//pembuatan descriptor set layoutnya untuk binding ke shader
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
  //menyimpan jumlah total binding karena nanti aka digunakan di descriptorpool size
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

/*ini untuk mengatur layout dari pipelinenya,vulkan memag verbose
 * tapi disitulah keunggulannya(free control)
 */
void stdEng::createPipelineLayout() {
  PipelineLayoutCreateInfo pipeLayInfo(PipelineLayoutCreateFlags(),*this->descSetLay);
  PipelineLayout pipeLay=this->dev->createPipelineLayout(pipeLayInfo);
  this->pipeLayInfo=&pipeLayInfo;
  this->pipeLay=&pipeLay;
}

//ini sebenarnya objek utama yang harus ada,wrapper dari semua atribut yang akan dikirim ke shader.
//kalo ga salah:v
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

//membuat descriptor pool
void stdEng::createDescriptorPool(){
  /*param kedua adalah jumlah setiap descriptor per tipe. karena tipenya disini hanya satu,maka
   * ini adalah jumlah total binding yang ada
   */
  DescriptorPoolSize descPoolSize(DescriptorType::eStorageBuffer,this->sumBind);
  this->descPoolSize=&descPoolSize;
  //kayanya param kedua ini jumlah descriptor setnya TODO fix kalo salah nanti
  DescriptorPoolCreateInfo descPoolInfo(DescriptorPoolCreateFlags(),this->bindings.size(),
      descPoolSize);
  this->descPoolInfo=&descPoolInfo;
  DescriptorPool descPool=this->dev->createDescriptorPool(descPoolInfo);
  this->descPool=&descPool;
}

//alokasi descriptor set
void stdEng::allocateDescriptorSet(){
  //param kedua hrusnya sih jumlah descriptor setnya jadi harus sama dengan jumlah total sst
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

//metode utama untuk menjalankan semua fungsi sebelumnya
void stdEng::run() {
  if(!(this->debugMode==DebugMode::NO)){
    cout<<"mulai membuat device dengan instance"<<endl;
    if(this->debugMode==DebugMode::VERBOSE){
      cout<<"\tApplicationInfo"<<endl;
      cout<<"\t\tNama Aplikasi="<<this->appInfo->pApplicationName<<","<<endl;
      cout<<"\t\tVersiAplikasi="<<this->appInfo->applicationVersion<<","<<endl;
      cout<<"\t\tNama Engine="<<this->appInfo->pEngineName<<","<<endl;
      cout<<"\t\tVersi Engine="<<this->appInfo->engineVersion<<","<<endl;
      cout<<"\t\tVersi Api="<<this->appInfo->apiVersion<<","<<endl;
      cout<<"\tInstanceCreateInfo"<<endl;
      cout<<"\t\tInstance Flags="<<to_string(this->instInfo->flags)<<","<<endl;
      cout<<"\t\tJumlah Layer Aktif="<<this->instInfo->enabledLayerCount<<","<<endl;
      cout<<"\t\tLayer Aktif="<<endl;
      for(uint32_t i=0;i<this->instInfo->enabledLayerCount;++i)
        cout<<"\t\t\t"<<this->instInfo->ppEnabledLayerNames[i]<<endl;
      cout<<"\t\t"<<this->instInfo->enabledExtensionCount<<endl;
      for(uint32_t i=0;i<this->instInfo->enabledExtensionCount;++i)
         cout<<"\t\t\t"<<this->instInfo->ppEnabledExtensionNames[i]<<","<<endl;
      cout<<endl;
    }
  }
  this->createDevice();

  if(!(this->debugMode==DebugMode::NO)){
    cout<<"device berhasil dibuat!"<<endl;
    if(this->debugMode==DebugMode::VERBOSE){
      cout<<"\tCreateInfo"<<endl;
      cout<<"\t\tDevice Flags"<<to_string(this->devInfo->flags)<<","<<endl;
      cout<<"\t\tDevice Queue Flags"<<to_string(this->devQInfo->flags)<<","<<endl;
      cout<<"\t\t Queue Family Index"<<this->devQInfo->queueFamilyIndex<<","<<endl;
      cout<<"\t\tJumlah Queue="<<this->devQInfo->queueCount<<endl;
      for(uint32_t i=0;i<this->devQInfo->queueCount;++i)
        cout<<"\t\t\t"<<this->devQInfo->pQueuePriorities[i]<<","<<endl;
    }
    cout<<"mulai membuat Buffer"<<endl;
  }
  this->createBuffer();
  if(!(this->debugMode==DebugMode::NO)){
    cout<<"Buffer berhasil dibuat!"<<endl;
    if(this->debugMode==DebugMode::VERBOSE){
      cout<<"\tInput Buffer Infos"<<endl;
      for(BufferCreateInfo* buffInfo:this->inBuffInfos){
        //cout<<"\t\t"<<buffInfo-><<endl;
        cout<<endl;
      }
    }
  }
  this->fillInputs();
  this->createDescriptorSetLayout();
  this->createPipelineLayout();
  this->createPipeline();
  this->createDescriptorPool();
  this->allocateDescriptorSet();
  this->createCommandBuffer();
  this->sendCommand();
  this->waitFence();

}

//destructornya ini mah
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
