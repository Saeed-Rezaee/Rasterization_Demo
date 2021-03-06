#pragma once

struct DESCRIPTOR_HEAP
{
    ID3D12DescriptorHeap *heap;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_start;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_start;
    u32 size;
    u32 capacity;
};

struct GRAPHICS_CONTEXT
{
    ID3D12Device3 *device;
    ID3D12GraphicsCommandList2 *cmdlist;
    ID3D12CommandQueue *cmdqueue;
    ID3D12CommandAllocator *cmdalloc[2];
    u32 resolution[2];
    u32 descriptor_size;
    u32 descriptor_size_rtv;
    u32 frame_index;
    u32 back_buffer_index;
    ID3D12Resource *swapbuffers[4];
    IDXGISwapChain3 *swapchain;
    ID3D12Resource *ds_buffer;
    DESCRIPTOR_HEAP rt_heap;
    DESCRIPTOR_HEAP ds_heap;
    DESCRIPTOR_HEAP cpu_descriptor_heap;
    DESCRIPTOR_HEAP gpu_descriptor_heaps[2];
    ID3D12Fence *frame_fence;
    HANDLE frame_fence_event;
    u64 frame_count;
};

void Init_Graphics_Context(HWND window, GRAPHICS_CONTEXT &gfx);
void Shutdown_Graphics_Context(GRAPHICS_CONTEXT &gfx);
DESCRIPTOR_HEAP &Get_Descriptor_Heap(GRAPHICS_CONTEXT &gfx, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, u32 &descriptor_size);
void Present_Frame(GRAPHICS_CONTEXT &gfx, u32 swap_interval);
void Wait_For_GPU(GRAPHICS_CONTEXT &gfx);

void Load_File(const char *filename, u32 &size, u8 *&data);
void Update_Frame_Stats(HWND window, const char *name, f64 &time, f32 &delta_time);
f64 Get_Time();
HWND Create_Window(const char *name, u32 width, u32 height);
void *Mem_Alloc(usize size);
void *Mem_Realloc(void *addr, usize size);
void Mem_Free(void *addr);

inline ID3D12GraphicsCommandList2 *
Get_And_Reset_Command_List(GRAPHICS_CONTEXT &gfx)
{
    gfx.cmdalloc[gfx.frame_index]->Reset();
    gfx.cmdlist->Reset(gfx.cmdalloc[gfx.frame_index], nullptr);
    return gfx.cmdlist;
}

inline void
Allocate_Descriptors(GRAPHICS_CONTEXT &gfx, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 count, D3D12_CPU_DESCRIPTOR_HANDLE& handle)
{
    u32 descriptor_size;
    DESCRIPTOR_HEAP &heap = Get_Descriptor_Heap(gfx, type, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, descriptor_size);
    assert((heap.size + count) < heap.capacity);
    handle.ptr = heap.cpu_start.ptr + heap.size * descriptor_size;
    heap.size += count;
}

inline void
Allocate_GPU_Descriptors(GRAPHICS_CONTEXT &gfx, u32 count, D3D12_CPU_DESCRIPTOR_HANDLE &cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE &gpu_handle)
{
    u32 descriptor_size;
    DESCRIPTOR_HEAP &heap = Get_Descriptor_Heap(gfx, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, descriptor_size);
    assert((heap.size + count) < heap.capacity);

    cpu_handle.ptr = heap.cpu_start.ptr + heap.size * descriptor_size;
    gpu_handle.ptr = heap.gpu_start.ptr + heap.size * descriptor_size;

    heap.size += count;
}

inline D3D12_GPU_DESCRIPTOR_HANDLE
Copy_Descriptors_To_GPU(GRAPHICS_CONTEXT &gfx, u32 count, D3D12_CPU_DESCRIPTOR_HANDLE src_base)
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_dst_base;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_dst_base;
    Allocate_GPU_Descriptors(gfx, count, cpu_dst_base, gpu_dst_base);
    gfx.device->CopyDescriptorsSimple(count, cpu_dst_base, src_base, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return gpu_dst_base;
}

inline void
Get_Back_Buffer(GRAPHICS_CONTEXT &gfx, ID3D12Resource *&buffer, D3D12_CPU_DESCRIPTOR_HANDLE &handle)
{
    buffer = gfx.swapbuffers[gfx.back_buffer_index];
    handle = gfx.rt_heap.cpu_start;
    handle.ptr += gfx.back_buffer_index * gfx.descriptor_size_rtv;
}

inline void
Get_Depth_Stencil_Buffer(GRAPHICS_CONTEXT &gfx, ID3D12Resource *&buffer, D3D12_CPU_DESCRIPTOR_HANDLE &handle)
{
    buffer = gfx.ds_buffer;
    handle = gfx.ds_heap.cpu_start;
}

inline void
Bind_GPU_Descriptor_Heap(const GRAPHICS_CONTEXT &gfx)
{
    gfx.cmdlist->SetDescriptorHeaps(1, &gfx.gpu_descriptor_heaps[gfx.frame_index].heap);
}
