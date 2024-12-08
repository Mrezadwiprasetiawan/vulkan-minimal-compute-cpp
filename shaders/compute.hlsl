#define WIDTH 128
[[vk::binding(0, 0)]] RWStructuredBuffer<float> InBuffer;
[[vk::binding(1, 0)]] RWStructuredBuffer<float> OutBuffer;

[numthreads(WIDTH, 1, 1)]

void main(uint3 DTid : SV_DispatchThreadID)
{
  //hanya reverse index sebagai contoh
  OutBuffer[DTid.x]=InBuffer[WIDTH-DTid.x];
}
