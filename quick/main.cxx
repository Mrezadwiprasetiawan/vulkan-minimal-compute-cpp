// Copyright 2024 M Reza Dwi Prasetiawan
// License under GNU GPL v3
// For more information visit https://www.gnu.org/licenses/gpl-3.0.html
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip> //untuk std::fixed dan std::setprecision()
#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>

/*randomisasi dengan random_device
 * mt19937 Mersenne Twister Generator
 * uniform_int_distribution
 */
#include <random>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#define WIDTH 128
#define HEIGHT 1
#define DEPTH 1

using namespace std;
using namespace vk;
int main() {

  random_device randDev;
  mt19937 gen(randDev());
  uniform_real_distribution<> dis(1, 128);
  // mendefinisikan input data
  vector<float> inputs;
  for (int i = 0; i < WIDTH; i++)
    inputs.push_back(dis(gen));

  // definisikan aplikasi
  const char *appName = "Komputasi";
  const char *engineName = "No Engine";
  const uint32_t apiVers = enumerateInstanceVersion();

  // AppInfo
  ApplicationInfo appInfo(appName, 1, engineName, 1, apiVers);
  // Instance info
  InstanceCreateInfo instInfo(InstanceCreateFlags(), &appInfo, 0, {}, 0, {});
  // Buat instance
  Instance inst = createInstance(instInfo);

  // enumerasi perangkat fisik
  vector<PhysicalDevice> physDevs = inst.enumeratePhysicalDevices();
  if (physDevs.empty()) {
    throw runtime_error("tidak ada gpu dengan driver vulkan di perangkat");
    exit(EXIT_FAILURE);
  }

  // Memilih gpu pertama
  PhysicalDevice physDev = physDevs[0];

  // Mendapatkan queue family properties untuk mendapatkan queue
  vector<::QueueFamilyProperties> queueFamProps =
      physDev.getQueueFamilyProperties();
  // Assign indeks yang cocok, dalam kasus ini Compute
  auto queueFamProp =
      find_if(queueFamProps.begin(), queueFamProps.end(),
              [](QueueFamilyProperties &qFProp) {
                return qFProp.queueFlags & ::QueueFlagBits::eCompute;
              });

  uint32_t queueFamIndex = ::distance(queueFamProps.begin(), queueFamProp);

  const float priorities = 1.0;
  DeviceQueueCreateInfo devQueueInfo(DeviceQueueCreateFlags(), queueFamIndex, 1,
                                     &priorities);

  // mulai membuat device
  DeviceCreateInfo devInfo(DeviceCreateFlags(), devQueueInfo);
  Device dev = physDev.createDevice(devInfo);

  // definisikan ukuran datanya
  DeviceSize buffSize = WIDTH * sizeof(float);

  // membuat buffer
  BufferCreateInfo buffInfo(BufferCreateFlags(), buffSize,
                            BufferUsageFlagBits::eStorageBuffer,
                            SharingMode::eExclusive);
  Buffer inBuff = dev.createBuffer(buffInfo);
  buffInfo.size = WIDTH * sizeof(float);
  Buffer outBuff = dev.createBuffer(buffInfo);

  // alokasi memori
  MemoryRequirements inMemReq = dev.getBufferMemoryRequirements(inBuff);
  MemoryRequirements outMemReq = dev.getBufferMemoryRequirements(outBuff);
  // mencari tipe memori yang sesuai
  PhysicalDeviceMemoryProperties heapMemProp = physDev.getMemoryProperties();
  uint32_t heapIndex = uint32_t(~0);
  for (uint32_t i = 0; i < heapMemProp.memoryTypeCount; ++i) {
    if ((heapMemProp.memoryTypes[i].propertyFlags &
         ::MemoryPropertyFlagBits::eHostVisible) &&
        (heapMemProp.memoryTypes[i].propertyFlags &
         ::MemoryPropertyFlagBits::eHostCoherent)) {
      heapIndex = i;
      break;
    }
  }

  if (heapIndex == ~0) {
    throw runtime_error("Fatal: tidak ditemukan heap dengan tipe yang sesuai");
    exit(EXIT_FAILURE);
  }

  MemoryAllocateInfo inBuffInfo(inMemReq.size, heapIndex);
  MemoryAllocateInfo outBuffInfo(outMemReq.size, heapIndex);
  DeviceMemory inBuffMem = dev.allocateMemory(inBuffInfo);
  DeviceMemory outBuffMem = dev.allocateMemory(outBuffInfo);

  // map memori lalu isi
  float *inBuffPtr =
      static_cast<float *>(dev.mapMemory(inBuffMem, 0, buffSize));
  // salin memori
  copy(inputs.begin(), inputs.end(), inBuffPtr);
  // unmap
  dev.unmapMemory(inBuffMem);

  // bind memorinya
  dev.bindBufferMemory(inBuff, inBuffMem, 0);
  dev.bindBufferMemory(outBuff, outBuffMem, 0);

  // muat shader
  vector<char> shaderRaw;
  ifstream shaderFile("compute.spv", ios::ate | ios::binary);
  if (!shaderFile) {
    throw runtime_error("Gagal membuka file compute.spv");
  }
  size_t shaderSize = shaderFile.tellg();
  shaderFile.seekg(0);
  shaderRaw.resize(shaderSize);
  shaderFile.read(shaderRaw.data(), shaderSize);
  shaderFile.close();

  // buat shader module
  ShaderModuleCreateInfo shaderModuleInfo(
      ShaderModuleCreateFlags(), shaderSize,
      reinterpret_cast<uint32_t *>(shaderRaw.data()));
  ShaderModule shaderModule = dev.createShaderModule(shaderModuleInfo);

  // buat descriptor set layout binding untuk binding dengan shader
  vector<DescriptorSetLayoutBinding> descSetLayoutBind{
      {0, DescriptorType::eStorageBuffer, 1, ShaderStageFlagBits::eCompute},
      {1, DescriptorType::eStorageBuffer, 1, ShaderStageFlagBits::eCompute}};
  // buat descriptorSetLayoutnya
  DescriptorSetLayoutCreateInfo descSetLayoutInfo(
      DescriptorSetLayoutCreateFlags(), descSetLayoutBind);
  DescriptorSetLayout descSetLayout =
      dev.createDescriptorSetLayout(descSetLayoutInfo);

  // membuat pipeline layout
  PipelineLayoutCreateInfo compPipeLayoutInfo(PipelineLayoutCreateFlags(),
                                              descSetLayout);
  PipelineLayout compPipeLayout = dev.createPipelineLayout(compPipeLayoutInfo);
  PipelineCache compPipeCache =
      dev.createPipelineCache(PipelineCacheCreateInfo());

  // akhirnya saat saat membuat pipeline
  PipelineShaderStageCreateInfo pipeShaderStageInfo(
      PipelineShaderStageCreateFlags(), ShaderStageFlagBits::eCompute,
      shaderModule, "main");
  ComputePipelineCreateInfo compPipeInfo(PipelineCreateFlags(),
                                         pipeShaderStageInfo, compPipeLayout);
  Pipeline compPipe =
      dev.createComputePipeline(compPipeCache, compPipeInfo).value;

  // buat descriptor pool
  DescriptorPoolSize descPoolSize(DescriptorType::eStorageBuffer, 2);
  DescriptorPoolCreateInfo descPoolInfo(DescriptorPoolCreateFlags(), 1,
                                        descPoolSize);
  DescriptorPool descPool = dev.createDescriptorPool(descPoolInfo);

  // alokasi descriptor set
  DescriptorSetAllocateInfo descSetsAllocInfo(descPool, 1, &descSetLayout);
  vector<DescriptorSet> descSets =
      dev.allocateDescriptorSets(descSetsAllocInfo);
  DescriptorSet descSet = descSets.front();
  // setup descriptor buat buffernya
  DescriptorBufferInfo descInBuffInfo(inBuff, 0, buffSize);
  DescriptorBufferInfo descOutBuffInfo(outBuff, 0, buffSize);
  const vector<WriteDescriptorSet> writeDescSets{
      {descSet, 0, 0, 1, DescriptorType::eStorageBuffer, nullptr,
       &descInBuffInfo},
      {descSet, 1, 0, 1, DescriptorType::eStorageBuffer, nullptr,
       &descOutBuffInfo}};
  dev.updateDescriptorSets(writeDescSets, {});

  // hampir finish, kali ini pembuatan commandPool
  CommandPoolCreateInfo cmdPoolInfo(CommandPoolCreateFlags(), queueFamIndex);
  CommandPool cmdPool = dev.createCommandPool(cmdPoolInfo);

  // alokasi commandBuffer
  CommandBufferAllocateInfo cmdBuffAllocInfo(cmdPool,
                                             CommandBufferLevel::ePrimary, 1);
  vector<CommandBuffer> cmdBuffs = dev.allocateCommandBuffers(cmdBuffAllocInfo);

  // set commandBuffer dengan vector pertama
  CommandBuffer cmdBuff = cmdBuffs.front();

  // mulai merekam command
  CommandBufferBeginInfo cmdBuffBeginInfo(
      CommandBufferUsageFlagBits::eOneTimeSubmit);
  cmdBuff.begin(cmdBuffBeginInfo);
  cmdBuff.bindPipeline(PipelineBindPoint::eCompute, compPipe);
  cmdBuff.bindDescriptorSets(PipelineBindPoint::eCompute, compPipeLayout, 0,
                             {descSet}, {});
  cmdBuff.dispatch(WIDTH, 1, 1);
  // akhiri
  cmdBuff.end();

  Queue Queue = dev.getQueue(queueFamIndex, 0);
  Fence fence = dev.createFence(vk::FenceCreateInfo());

  SubmitInfo SubmitInfo(0, nullptr, nullptr, 1, &cmdBuff);
  Queue.submit({SubmitInfo}, fence);
  Result waitFenceRes = dev.waitForFences({fence}, true, uint64_t(-1));

  inBuffPtr = static_cast<float *>(dev.mapMemory(inBuffMem, 0, buffSize));
  cout << fixed << setprecision(9) << buffSize << endl;
  float *outBuffPtr =
      static_cast<float *>(dev.mapMemory(outBuffMem, 0, buffSize));

  cout << "Input= [" << endl;
  for (uint32_t i = 0; i < WIDTH; ++i)
    cout << inBuffPtr[i] << "," << endl;
  cout << "]" << endl << "Output= [";
  for (uint32_t i = 0; i < WIDTH; ++i)
    cout << outBuffPtr[i] << "," << endl;
  cout << "]" << endl;

  dev.unmapMemory(inBuffMem);
  dev.unmapMemory(outBuffMem);

  // cleaning dimulai dari fence
  dev.destroyFence(fence);
  dev.resetCommandPool(cmdPool);
  dev.destroyDescriptorSetLayout(descSetLayout);
  dev.destroyPipelineLayout(compPipeLayout);
  dev.destroyPipelineCache(compPipeCache);
  dev.destroyShaderModule(shaderModule);
  dev.destroyPipeline(compPipe);
  dev.destroyDescriptorPool(descPool);
  dev.destroyCommandPool(cmdPool);
  dev.freeMemory(inBuffMem);
  dev.freeMemory(outBuffMem);
  dev.destroyBuffer(inBuff);
  dev.destroyBuffer(outBuff);
  // menghancurkan device
  dev.destroy();
  // menghancurkan instance
  inst.destroy();
}
