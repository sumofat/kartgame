#define USE_DEBUG_LAYER 1

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
//#include <DirectXMath.h>

// D3D12 extension library.
#include "d3dx12.h"

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
#include <chrono>

#include "RenderPipelineState.h"

using namespace Microsoft::WRL;

// TODO(Ray Garner): Make a more sophistacated threshold here this is probably
//too simplistic and not yield optimal results.
#define UPLOAD_OP_THRESHOLD 10
#define MAX_SRV_DESC_HEAP_COUNT 512// NOTE(Ray Garner): totally arbiturary number pulled out of my ass

#define SHADER_DEBUG_FLAGS D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_PARTIAL_PRECISION | D3DCOMPILE_OPTIMIZATION_LEVEL0 | D3DCOMPILE_WARNINGS_ARE_ERRORS //| D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY
#define GRAPHICS_MAX_RENDER_TARGETS 4
struct alignas( 16 ) GenerateMipsCB
{
    u32 SrcMipLevel;           // Texture level of source mip
    u32 NumMipLevels;          // Number of OutMips to write: [1-4]
    u32 SrcDimension;          // Width and height of the source texture are even or odd.
    u32 IsSRGB;                // Must apply gamma correction to sRGB textures.
    f2 TexelSize;    // 1.0 / OutMip1.Dimensions
};

struct CompatibilityProfile
{
    int level;
};

//TODO(Ray):Thinkof a better way to deal with the device object pointer!!
//Requires a dereference and a cast for each access.
struct RenderDevice
{
    void* device;
    void* device_context;
    int max_render_targets = GRAPHICS_MAX_RENDER_TARGETS;
    CompatibilityProfile profile;
    //TODO(Ray):- newArgumentEncoderWithArguments:
    //Creates a new argument encoder for a specific array of arguments.
    //Required.
    ArgumentBuffersTier argument_buffers_support;
    //This limit is only applicable to samplers that have their supportArgumentBuffers property set to YES.
    uint32_t max_argument_buffer_sampler_count;
};

struct GPUMemoryResult
{
    u64 Budget;
    u64 CurrentUsage;
    u64 AvailableForReservation;
    u64 CurrentReservation;
};

struct LoadedTexture
{
    void* texels;
    f2 dim;
    u32 size;
    u32 bytes_per_pixel;
    f32 width_over_height;// TODO(Ray Garner): probably remove
    f2 align_percentage;// TODO(Ray Garner): probably remove this
    u32 channel_count;//grey = 1: grey,alpha = 2,rgb = 3,rgba = 4
    Texture texture;
    void* state;// NOTE(Ray Garner): temp addition
    uint32_t slot;
};

struct CreateDeviceResult
{
    bool is_init;
    int compatible_level;
    f2 dim;
    RenderDevice device;
};

//Commands
enum D12CommandType
{
    D12CommandType_StartCommandList,
    D12CommandType_EndCommandList,
    D12CommandType_Draw,
    D12CommandType_DrawIndexed,
    D12CommandType_Viewport,
    D12CommandType_PipelineState,
    D12CommandType_RootSignature,
    D12CommandType_ScissorRect,
    D12CommandType_GraphicsRootDescTable,
    D12CommandType_GraphicsRootConstant
};

struct D12RenderCommandList
{
    FMJMemoryArena arena;
    u64 count;
};

struct D12CommandHeader
{
    D12CommandType type;
    u32 pad;
};

struct D12CommandBasicDraw
{
    u32 vertex_offset;
    u32 count;
    D3D12_PRIMITIVE_TOPOLOGY topology;
    u32 heap_count;
    //ID3D12DescriptorHeap* heaps;
    D3D12_VERTEX_BUFFER_VIEW buffer_view;// TODO(Ray Garner): add a way to bind multiples
    // TODO(Ray Garner): add a way to bind multiples
};

struct D12CommandIndexedDraw
{
    u32 index_count;
    D3D12_PRIMITIVE_TOPOLOGY topology;
    u32 heap_count;
    //ID3D12DescriptorHeap* heaps;
    D3D12_VERTEX_BUFFER_VIEW uv_view;
    D3D12_VERTEX_BUFFER_VIEW buffer_view;// TODO(Ray Garner): add a way to bind multiples
    D3D12_INDEX_BUFFER_VIEW index_buffer_view;
    // TODO(Ray Garner): add a way to bind multiples
};

struct D12CommandViewport
{
    f4 viewport;
};

struct D12CommandRootSignature
{
    ID3D12RootSignature* root_sig;
};

struct D12CommandPipelineState
{
    ID3D12PipelineState* pipeline_state;
};

struct D12CommandScissorRect
{
    CD3DX12_RECT rect; 
    //f4 rect;
};

struct D12CommandGraphicsRootDescTable
{
    u64 index;
    ID3D12DescriptorHeap* heap;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
};

struct D12CommandGraphicsRoot32BitConstant
{
    u32 index;
    u32 num_values;
    void* gpuptr;
    u32 offset;
};

struct D12CommandStartCommandList
{
    bool dummy;
};

struct D12CommandEndCommmandList
{
    bool dummy;
};

struct D12RenderTargets
{
    bool is_desc_range;
    u32 count;
    D3D12_CPU_DESCRIPTOR_HANDLE* descriptors;
    D3D12_CPU_DESCRIPTOR_HANDLE* depth_stencil_handle;
};

struct GPUArena
{
    u64 size;
    ID3D12Heap* heap;
    ID3D12Resource* resource;
    union
    {
        D3D12_VERTEX_BUFFER_VIEW buffer_view;
        D3D12_INDEX_BUFFER_VIEW index_buffer_view;
    };
};

struct DrawTest
{
    FMJ3DTrans ot;
    GPUArena v_buffer;
    ID3D12DescriptorHeap* heap;
};

struct UploadOperations
{
    u64 count;
    AnyCache table_cache;
    FMJTicketMutex ticket_mutex;
    u64 current_op_id;
    u64 fence_value;
    ID3D12Fence* fence;
    HANDLE fence_event;
};

struct UploadOp
{
    u64 id;
    u64 thread_id;
    GPUArena arena;
    GPUArena temp_arena;
};

struct UploadOpKey
{
    u64 id;
};

//Similar to a MTLCommandBuffer
struct D12CommandAllocatorKey
{
    u64 ptr;
    u64 thread_id;//    
};

struct D12CommandAllocatorEntry
{
    ID3D12CommandAllocator* allocator;
	FMJStretchBuffer used_list_indexes;//Queued list indexes inflight being processed 
	u64 index;
    u64 fence_value;
    u64 thread_id;
    D3D12_COMMAND_LIST_TYPE type;
};

struct D12CommandAlloctorToCommandListKeyEntry
{
	u64 command_allocator_index;
	u64 command_list_index;
};

//similar to MTLRenderCommandEncoder
struct D12CommandListEntry
{
    u64 index;
	ID3D12GraphicsCommandList* list;
//    RenderPassDescriptor* pass_desc;
	u64 encoding_thread_index;
	bool is_encoding;
    D3D12_COMMAND_LIST_TYPE type;
    FMJStretchBuffer temp_resources;//temp resources to release after execution is finished.
};

struct D12CommandAllocatorTables
{
    FMJStretchBuffer command_buffers;
    FMJStretchBuffer free_command_buffers;
    FMJStretchBuffer free_allocators;
    FMJStretchBuffer command_lists;
	FMJStretchBuffer allocator_to_list_table;
    
    //One for each type 
    FMJStretchBuffer free_allocator_table;//direct/graphics
    FMJStretchBuffer free_allocator_table_compute;
    FMJStretchBuffer free_allocator_table_copy;
    
    AnyCache fl_ca;//command_allocators
};

struct D12Drawable
{
    ID3D12Resource* back_buffer;
};

struct D12CommandBufferEntry
{
	FMJStretchBuffer command_lists;//D12CommandListEntry
};

struct CommandAllocToListResult
{
    D12CommandListEntry list;
    u64 index;
    bool found;
};

struct D12ResourceTables
{
    AnyCache resources;
    FMJStretchBuffer per_thread_sub_resource_state;
    FMJStretchBuffer global_sub_resrouce_state;
};

struct D12Resource
{
    u32 id;
    ID3D12Resource* state;
    D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support;
    u32 thread_id;
};

struct D12ResourceKey
{
    //u32 id;
    u64 id;//pointer to resource
    u32 thread_id;
};

struct D12ResourceStateEntry
{
    D12Resource resource;
    D12CommandListEntry list;
    D3D12_RESOURCE_STATES state;
};


#include "D12ResourceState.h"


namespace D12RendererCode
{
    const uint8_t num_of_back_buffers = 3;
	// Use WARP adapter
	bool g_UseWarp = false;
	// Window handle.
	HWND g_hWnd;
	// Window rectangle (used to toggle fullscreen state).
	RECT g_WindowRect;
    
	// DirectX 12 Objects
	ID3D12Device2* device;
	
    ID3D12CommandQueue* command_queue;
    ID3D12CommandQueue* copy_command_queue;
    ID3D12CommandQueue* compute_command_queue;
    
	FMJStretchBuffer temp_queue_command_list;
    
	IDXGISwapChain4* swap_chain;
    ID3D12Resource* back_buffers[num_of_back_buffers];
	ID3D12DescriptorHeap* rtv_descriptor_heap;
	UINT rtv_desc_size;
    D12CommandAllocatorTables allocator_tables;
    
	//SYNCHRO
	// Synchronization objects
	ID3D12Fence* fence;
    ID3D12Fence* end_frame_fence;
    ID3D12Fence* compute_fence;
    
    u64 fence_value;
    u64 compute_fence_value;
    
    u64 frame_fence_values[num_of_back_buffers] = {};
	HANDLE fence_event;
	u64 current_allocator_index;
    
    ID3D12CommandAllocator* resource_ca;
    ID3D12GraphicsCommandList* resource_cl;
    bool is_resource_cl_recording = false;
    
    UploadOperations upload_operations;
    
    ID3D12PipelineState* pipeline_state;
    ID3D12Resource* depth_buffer;
    ID3D12DescriptorHeap* dsv_heap;
    ID3D12RootSignature* root_sig;
    D3D12_VERTEX_BUFFER_VIEW buffer_view;
    D3D12_VIEWPORT  viewport;
    D3D12_RECT sis_rect;
    
    IDXGIAdapter4* dxgiAdapter4;
    
    FMJMemoryArena constants_arena;
    
    D12RenderCommandList render_com_buf;
    ID3D12DescriptorHeap* default_srv_desc_heap;
    u32 tex_heap_count;
    
    ID3D12RootSignature* CreatRootSignature(D3D12_ROOT_PARAMETER1* params,int param_count,D3D12_STATIC_SAMPLER_DESC* samplers,int sampler_count,D3D12_ROOT_SIGNATURE_FLAGS flags)
    {
        ID3D12RootSignature* result;
        
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_d = {};
        root_sig_d.Init_1_1(param_count, params, sampler_count, samplers, flags);
        
        D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
        {
            feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }
        
        // Serialize the root signature.
        ID3DBlob* root_sig_blob;
        ID3DBlob* err_blob;
        D3DX12SerializeVersionedRootSignature(&root_sig_d,
                                              feature_data.HighestVersion, &root_sig_blob, &err_blob);
        if ( err_blob )
        {
            OutputDebugStringA( (const char*)err_blob->GetBufferPointer());
            ASSERT(false);
        }
        // Create the root signature.
        HRESULT r = device->CreateRootSignature(0, root_sig_blob->GetBufferPointer(), 
                                                root_sig_blob->GetBufferSize(), IID_PPV_ARGS(&result));
        ASSERT(SUCCEEDED(r));
        return result;
    }
    
    ID3D12DescriptorHeap* CreateDescriptorHeap(int num_desc,D3D12_DESCRIPTOR_HEAP_TYPE type,D3D12_DESCRIPTOR_HEAP_FLAGS  flags)
    {
        ID3D12DescriptorHeap* result;
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = num_desc;
        heapDesc.Flags = flags;
        heapDesc.Type = type;
        HRESULT r = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&result));
        if (FAILED(r))
        {
            ASSERT(false);
        }
        return result;
    }
    
    ID3D12PipelineState*  CreatePipelineState(D3D12_INPUT_ELEMENT_DESC* input_layout,int input_layout_count,ID3DBlob* vs_blob,ID3DBlob* fs_blob)
    {
        ID3D12PipelineState* result;
        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
            CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RasterizerState;
            CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC blend_state;            
        };
        
        D3D12_RT_FORMAT_ARRAY rtv_formats = {};
        rtv_formats.NumRenderTargets = 1;
        rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

        
        PipelineStateStream ppss = {};
        ppss.pRootSignature = D12RendererCode::root_sig;
        ppss.InputLayout = { input_layout,(u32)input_layout_count };
        
        ppss.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        ppss.VS = CD3DX12_SHADER_BYTECODE(vs_blob);
        ppss.PS = CD3DX12_SHADER_BYTECODE(fs_blob);
        ppss.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        ppss.RTVFormats = rtv_formats;
        CD3DX12_DEFAULT d = {};
        CD3DX12_RASTERIZER_DESC raster_desc = CD3DX12_RASTERIZER_DESC(d);
        raster_desc.CullMode = D3D12_CULL_MODE_NONE;
        ppss.RasterizerState = CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(raster_desc);
        CD3DX12_BLEND_DESC bdx = CD3DX12_BLEND_DESC(d);
        bdx.RenderTarget[0].BlendEnable = true;
        bdx.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        bdx.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        ppss.blend_state = CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC(bdx);
        
        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = 
        {
            sizeof(PipelineStateStream), &ppss
        };
        
        HRESULT r = device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&result));
        ASSERT(SUCCEEDED(r));
        return result;
    }
    
#define Pop(ptr,type) (type*)Pop_(ptr,sizeof(type));ptr = (uint8_t*)ptr + (sizeof(type));
    inline void* Pop_(void* ptr,u32 size)
    {
        return ptr;
    }
    
    inline void AddHeader(D12CommandType type)
    {
        FMJMemoryArenaPushParams def = fmj_arena_push_params_default();
        def.alignment = 0;
        D12CommandHeader* header = PUSHSTRUCT(&render_com_buf.arena,D12CommandHeader,def);
        header->type = type;
    }
    
#define AddCommand(type) (type*)AddCommand_(sizeof(type));
    inline void* AddCommand_(u32 size)
    {
        ++render_com_buf.count;
        FMJMemoryArenaPushParams def = fmj_arena_push_params_default();
        def.alignment = 0;
        return PUSHSIZE(&render_com_buf.arena,size,def);
    }
    
    void AddDrawIndexedCommand(u32 index_count,u32 heap_count,ID3D12DescriptorHeap* heaps,D3D12_PRIMITIVE_TOPOLOGY topology,
                               D3D12_VERTEX_BUFFER_VIEW uv_view,
                               D3D12_VERTEX_BUFFER_VIEW buffer_view,
                               D3D12_INDEX_BUFFER_VIEW index_buffer_view)
    {
        AddHeader(D12CommandType_DrawIndexed);
        D12CommandIndexedDraw* com = AddCommand(D12CommandIndexedDraw);
        com->index_count = index_count;
        com->heap_count = heap_count;
        com->topology = topology;
        com->uv_view = uv_view;
        com->buffer_view = buffer_view;
        com->index_buffer_view = index_buffer_view;
    }
    
    void AddDrawCommand(u32 offset,u32 count,D3D12_PRIMITIVE_TOPOLOGY topology,D3D12_VERTEX_BUFFER_VIEW buffer_view)
    {
        ASSERT(count != 0);
        AddHeader(D12CommandType_Draw);
        D12CommandBasicDraw* com = AddCommand(D12CommandBasicDraw);
        com->count = count;
        com->vertex_offset = offset;
        com->topology = topology;
        com->buffer_view = buffer_view;
    }
    
    void AddViewportCommand(f4 vp)
    {
        AddHeader(D12CommandType_Viewport);
        D12CommandViewport* com = AddCommand(D12CommandViewport);
        com->viewport = vp;
    }
    
    void AddRootSignatureCommand(ID3D12RootSignature* root)
    {
        AddHeader(D12CommandType_RootSignature);
        D12CommandRootSignature* com = AddCommand(D12CommandRootSignature);
        com->root_sig = root;
    }
    
    void AddPipelineStateCommand(ID3D12PipelineState* ps)
    {
        AddHeader(D12CommandType_PipelineState);
        D12CommandPipelineState* com = AddCommand(D12CommandPipelineState);
        com->pipeline_state = ps;
    }
    
    void AddScissorRectCommand(CD3DX12_RECT rect)
    {
        AddHeader(D12CommandType_ScissorRect);
        D12CommandScissorRect* com = AddCommand(D12CommandScissorRect);
        com->rect = rect;
    }
    
    void AddStartCommandListCommand()
    {
        AddHeader(D12CommandType_StartCommandList);
        D12CommandStartCommandList* com = AddCommand(D12CommandStartCommandList);
    }
    
    void AddEndCommandListCommand()
    {
        AddHeader(D12CommandType_EndCommandList);
        D12CommandEndCommmandList* com = AddCommand(D12CommandEndCommmandList);
    }
    
    // TODO(Ray Garner): Replace these with something later
    void AddGraphicsRootDescTable(u64 index,ID3D12DescriptorHeap* heaps,D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
    {
        AddHeader(D12CommandType_GraphicsRootDescTable);
        D12CommandGraphicsRootDescTable* com = AddCommand(D12CommandGraphicsRootDescTable);
        com->index = index;
        com->heap = heaps;
        com->gpu_handle = gpu_handle;
    };
    
    void AddGraphicsRoot32BitConstant(u32 index,u32 num_values,void* gpuptr,u32 offset)
    {
        AddHeader(D12CommandType_GraphicsRootConstant);
        D12CommandGraphicsRoot32BitConstant* com = AddCommand(D12CommandGraphicsRoot32BitConstant);
        com->index = index;
        com->num_values = num_values;
        u32 byte_count = num_values*sizeof(u32);
        void* mem_ptr = PUSHSIZE(&constants_arena,byte_count,fmj_arena_push_params_no_clear());
        uint8_t* ptr = (uint8_t*)mem_ptr;
        for(int i = 0;i < byte_count;++i)
        {
            *ptr++ = *((uint8_t*)gpuptr + i);
        }
        
        com->gpuptr = mem_ptr;
        com->offset = offset;
    };
    
    GPUMemoryResult QueryGPUFastMemory()
    {
        DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
        // NOTE(Ray Garner): Zero is only ok if we are single GPUAdaptor
        HRESULT r = dxgiAdapter4->QueryVideoMemoryInfo(
            0,
            DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
            &info);
        ASSERT(SUCCEEDED(r));
        GPUMemoryResult result = {info.Budget,info.CurrentUsage,info.AvailableForReservation,info.CurrentReservation};
        return result;
    }
    
    static IDXGIAdapter4* GetAdapter(bool useWarp)
    {
        IDXGIFactory4* dxgifactory;
        UINT create_fac_flags = 0;
#if defined(_DEBUG)
        //create_fac_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif
        
        CreateDXGIFactory2(create_fac_flags, IID_PPV_ARGS(&dxgifactory));
        IDXGIAdapter1* dxgi_adapter1;
        IDXGIAdapter4* dxgi_adapter4;
        
        if (useWarp)
        {
            dxgifactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgi_adapter1));
            dxgi_adapter4 = (IDXGIAdapter4*)dxgi_adapter1;
            //dxgi_adapter1->As(&dxgi_adapter4);
        }
        else
        {
            SIZE_T maxDedicatedVideoMemory = 0;
            for (UINT i = 0; dxgifactory->EnumAdapters1(i, &dxgi_adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
                dxgi_adapter1->GetDesc1(&dxgiAdapterDesc1);
                
                // Check to see if the adapter can create a D3D12 device without actually 
                // creating it. The adapter with the largest dedicated video memory
                // is favored.
                if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && 
                    SUCCEEDED(D3D12CreateDevice(dxgi_adapter1,D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
                    dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
                {
                    maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                    dxgi_adapter4 = (IDXGIAdapter4*)dxgi_adapter1;
                }
            }
        }
        return dxgi_adapter4;
    }
    
    bool EnableDebugLayer()
    {
#if defined(_DEBUG)
        // Always enable the debug layer before doing anything DX12 related
        // so all possible errors generated while creating DX12 objects
        // are caught by the debug layer.
        ID3D12Debug* debugInterface;
        D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
        if (!debugInterface)
        {
            ASSERT(false);
        }
        debugInterface->EnableDebugLayer();
        return true;
#endif
    }
    
    static ID3D12Device2* CreateDevice(IDXGIAdapter4* adapter)
    {
        
#if USE_DEBUG_LAYER
        EnableDebugLayer();
#endif
        ID3D12Device2* d3d12Device2;
        D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2));
        
        HRESULT WINAPI D3D12CreateDevice(
			_In_opt_  IUnknown          *pAdapter,
			D3D_FEATURE_LEVEL MinimumFeatureLevel,
			_In_      REFIID            riid,
			_Out_opt_ void              **ppDevice
            );
        
        // Enable debug messages in debug mode.
#if USE_DEBUG_LAYER
        ID3D12InfoQueue* pInfoQueue;
        HRESULT result = d3d12Device2->QueryInterface(&pInfoQueue);
        if (SUCCEEDED(result))
        {
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
            // Suppress whole categories of messages
            //D3D12_MESSAGE_CATEGORY Categories[] = {};
            
            // Suppress messages based on their severity level
            D3D12_MESSAGE_SEVERITY Severities[] =
            {
                D3D12_MESSAGE_SEVERITY_INFO
            };
            
            // Suppress individual messages by their ID
            D3D12_MESSAGE_ID DenyIds[] = {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
            };
            
            D3D12_INFO_QUEUE_FILTER NewFilter = {};
            //NewFilter.DenyList.NumCategories = _countof(Categories);
            //NewFilter.DenyList.pCategoryList = Categories;
            NewFilter.DenyList.NumSeverities = _countof(Severities);
            NewFilter.DenyList.pSeverityList = Severities;
            NewFilter.DenyList.NumIDs = _countof(DenyIds);
            NewFilter.DenyList.pIDList = DenyIds;
            
            pInfoQueue->PushStorageFilter(&NewFilter);
        }
        else
        {
            ASSERT(false);
        }
#endif
        return d3d12Device2;
    }
    
    ID3D12GraphicsCommandList* CreateCommandList(ID3D12Device2* device,ID3D12CommandAllocator* commandAllocator, D3D12_COMMAND_LIST_TYPE type)
    {
        ID3D12GraphicsCommandList* command_list;
        HRESULT result = (device->CreateCommandList(0, type, commandAllocator, nullptr, IID_PPV_ARGS(&command_list)));
#if 1
        HRESULT removed_reason = device->GetDeviceRemovedReason();
        DWORD e = GetLastError();
#endif
        command_list->Close();
        return command_list;
    }
    
    static ID3D12CommandQueue* CreateCommandQueue(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type)
    {
        ID3D12CommandQueue* d3d12CommandQueue;
        
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type =     type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags =    D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;
        DWORD error = GetLastError();
        HRESULT r = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue));
        if(!SUCCEEDED(r))
        {
            ASSERT(false);
        }
        error = GetLastError();
        /*
        ID3D12CommandQueue* d3d12CommandQueue;
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        DWORD error = GetLastError();
        (device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));
        error = GetLastError();
        */
        return d3d12CommandQueue;
    }
    
    static bool CheckTearingSupport()
    {
        BOOL allowTearing = FALSE;
        // Rather than create the DXGI 1.5 factory interface directly, we create the
        // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
        // graphics debugging tools which will not support the 1.5 factory interface 
        // until a future update.
        IDXGIFactory4* factory4;
        if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
        {
            IDXGIFactory5* factory5;
            HRESULT r = factory4->QueryInterface(&factory5);
            if (SUCCEEDED(r))
            {
                if (FAILED(factory5->CheckFeatureSupport(
					DXGI_FEATURE_PRESENT_ALLOW_TEARING,
					&allowTearing, sizeof(allowTearing))))
                {
                    allowTearing = FALSE;
                }
            }
        }
        return allowTearing == TRUE;
    }
    
    IDXGISwapChain4* CreateSwapChain(HWND hWnd,ID3D12CommandQueue* commandQueue,u32 width, u32 height, u32 bufferCount)
    {
        IDXGISwapChain4* dxgiSwapChain4;
        IDXGIFactory4* dxgiFactory4;
        UINT createFactoryFlags = 0;
#if defined(_DEBUG)
        //createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
        
        (CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc = { 1, 0 };
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = bufferCount;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        // It is recommended to always allow tearing if tearing support is available.
        swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
        ComPtr<IDXGISwapChain1> swapChain1;
        (dxgiFactory4->CreateSwapChainForHwnd(
			commandQueue,
			hWnd,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain1));
        
        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
        // will be handled manually.
        (dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
        HRESULT result = swapChain1->QueryInterface(&dxgiSwapChain4);
        ASSERT(SUCCEEDED(result));
        //(swapChain1.As(&dxgiSwapChain4));
        return dxgiSwapChain4;
    }
    
    static ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device2* device,D3D12_DESCRIPTOR_HEAP_TYPE type, u32 numDescriptors)
    {
        ID3D12DescriptorHeap* descriptorHeap;
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = numDescriptors;
        desc.Type = type;
        (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));
        return descriptorHeap;
    }
    
    static void UpdateRenderTargetViews(ID3D12Device2* device,
                                        IDXGISwapChain4* swapChain, ID3D12DescriptorHeap* descriptorHeap)
    {
        auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
        for (int i = 0; i < num_of_back_buffers; ++i)
        {
            ID3D12Resource* backBuffer;
            (swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
            device->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);
            back_buffers[i] = backBuffer;
            rtvHandle.Offset(rtvDescriptorSize);
        }
    }
    
    ID3D12Fence* CreateFence(ID3D12Device2* device)
    {
        ID3D12Fence* fence;
        (device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        return fence;
    }
    
    HANDLE CreateEventHandle()
    {
        HANDLE fenceEvent;
        fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        ASSERT(fenceEvent && "Failed to create fence event.");
        return fenceEvent;
    }
    
    u64 Signal(ID3D12CommandQueue* commandQueue, ID3D12Fence* fence,u64& fenceValue)
    {
        u64 fenceValueForSignal = ++fenceValue;
        (commandQueue->Signal(fence, fenceValueForSignal));
        return fenceValueForSignal;
    }
    
    bool IsFenceComplete(ID3D12Fence* fence,u64 fence_value)
    {
        return fence->GetCompletedValue() >= fence_value;
    }
    
    void WaitForFenceValue(ID3D12Fence* fence, u64 fenceValue, HANDLE fenceEvent,double duration = FLT_MAX)
    {
        if (IsFenceComplete(fence,fenceValue))
        {
            (fence->SetEventOnCompletion(fenceValue, fenceEvent));
            ::WaitForSingleObject(fenceEvent, duration);
        }
    }
    
    void Flush(ID3D12CommandQueue* commandQueue, ID3D12Fence* fence,u64& fenceValue, HANDLE fenceEvent)
    {
        u64 fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
        WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
    }
    
    ID3D12CommandAllocator* CreateCommandAllocator(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type)
    {
        ID3D12CommandAllocator* commandAllocator;
        HRESULT result = (device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));
#if 1
        HRESULT removed_reason = device->GetDeviceRemovedReason();
        DWORD e = GetLastError();
#endif
        ASSERT(SUCCEEDED(result));
        return commandAllocator;
    }
    
    D12CommandAllocatorEntry* AddFreeCommandAllocatorEntry(D3D12_COMMAND_LIST_TYPE type)
    {
        D12CommandAllocatorEntry entry = {};
        entry.allocator = CreateCommandAllocator(device, type);
        ASSERT(entry.allocator);
        entry.used_list_indexes = fmj_stretch_buffer_init(1, sizeof(u64),8);
        entry.fence_value = 0;
        entry.thread_id = fmj_thread_get_thread_id();
        entry.type = type;
        entry.index = current_allocator_index++;
        D12CommandAllocatorKey key = {(u64)entry.allocator,entry.thread_id};
        //TODO(Ray):Why is the key parameter backwards here?
        
        fmj_anycache_add_to_free_list(&allocator_tables.fl_ca,&key,&entry);
//        D12CommandAllocatorEntry* result = (D12CommandAllocatorEntry*)AnythingCacheCode::GetThing(&allocator_tables.fl_ca, &key);
        D12CommandAllocatorEntry* result = (D12CommandAllocatorEntry*)fmj_anycache_get_(&allocator_tables.fl_ca, &key);
        ASSERT(result);
        return  result;
    }
    
    bool GetCAPredicateCOPY(void* ca)
    {
        D12CommandAllocatorEntry* entry = (D12CommandAllocatorEntry*)ca;
        if(entry->type == D3D12_COMMAND_LIST_TYPE_COPY && IsFenceComplete(upload_operations.fence,upload_operations.fence_value))
        {
            return true;
        }
        return false;
    }
    
    bool GetCAPredicateDIRECT(void* ca)
    {
        D12CommandAllocatorEntry* entry = (D12CommandAllocatorEntry*)ca;
        if(entry->type == D3D12_COMMAND_LIST_TYPE_DIRECT && IsFenceComplete(fence,entry->fence_value))
        {
            return true;
        }
        return false;
    }
    
    FMJStretchBuffer* GetTableForType(D3D12_COMMAND_LIST_TYPE type)
    {
        FMJStretchBuffer* table;
        if(type == D3D12_COMMAND_LIST_TYPE_DIRECT)
        {
            table = &allocator_tables.free_allocator_table;
        }
        else if(type == D3D12_COMMAND_LIST_TYPE_COPY)
        {
            table = &allocator_tables.free_allocator_table_copy;
        }
        else if(type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
        {
            table = &allocator_tables.free_allocator_table_compute;
        }
        return table;
    }
    
    D12CommandAllocatorEntry* GetFreeCommandAllocatorEntry(D3D12_COMMAND_LIST_TYPE  type)
    {
        D12CommandAllocatorEntry* result;
        //Forget the free list we will access the table directly and get the first free
        //remove and make a inflight allocator table.
        //This does not work with free lists due to the fact taht the top element might always be busy
        //in some cases causing the infinite allocation of command allocators.
        //result = GetFirstFreeWithPredicate(D12CommandAllocatorEntry,allocator_tables.fl_ca,GetCAPredicateDIRECT);
        FMJStretchBuffer* table = GetTableForType(type);
            
        if(table->fixed.count <= 0)
        {
            result = 0;
        }
        else
        {
            // NOTE(Ray Garner): We assume if we get a free one you WILL use it.
            //otherwise we will need to do some other bookkeeping.
            //result = *YoyoPeekVectorElementPtr(D12CommandAllocatorEntry*,table);
            result = *(D12CommandAllocatorEntry**)fmj_stretch_buffer_get_(table,table->fixed.count - 1);
            if(!IsFenceComplete(fence,(result)->fence_value))
            {
                result = 0;
            }
            else
            {
                fmj_stretch_buffer_pop(table);
            }
        }
        if (!result)
        {
            result = AddFreeCommandAllocatorEntry(type);
        }
        ASSERT(result);
        return result;
    }
    
    void CheckReuseCommandAllocators()
    {
        fmj_stretch_buffer_clear(&allocator_tables.free_allocator_table);
        fmj_stretch_buffer_clear(&allocator_tables.free_allocator_table_compute);
        fmj_stretch_buffer_clear(&allocator_tables.free_allocator_table_copy);                
        for(int i = 0;i < allocator_tables.fl_ca.anythings.fixed.count;++i)
        {
            D12CommandAllocatorEntry* entry = (D12CommandAllocatorEntry*)allocator_tables.fl_ca.anythings.fixed.base + i;
            //Check the fence values
            if(IsFenceComplete(fence,entry->fence_value))
            {
                FMJStretchBuffer* table = GetTableForType(entry->type);
                //if one put them on the free table for reuse.
                fmj_stretch_buffer_push(table,(void*)&entry);
            }
        }
    }
    
    void CheckFeatureSupport(D12Resource* resource)
    {
        if (resource && resource->state)
        {
            auto desc = resource->state->GetDesc();
            resource->format_support.Format = desc.Format;
            HRESULT hr = device->CheckFeatureSupport(
                D3D12_FEATURE_FORMAT_SUPPORT,
                &resource->format_support,
                sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT));
            ASSERT(SUCCEEDED(hr));
        }
        else
        {
            resource->format_support = {};
        }
    }
    
    static CommandAllocToListResult GetFirstAssociatedList(D12CommandAllocatorEntry* allocator)
    {
        CommandAllocToListResult result = {};
        bool found = false;
        //Run through all the list that are associated with an allocator check for first available list
        for (int i = 0; i < allocator_tables.allocator_to_list_table.fixed.count; ++i)
        {
            D12CommandAlloctorToCommandListKeyEntry* entry = (D12CommandAlloctorToCommandListKeyEntry*)allocator_tables.allocator_to_list_table.fixed.base + i;
            if (allocator->index == entry->command_list_index)
            {
                //D12CommandAllocatorEntry* caentry = YoyoGetVectorElement(D12CommandAllocatorEntry, &allocator_tables.allocators, entry->command_allocator_index);
                D12CommandListEntry* e = (D12CommandListEntry*)fmj_stretch_buffer_get_(&allocator_tables.command_lists, entry->command_list_index);
                ASSERT(e);
                if (!e->is_encoding)
                {
                    result.list = *e;
                    //Since at this point this allocator should have all command list associated with it finished processing we can just grab the first command list.
                    //and use it.
                    result.index = entry->command_list_index;
                    found = true;
                }
                break;
            }
        }
        result.found = found;
        //TODO(Ray):Create some validation feedback here that can be removed at compile time. 
        return result;
    }
    
    u32 GetCurrentBackBufferIndex()
    {
        return swap_chain->GetCurrentBackBufferIndex();
    }
    
    ID3D12Resource* GetCurrentBackBuffer()
    {
        u32 bbi = GetCurrentBackBufferIndex();
        ID3D12Resource* result = back_buffers[bbi];
        ASSERT(result);
        return result;
    }
    
    
    inline ID3D12Device2* GetDevice()
    {
        return device;
    }
    
    D12ResourceTables resource_tables;
    
    FMJTicketMutex resource_ticket_mutex;
    u64 resource_id;
    
    // NOTE(Ray Garner): //Make sure this access is thread safe
    u64 GetNewResourceID()
    {
        fmj_thread_begin_ticket_mutex(&resource_ticket_mutex);
        ++resource_id;
        fmj_thread_end_ticket_mutex(&resource_ticket_mutex);
        return resource_id;
    }
    
    //render encoder
    D12CommandListEntry GetAssociatedCommandList(D12CommandAllocatorEntry* ca)
    {
        CommandAllocToListResult listandindex = GetFirstAssociatedList(ca);
        D12CommandListEntry command_list_entry = listandindex.list;
        u64 cl_index = listandindex.index;
        if(!listandindex.found)
        {
            command_list_entry.list = CreateCommandList(device, ca->allocator, ca->type);
            command_list_entry.is_encoding = true;
            command_list_entry.index = allocator_tables.command_lists.fixed.count;
            command_list_entry.temp_resources = fmj_stretch_buffer_init(1,sizeof(ID3D12Object*),8);
            
            cl_index = fmj_stretch_buffer_push(&allocator_tables.command_lists, (void*)&command_list_entry);
            D12CommandAlloctorToCommandListKeyEntry a_t_l_e = {};
            a_t_l_e.command_allocator_index = ca->index;
            a_t_l_e.command_list_index = cl_index;
            fmj_stretch_buffer_push(&allocator_tables.allocator_to_list_table, (void*)&a_t_l_e);
        }
        fmj_stretch_buffer_push(&ca->used_list_indexes, (void*)&cl_index);
        return command_list_entry;
    }
    
    void Init()
    {
        tex_heap_count = 0;
        allocator_tables = {};
        allocator_tables.free_allocator_table = fmj_stretch_buffer_init(1,sizeof(D12CommandAllocatorEntry*),8);
        allocator_tables.free_allocator_table_copy = fmj_stretch_buffer_init(1,sizeof(D12CommandAllocatorEntry*),8);
        allocator_tables.free_allocator_table_compute = fmj_stretch_buffer_init(1,sizeof(D12CommandAllocatorEntry*),8);
        
        allocator_tables.free_allocators = fmj_stretch_buffer_init(1, sizeof(D12CommandAllocatorEntry), 8);
        allocator_tables.command_lists = fmj_stretch_buffer_init(1, sizeof(D12CommandListEntry), 8);
        allocator_tables.allocator_to_list_table = fmj_stretch_buffer_init(1, sizeof(D12CommandAlloctorToCommandListKeyEntry), 8);
        temp_queue_command_list = fmj_stretch_buffer_init(1, sizeof(ID3D12GraphicsCommandList*),8);

        //AnythingCacheCode::Init(&allocator_tables.fl_ca, 4096, sizeof(D12CommandAllocatorEntry), sizeof(D12CommandAllocatorKey), true);
        allocator_tables.fl_ca = fmj_anycache_init(4096,sizeof(D12CommandAllocatorEntry), sizeof(D12CommandAllocatorKey), true);

        //render_com_buf.arena = PlatformAllocatePartition(MegaBytes(2));
        render_com_buf.arena = fmj_arena_allocate(FMJMEGABYTES(2));
        
        //Resource bookkeeping
        resource_ca = D12RendererCode::CreateCommandAllocator(device,D3D12_COMMAND_LIST_TYPE_COPY);
        
        resource_cl = 
            D12RendererCode::CreateCommandList(device,resource_ca,D3D12_COMMAND_LIST_TYPE_COPY);
        
        resource_ca->Reset();
        resource_cl->Reset(resource_ca,nullptr);
        
        upload_operations = {};
//        AnythingCacheCode::Init(&upload_operations.table_cache,4096,sizeof(UploadOp),sizeof(UploadOpKey),true);
        upload_operations.table_cache = fmj_anycache_init(4096,sizeof(UploadOp),sizeof(UploadOpKey),true);
        
        upload_operations.ticket_mutex = {};
        upload_operations.current_op_id = 1;
        upload_operations.fence_value = 0;
        upload_operations.fence = CreateFence(device);
        upload_operations.fence_event = CreateEventHandle();
        
        //This is the global resources cache for direcx12 
        //resources
//        AnythingCacheCode::Init(&resource_tables.resources,4096,sizeof(D12Resource),sizeof(D12ResourceKey));
        resource_tables.resources = fmj_anycache_init(4096,sizeof(D12Resource),sizeof(D12ResourceKey),true);
        //This is the perframe perthread resource state tracking of subresources
        //in use by command list about to be excecuted on this frame.
        //will be reset ever frame after command list execution.
        resource_tables.per_thread_sub_resource_state = fmj_stretch_buffer_init(1,sizeof(D12ResourceStateEntry),8);
        //This it the inter thread and intra list sub resource state 
        //for each resource. at the end of every use by a list on ever thread there is a final state.
        //We put that here so that when executing it can be cross reference and checked to see if the sub resource is in the proper state BEFORE its used in the command list.
        //If not we create a new command list to transition that sub resource into the proper state ONLY if its needed other wise ensure we do absolutely nothing.
        resource_tables.global_sub_resrouce_state =  fmj_stretch_buffer_init(1,sizeof(D12ResourceStateEntry),8);
        is_resource_cl_recording = true;
        
        constants_arena = fmj_arena_allocate(FMJMEGABYTES(4));
        
        //Descriptor heap for our resources
        // create the descriptor heap that will store our srv
        default_srv_desc_heap = CreateDescriptorHeap(MAX_SRV_DESC_HEAP_COUNT,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        
        
        D12ResourceState::Init(device);
    }
    
    D12Resource AddResource(ID3D12Resource* resource)
    {
        u64 r_id = (u64)resource;
        u32 thread_id = fmj_thread_get_thread_id();
        D12ResourceKey key = {r_id,thread_id};
        D12Resource new_resource = {};
        new_resource.id = 0;//not added due to dont want to stall the cpu..waiting to generate an id.
        new_resource.state = resource;
        new_resource.thread_id = thread_id;
        CheckFeatureSupport(&new_resource);
//        AnythingCacheCode::AddThingFL(&resource_tables.resources,&key,&new_resource);
        fmj_anycache_add_to_free_list(&resource_tables.resources,&key,&new_resource);
        return new_resource;
    }
    
    void AddGlobalSubResourceState(D12Resource resource,D3D12_RESOURCE_STATES state)
    {
        D12ResourceStateEntry e = {};
        e.resource = resource;
        e.state = state;
        fmj_stretch_buffer_push(&resource_tables.global_sub_resrouce_state,(void*)&e);
    }
    
    void AddSubResourceState(D12CommandListEntry cl_entry,D12Resource resource,D3D12_RESOURCE_STATES state)
    {
        D12ResourceStateEntry e = {};
        e.resource = resource;
        e.list = cl_entry;
        e.state = state;
        fmj_stretch_buffer_push(&resource_tables.per_thread_sub_resource_state,(void*)&e);
    }
    
    bool CheckFormatSupport(D12Resource resource,D3D12_FORMAT_SUPPORT1 formatSupport) 
    {
        return (resource.format_support.Support1 & formatSupport) != 0;
    }
    
    bool CheckFormatSupport(D12Resource resource,D3D12_FORMAT_SUPPORT2 formatSupport) 
    {
        return (resource.format_support.Support2 & formatSupport) != 0;
    }
    
    bool CheckUAVSupport(D12Resource resource)
    {
        return CheckFormatSupport(resource,D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
            CheckFormatSupport(resource,D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) &&
            CheckFormatSupport(resource,D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
    }
    
    DXGI_FORMAT GetUAVCompatableFormat(DXGI_FORMAT format)
    {
        DXGI_FORMAT result = format;
        switch (format)
        {
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_B8G8R8X8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            result = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;
            case DXGI_FORMAT_D32_FLOAT:
            result = DXGI_FORMAT_R32_FLOAT;
            break;
        }
        return result;
    }
    
    void GenerateMips(D12Resource texture)
    {
        //We know we will always generate mips on teh computer engine queue.
        D12CommandAllocatorEntry* ca_entry = GetFreeCommandAllocatorEntry(D3D12_COMMAND_LIST_TYPE_COMPUTE);
        D12CommandListEntry cl_entry = GetAssociatedCommandList(ca_entry);
        
        //if no valid resource do nothing
        auto resource = texture.state;
        if(!resource)return;
        auto resource_desc = resource->GetDesc();
        
        //if texture only has a single mip level do nothing.
        if(resource_desc.MipLevels == 1)return;
        //Only non multi sampled texture are support atm
        if(resource_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || 
           resource_desc.DepthOrArraySize != 1 || 
           resource_desc.SampleDesc.Count > 1)
        {
            ASSERT(false);
        }
        
        ID3D12Resource* uav_resource = resource;
        auto alias_desc = resource_desc;
        
        auto uav_desc = alias_desc;   
        // Create an alias of the original resource.
        // This is done to perform a GPU copy of resources with different formats.
        // BGR -> RGB texture copies will fail GPU validation unless performed 
        // through an alias of the BRG resource in a placed heap.
        ID3D12Resource* alias_resource;
        //If the passed in resource does not allow for UAV access than create a staging resource that is used to create the mip map chain.
        if(CheckUAVSupport(texture) || (resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
        {
            // Describe an alias resource that is used to copy the original texture.
            // Placed resources can't be render targets or depth-stencil views.
            alias_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            alias_desc.Flags &= ~(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
            
            // Describe a UAV compatible resource that is used to perform
            // mipmapping of the original texture.
            // The flags for the UAV description must match that of the alias description.
            uav_desc.Format = GetUAVCompatableFormat(resource_desc.Format);
            
            D3D12_RESOURCE_DESC resource_descs[] = 
            {
                alias_desc,
                uav_desc
            };
            
            // Create a heap that is large enough to store a copy of the original resource.
            auto allocation_info = device->GetResourceAllocationInfo(0, _countof(resource_descs), resource_descs );
            
            D3D12_HEAP_DESC heap_desc = {};
            heap_desc.SizeInBytes = allocation_info.SizeInBytes;
            heap_desc.Alignment = allocation_info.Alignment;
            heap_desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
            heap_desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
            
            ID3D12Heap* t_heap;
            device->CreateHeap(&heap_desc, IID_PPV_ARGS(&t_heap));
            
            // Make sure the heap does not go out of scope until the command list
            // is finished executing on the command queue.
            
            //removed as soon as the command list is finished executing.
            D12ResourceState::TrackResource(&cl_entry,t_heap);
            
            // Create a placed resource that matches the description of the 
            // original resource. This resource is used to copy the original 
            // texture to the UAV compatible resource.
            device->CreatePlacedResource(
                t_heap,
                0,
                &alias_desc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&alias_resource)
                );
            D12Resource a_r = AddResource(alias_resource);
            
            AddGlobalSubResourceState(a_r,D3D12_RESOURCE_STATE_COMMON);
            
            //removed as soon as the command list is finished executing.
            D12ResourceState::TrackResource(&cl_entry,alias_resource);
            
            // Create a UAV compatible resource in the same heap as the alias
            device->CreatePlacedResource(
                t_heap,
                0,
                &uav_desc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&uav_resource));
            D12Resource u_r = AddResource(uav_resource);
            
            //AddGlobalSubResourceState(u_r,D3D12_RESOURCE_STATE_COMMON);
            
            //removed as soon as the command list is finished executing.
            D12ResourceState::TrackResource(&cl_entry,uav_resource);
            //auto barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(nullptr, alias_resource);
            D12ResourceState::AliasingBarrier(&cl_entry,nullptr,alias_resource);
            
            cl_entry.list->CopyResource(alias_resource, uav_resource);
            //auto alias_uav_barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(alias_resource, uav_resource);
            D12ResourceState::AliasingBarrier(&cl_entry,alias_resource,uav_resource);
        }
        
        cl_entry.list->SetPipelineState(D12ResourceState::pipeline_state);
        //SetComputeRootSignature( m_GenerateMipsPSO->GetRootSignature() );
        cl_entry.list->SetComputeRootSignature(D12ResourceState::root_sig.state);
        
        //Check if texture is SRGB format
        GenerateMipsCB generateMipsCB;
        // TODO(Ray Garner): Properly get this
        generateMipsCB.IsSRGB = D12ResourceState::IsSRGBFormat(uav_desc.Format);
        
        //ID3D12Resource* resource = uav_resource;//texture.GetD3D12Resource();
        auto resourceDesc = uav_desc;//resource->GetDesc();
        
        // Create an SRV that uses the format of the original texture.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = generateMipsCB.IsSRGB ? D12ResourceState::GetSRGBFormat(resourceDesc.Format) : resourceDesc.Format;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;  // Only 2D textures are supported (this was checked in the calling function).
        srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
        
        for ( u32 srcMip = 0; srcMip < resourceDesc.MipLevels - 1u; )
        {
            u64 srcWidth = resourceDesc.Width >> srcMip;
            u32 srcHeight = resourceDesc.Height >> srcMip;
            u32 dstWidth = static_cast<u32>( srcWidth >> 1 );
            u32 dstHeight = srcHeight >> 1;
            // 0b00(0): Both width and height are even.
            // 0b01(1): Width is odd, height is even.
            // 0b10(2): Width is even, height is odd.
            // 0b11(3): Both width and height are odd.
            generateMipsCB.SrcDimension = ( srcHeight & 1 ) << 1 | ( srcWidth & 1 );
            // How many mipmap levels to compute this pass (max 4 mips per pass)
            u32 mipCount;
            
            // The number of times we can half the size of the texture and get
            // exactly a 50% reduction in size.
            // A 1 bit in the width or height indicates an odd dimension.
            // The case where either the width or the height is exactly 1 is handled
            // as a special case (as the dimension does not require reduction).
            _BitScanForward( &(DWORD)mipCount, ( dstWidth == 1 ? dstHeight : dstWidth ) | 
                            ( dstHeight == 1 ? dstWidth : dstHeight ) );
            // Maximum number of mips to generate is 4.
            mipCount = (u32)min( 4, mipCount + 1 );
            // Clamp to total number of mips left over.
            mipCount = ( srcMip + mipCount ) >= resourceDesc.MipLevels ? 
                resourceDesc.MipLevels - srcMip - 1 : mipCount;
            
            // Dimensions should not reduce to 0.
            // This can happen if the width and height are not the same.
            dstWidth =  max( 1, dstWidth );
            dstHeight = max( 1, dstHeight );
            
            generateMipsCB.SrcMipLevel = srcMip;
            generateMipsCB.NumMipLevels = mipCount;
            generateMipsCB.TexelSize = f2_create(1.0f / (float)dstWidth,1.0f / (float)dstHeight);
            cl_entry.list->SetComputeRoot32BitConstants(0, sizeof(GenerateMipsCB) / sizeof(u32),&generateMipsCB  ,0);
        }
    }
    
    GPUArena AllocateGPUArena(u64 size);
    void UploadBufferData(GPUArena* g_arena,void* data,u64 size);
    
    CreateDeviceResult Init(HWND* window,f2 dim)
    {
        CreateDeviceResult result = {};
        dxgiAdapter4 = GetAdapter(g_UseWarp);
        device = CreateDevice(dxgiAdapter4);

        command_queue = CreateCommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
        copy_command_queue = CreateCommandQueue(device, D3D12_COMMAND_LIST_TYPE_COPY);
        compute_command_queue = CreateCommandQueue(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
        
        swap_chain = D12RendererCode::CreateSwapChain(*window, command_queue,dim.x, dim.y, num_of_back_buffers);
        u32 current_back_buffer_index = swap_chain->GetCurrentBackBufferIndex();
        rtv_descriptor_heap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, num_of_back_buffers);
        rtv_desc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        UpdateRenderTargetViews(device, swap_chain, rtv_descriptor_heap);
        
        fence = CreateFence(device);
        fence_event = CreateEventHandle();
        
        result.is_init = true;
        Init();
        result.device.device = device;
        return result;
    }
    
    // NOTE(Ray Garner): // TODO(Ray Garner): Use Placed resources and implement proper
    //memory control techniques. Pre allocate Texture and Vertex buffer resources set a fixed size.
    //If we go over budget assert and reasses at that point.
    // NOTE(Ray Garner): Nvidia reccomends using commited resources whereever possible so for now we will 
    //stick with that until we find a good reason to do otherwise.
    GPUArena AllocateGPUArena(u64 size)
    {
        GPUArena result = {};
        size_t bufferSize = size;
        D3D12_HEAP_PROPERTIES hp =  
        {
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            0,
            0
        };
        
        DXGI_SAMPLE_DESC sample_d =  
        {
            1,
            0
        };
        
        D3D12_RESOURCE_DESC res_d =  
        {
            D3D12_RESOURCE_DIMENSION_BUFFER,
            0,
            size,
            1,
            1,
            1,
            DXGI_FORMAT_UNKNOWN,
            sample_d,
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            D3D12_RESOURCE_FLAG_NONE,
        };
        
        // Create a committed resource for the GPU resource in a default heap.
        HRESULT r = (device->CreateCommittedResource(
            &hp,
            D3D12_HEAP_FLAG_NONE,
            &res_d,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&result.resource)));
        ASSERT(SUCCEEDED(r));
        return result;
    }
    
    FMJStretchBuffer temp_uop_keys;
    
    // NOTE(Ray Garner): This is kind of like the OpenGL Texture 2D
    // we will make space on the gpu and upload the texture from cpu
    //to gpu right away. LoadedTexture is like the descriptor and also
    //holds a pointer to the texels on cpu.
    void Texture2D(LoadedTexture* lt)
    {
        D12CommandAllocatorEntry* free_ca  = GetFreeCommandAllocatorEntry(D3D12_COMMAND_LIST_TYPE_COPY);
        resource_ca = free_ca->allocator;
        if(!is_resource_cl_recording)
        {
            resource_ca->Reset();
            resource_cl->Reset(resource_ca,nullptr);
            is_resource_cl_recording = true;
        }
        
        D3D12_HEAP_PROPERTIES hp =  
        {
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            0,
            0
        };
        
        DXGI_SAMPLE_DESC sample_d =  
        {
            1,
            0
        };
        
        D3D12_RESOURCE_DESC res_d = {};  
        res_d = CD3DX12_RESOURCE_DESC::Tex2D( 
            DXGI_FORMAT_R8G8B8A8_UNORM,(u64)lt->dim.x,(u64)lt->dim.y,1);
        
        D12Resource tex_resource;
        UploadOp uop = {};
        
        HRESULT hr = device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &res_d,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&tex_resource.state));
        ASSERT(SUCCEEDED(hr));
        
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2 = {};
        srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc2.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc2.Texture2D.MipLevels = 1;

        u32 hmdh_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        
        D3D12_CPU_DESCRIPTOR_HANDLE hmdh = default_srv_desc_heap->GetCPUDescriptorHandleForHeapStart();
        
        // TODO(Ray Garner): We will have to properly implement this later! For now we just keep adding texture as we cant remove them yet.

        hmdh.ptr += (hmdh_size * tex_heap_count);
        u32 first_free_texture_slot = tex_heap_count;        
        lt->slot = first_free_texture_slot;
        tex_heap_count++;
        
        device->CreateShaderResourceView((ID3D12Resource*)tex_resource.state, &srvDesc2, hmdh);
        
        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = lt->texels;
        // TODO(Ray Garner): Handle minimum size for alignment.
        //This wont work for a smaller texture im pretty sure.
        subresourceData.RowPitch = lt->dim.x * lt->bytes_per_pixel;
        subresourceData.SlicePitch = subresourceData.RowPitch;
        
        UINT64 req_size = GetRequiredIntermediateSize( tex_resource.state, 0, 1);
        // Create a temporary (intermediate) resource for uploading the subresources
        ID3D12Resource* intermediate_resource;
        hr = device->CreateCommittedResource(
            &hp,
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer( req_size ),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            0,
            IID_PPV_ARGS( &uop.temp_arena.resource ));
        
        uop.temp_arena.resource->SetName(L"TEMP_UPLOAD_TEXTURE");
        
        ASSERT(SUCCEEDED(hr));
        
        hr = UpdateSubresources(resource_cl, 
                                tex_resource.state, uop.temp_arena.resource,
                                (u32)0, (u32)0, (u32)1, &subresourceData);
        
        CheckFeatureSupport(&tex_resource);
        
        //if (tex_resource.state->GetDesc().MipLevels && lt->dim.x() > 2 && lt->dim.y() > 2)
        {
            //GenerateMips(tex_resource);
        }
        
        lt->texture.state = tex_resource.state;
        fmj_thread_begin_ticket_mutex(&upload_operations.ticket_mutex);
        uop.id = upload_operations.current_op_id++;
        UploadOpKey k = {uop.id};
        //AnythingCacheCode::AddThingFL(&upload_operations.table_cache,&k,&uop);
        fmj_anycache_add_to_free_list(&upload_operations.table_cache,&k,&uop);
        // NOTE(Ray Garner): Implement this.
        //if(upload_ops.anythings.count > UPLOAD_OP_THRESHOLD)
        {
            if(is_resource_cl_recording)
            {
                resource_cl->Close();
                is_resource_cl_recording = false;
            }
            ID3D12CommandList* const command_lists[] = {
                resource_cl
            };
            D12RendererCode::copy_command_queue->ExecuteCommandLists(_countof(command_lists), command_lists);
            upload_operations.fence_value = D12RendererCode::Signal(copy_command_queue, upload_operations.fence, upload_operations.fence_value);
            
            D12RendererCode::WaitForFenceValue(upload_operations.fence, upload_operations.fence_value, upload_operations.fence_event);
            
            //If we have gotten here we remove the temmp transient resource. and remove them from the cache
            for(int i = 0;i < upload_operations.table_cache.anythings.fixed.count;++i)
            {
                UploadOp *finished_uop = (UploadOp*)fmj_stretch_buffer_get_(&upload_operations.table_cache.anythings,i);
                // NOTE(Ray Garner): Upload should always be a copy operation and so we cant/dont need to 
                //call discard resource.
                // TODO(Ray Garner): Figure out how to properly release this
                //finished_uop->temp_arena.resource->Release();
                UploadOpKey k = {finished_uop->id};
                //AnythingCacheCode::RemoveThingFL(&upload_operations.table_cache,&k);
                fmj_anycache_remove_free_list(&upload_operations.table_cache,&k);
            }
            //AnythingCacheCode::ResetCache(&upload_operations.table_cache);
            fmj_anycache_reset(&upload_operations.table_cache);
        }
        fmj_thread_end_ticket_mutex(&upload_operations.ticket_mutex);
    }
/*    
    void UploadShaderResourceBuffer(GPUArena* g_arena,void* data,u64 size)
    {
        D12CommandAllocatorEntry* free_ca  = GetFreeCommandAllocatorEntry(D3D12_COMMAND_LIST_TYPE_COPY);
        resource_ca = free_ca->allocator;
        if(!is_resource_cl_recording)
        {
            resource_ca->Reset();
            resource_cl->Reset(resource_ca,nullptr);
            is_resource_cl_recording = true;
        }
        
        D3D12_HEAP_PROPERTIES hp =  
        {
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            0,
            0
        };
        
        DXGI_SAMPLE_DESC sample_d =  
        {
            1,
            0
        };
        
        D3D12_RESOURCE_DESC res_d = {};  
        res_d = CD3DX12_RESOURCE_DESC::Buffer(UINT64 width,D3D12_RESOURCE_FLAG_NONE,0);
        
        D12Resource buffer_resource;
        UploadOp uop = {};
        
        HRESULT hr = device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &res_d,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&tex_resource.state));
        ASSERT(SUCCEEDED(hr));
        
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2 = {};
        srvDesc2.Format = DXGI_FORMAT_UNKNOWN;//DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;//D3D12_SRV_DIMENSION_TEXTURE2D;

        u32 hmdh_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        
        // TODO(Ray Garner): We will have to properly implement this later! For now we just keep adding texture as we cant remove them yet.

        hmdh.ptr += (hmdh_size * tex_heap_count);
        u32 first_free_texture_slot = tex_heap_count;        
        lt->slot = first_free_texture_slot;
        tex_heap_count++;

        device->CreateShaderResourceView((ID3D12Resource*)buffer_resource.state, &srvDesc2, hmdh);        
        
        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = gpu_arena.base;
        // TODO(Ray Garner): Handle minimum size for alignment.
        //This wont work for a smaller texture im pretty sure.
        subresourceData.RowPitch = gpu_arena.size;//lt->dim.x * lt->bytes_per_pixel;
//        subresourceData.SlicePitch = subresourceData.RowPitch;
        UINT64 req_size = GetRequiredIntermediateSize( buffer_resource.state, 0, 1);
                // Create a temporary (intermediate) resource for uploading the subresources
        ID3D12Resource* intermediate_resource;
        hr = device->CreateCommittedResource(
            &hp,
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer( req_size ),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            0,
            IID_PPV_ARGS( &uop.temp_arena.resource ));
        
        uop.temp_arena.resource->SetName(L"BUFFER TEMP_UPLOAD_TEXTURE");
        ASSERT(SUCCEEDED(hr));
        
    }
*/
    
    void UploadBufferData(GPUArena* g_arena,void* data,u64 size)
    {
        D12CommandAllocatorEntry* free_ca  = GetFreeCommandAllocatorEntry(D3D12_COMMAND_LIST_TYPE_COPY);
        resource_ca = free_ca->allocator;
        
        if(!is_resource_cl_recording)
        {
            resource_ca->Reset();
            resource_cl->Reset(resource_ca,nullptr);
            is_resource_cl_recording = true;
        }
        
        D3D12_HEAP_PROPERTIES hp =  
        {
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            0,
            0
        };
        
        DXGI_SAMPLE_DESC sample_d =  
        {
            1,
            0
        };
        
        D3D12_RESOURCE_DESC res_d =  
        {
            D3D12_RESOURCE_DIMENSION_BUFFER,
            0,
            size,
            1,
            1,
            1,
            DXGI_FORMAT_UNKNOWN,
            sample_d,
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            D3D12_RESOURCE_FLAG_NONE,
        };
        
        UploadOp uop = {};
        uop.arena = *g_arena;
        
        HRESULT hr = device->CreateCommittedResource(
            &hp,
            D3D12_HEAP_FLAG_NONE,
            &res_d,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uop.temp_arena.resource));
        
        uop.temp_arena.resource->SetName(L"TEMP_UPLOAD_BUFFER");
        
        ASSERT(SUCCEEDED(hr));
        
        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = data;
        subresourceData.RowPitch = size;
        subresourceData.SlicePitch = subresourceData.RowPitch;
        
        hr = UpdateSubresources(resource_cl, 
                                g_arena->resource, uop.temp_arena.resource,
                                (u32)0, (u32)0, (u32)1, &subresourceData);
        // NOTE(Ray Garner): We will batch as many texture ops into one command list as possible 
        //and only after we have reached a signifigant amout flush the commands.
        //and do a final check at the end of the frame to flush any that were not flushed earlier.
        //meaning we batch as many as possible per frame but never wait long than one frame to batch.
        
        fmj_thread_begin_ticket_mutex(&upload_operations.ticket_mutex);
        uop.id = upload_operations.current_op_id++;
        UploadOpKey k = {uop.id};
        //AnythingCacheCode::AddThingFL(&upload_operations.table_cache,&k,&uop);
        fmj_anycache_add_to_free_list(&upload_operations.table_cache,&k,&uop);
        //if(upload_ops.anythings.count > UPLOAD_OP_THRESHOLD)
        {
            if(is_resource_cl_recording)
            {
                resource_cl->Close();
                is_resource_cl_recording = false;
            }
            ID3D12CommandList* const command_lists[] = {
                resource_cl
            };
            D12RendererCode::copy_command_queue->ExecuteCommandLists(_countof(command_lists), command_lists);
            upload_operations.fence_value = D12RendererCode::Signal(copy_command_queue, upload_operations.fence, upload_operations.fence_value);
            
            D12RendererCode::WaitForFenceValue(upload_operations.fence, upload_operations.fence_value, upload_operations.fence_event);
            
            //If we have gotten here we remove the temmp transient resource. and remove them from the cache
            for(int i = 0;i < upload_operations.table_cache.anythings.fixed.count;++i)
            {
                UploadOp *finished_uop = (UploadOp*)fmj_stretch_buffer_get_(&upload_operations.table_cache.anythings,i);
                // NOTE(Ray Garner): Upload should always be a copy operation and so we cant/dont need to 
                //call discard resource.
                
                //finished_uop->temp_arena.resource->Release();
                UploadOpKey k = {finished_uop->id};
                //AnythingCacheCode::RemoveThingFL(&upload_operations.table_cache,&k);
                fmj_anycache_remove_free_list(&upload_operations.table_cache,&k);
            }
//            AnythingCacheCode::ResetCache(&upload_operations.table_cache);
            fmj_anycache_reset(&upload_operations.table_cache);
        }
        
//        EndTicketMutex(&upload_operations.ticket_mutex);
        fmj_thread_end_ticket_mutex(&upload_operations.ticket_mutex);
    }
    
    void SetArenaToVertexBufferView(GPUArena* g_arena,u64 size,u32 stride)
    {
        g_arena->buffer_view = 
        {
            g_arena->resource->GetGPUVirtualAddress(),
            (UINT)size,
            (UINT)stride//(UINT)24
        };
    }
    
    void SetArenaToIndexVertexBufferView(GPUArena* g_arena,u64 size,DXGI_FORMAT format)
    {
        D3D12_INDEX_BUFFER_VIEW index_buffer_view;
        index_buffer_view.BufferLocation = g_arena->resource->GetGPUVirtualAddress();
        index_buffer_view.SizeInBytes = size;
        index_buffer_view.Format = format;
        g_arena->index_buffer_view = index_buffer_view;
    }
    
    void EndCommandListEncodingAndExecute(D12CommandAllocatorEntry* ca,D12CommandListEntry cl)
    {
        //Render encoder end encoding
        u64 index = cl.index;
        D12CommandListEntry* le = (D12CommandListEntry*)fmj_stretch_buffer_get_(&allocator_tables.command_lists, index);
        le->list->Close();
        le->is_encoding = false;
        
        ID3D12CommandList* const commandLists[] = {
            cl.list
        };
        
        for (int i = 0; i < ca->used_list_indexes.fixed.count; ++i)
        {
            u64 index = *((u64*)ca->used_list_indexes.fixed.base + i);
            D12CommandListEntry* cle = (D12CommandListEntry*)fmj_stretch_buffer_get_(&allocator_tables.command_lists,index);
            fmj_stretch_buffer_push(&temp_queue_command_list, (void*)&cle->list);
        }
        
        ID3D12CommandList* const* temp = (ID3D12CommandList * const*)temp_queue_command_list.fixed.base;
        command_queue->ExecuteCommandLists(temp_queue_command_list.fixed.count, temp);
        HRESULT removed_reason = device->GetDeviceRemovedReason();
        DWORD e = GetLastError();
        
        fmj_stretch_buffer_clear(&temp_queue_command_list);
        fmj_stretch_buffer_clear(&ca->used_list_indexes);
    }
    
    // TODO(Ray Garner): // NOTE(Ray Garner): It has been reccommended that we store tranistions until the
    //copy draw dispatch or push needs to be executed and deffer them to the last minute as possible as 
    //batches. For now we ignore that advice but will come back to that later.
    void TransitionResource(D12CommandListEntry cle,ID3D12Resource* resource,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to)
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            resource,
            from, to);
        cle.list->ResourceBarrier(1, &barrier);
    }
    
    void EndFrame(DrawTest *draw_test)
    {
        fmj_thread_begin_ticket_mutex(&upload_operations.ticket_mutex);
        u32 current_backbuffer_index = D12RendererCode::GetCurrentBackBufferIndex();
        if(is_resource_cl_recording)
        {
            resource_cl->Close();
            is_resource_cl_recording = false;
        }
        //Prepare
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(D12RendererCode::rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
                                          current_backbuffer_index, D12RendererCode::rtv_desc_size);
        
        D3D12_CPU_DESCRIPTOR_HANDLE dsv_cpu_handle = dsv_heap->GetCPUDescriptorHandleForHeapStart();
        
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_cpu_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
                                                                                     current_backbuffer_index, rtv_desc_size);
        
        //D12Present the current framebuffer
        //Commandbuffer
        D12CommandAllocatorEntry* allocator_entry = D12RendererCode::GetFreeCommandAllocatorEntry(D3D12_COMMAND_LIST_TYPE_DIRECT);
        
        D12CommandListEntry command_list = GetAssociatedCommandList(allocator_entry);
        
        //Graphics
        ID3D12Resource* back_buffer = D12RendererCode::GetCurrentBackBuffer();
        bool fc = IsFenceComplete(fence,allocator_entry->fence_value);
        ASSERT(fc);
        allocator_entry->allocator->Reset();
        
        command_list.list->Reset(allocator_entry->allocator, nullptr);
        
        // Clear the render target.
        TransitionResource(command_list,back_buffer,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET);
        
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        
        command_list.list->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        command_list.list->ClearDepthStencilView(dsv_cpu_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        
        //finish up
        EndCommandListEncodingAndExecute(allocator_entry,command_list);
        //insert signal in queue to so we know when we have executed up to this point. 
        //which in this case is up to the command clear and tranition back to present transition 
        //for back buffer.
        allocator_entry->fence_value = D12RendererCode::Signal(command_queue, fence, fence_value);
        
        D12RendererCode::WaitForFenceValue(fence, allocator_entry->fence_value, fence_event);
        
        //D12Rendering
        D12CommandAllocatorEntry* current_ae;
        D12CommandListEntry current_cl;
        
        void* at = render_com_buf.arena.base;
        for(int i = 0;i < render_com_buf.count;++i)
        {
            D12CommandHeader* header = (D12CommandHeader*)at;
            D12CommandType command_type = header->type;
            at = (uint8_t*)at + sizeof(D12CommandHeader);
            /*
            D12CommandType_Draw,
            D12CommandType_Viewport,
            D12CommandType_PipelineState,
            D12CommandType_RootSignature,
            D12CommandType_ScissorRect
            */
            
            if(command_type == D12CommandType_StartCommandList)
            {
                D12CommandStartCommandList* com = (D12CommandStartCommandList*)at;
                at = (uint8_t*)at + (sizeof(D12CommandStartCommandList));                

                //Pop(at,D12CommandStartCommandList);
                current_ae = D12RendererCode::GetFreeCommandAllocatorEntry(D3D12_COMMAND_LIST_TYPE_DIRECT);
                
                current_cl = GetAssociatedCommandList(current_ae);
                bool fcgeo = IsFenceComplete(fence,current_ae->fence_value);
                ASSERT(fcgeo);
                current_ae->allocator->Reset();
                current_cl.list->Reset(current_ae->allocator, nullptr);
                
                current_cl.list->OMSetRenderTargets(1, &rtv_cpu_handle, FALSE, &dsv_cpu_handle);

                continue;
            }

            else if(command_type == D12CommandType_EndCommandList)
            {
                D12CommandEndCommmandList* com = Pop(at,D12CommandEndCommmandList);
                // NOTE(Ray Garner): For now we do this here but we need to do something else  setting render targets.
                
                //End D12 Renderering
                EndCommandListEncodingAndExecute(current_ae,current_cl);
                current_ae->fence_value = D12RendererCode::Signal(command_queue, fence, fence_value);
                // NOTE(Ray Garner): // TODO(Ray Garner): If there are dependencies from the last command list we need to enter a waitforfence value
                //so that we can finish executing this command list before and have the result ready for the next one.
                //If not we dont need to worry about this.
                /*
                //wait for the gpu to execute up until this point before we procede this is the allocators..
                //current fence value which we got when we signaled. 
                //the fence value that we give to each allocator is based on the fence value for the queue.
                D12RendererCode::WaitForFenceValue(fence, ae->fence_value, fence_event);
                */
                
                //wait for the gpu to execute up until this point before we procede this is the allocators..
                //current fence value which we got when we signaled. 
                //the fence value that we give to each allocator is based on the fence value for the queue.
                D12RendererCode::WaitForFenceValue(fence, current_ae->fence_value, fence_event);
                fmj_stretch_buffer_clear(&current_ae->used_list_indexes);
                continue;
            }

            else if(command_type == D12CommandType_Viewport)
            {
                D12CommandViewport* com = Pop(at,D12CommandViewport);
                D3D12_VIEWPORT  new_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,com->viewport.z, com->viewport.w);
                current_cl.list->RSSetViewports(1, &new_viewport);
                continue;
            }

            else if(command_type == D12CommandType_ScissorRect)
            {
                D12CommandScissorRect* com = Pop(at,D12CommandScissorRect);
                //D12RendererCode::sis_rect = CD3DX12_RECT((u64)com->rect.x(), (u64)com->rect.y(), (u64)com->rect.z(), (u64)com->rect.w());
                current_cl.list->RSSetScissorRects(1, &com->rect);
                continue;
            }

            else if(command_type == D12CommandType_RootSignature)
            {
                D12CommandRootSignature* com = Pop(at,D12CommandRootSignature);
                ASSERT(com->root_sig);
                current_cl.list->SetGraphicsRootSignature(com->root_sig);
                continue;
            }
            
            else if(command_type == D12CommandType_PipelineState)
            {
                D12CommandPipelineState* com = Pop(at,D12CommandPipelineState);
                ASSERT(com->pipeline_state);
                current_cl.list->SetPipelineState(com->pipeline_state);
                continue;
            }
            
            else if(command_type == D12CommandType_Draw)
            {
                D12CommandBasicDraw* com = Pop(at,D12CommandBasicDraw);
                current_cl.list->IASetVertexBuffers(0, 1, &com->buffer_view);
                current_cl.list->IASetPrimitiveTopology(com->topology);
                current_cl.list->DrawInstanced(com->count, 1, com->vertex_offset, 0);
                continue;
            }
            
            else if(command_type == D12CommandType_DrawIndexed)
            {
                D12CommandIndexedDraw* com = Pop(at,D12CommandIndexedDraw);
                D3D12_VERTEX_BUFFER_VIEW views[2] = {com->buffer_view,com->uv_view};
                
                //current_cl.list->IASetVertexBuffers(1, 1, &com->uv_view);
                current_cl.list->IASetVertexBuffers(0, 2, views);
                current_cl.list->IASetIndexBuffer(&com->index_buffer_view);
                // NOTE(Ray Garner): // TODO(Ray Garner): Get the heaps
                //that match with the pipeline state and root sig
                current_cl.list->IASetPrimitiveTopology(com->topology);
                current_cl.list->DrawIndexedInstanced(com->index_count,1,0,0,0);
                continue;
            }
            
            else if(command_type == D12CommandType_GraphicsRootDescTable)
            {
                D12CommandGraphicsRootDescTable* com = Pop(at,D12CommandGraphicsRootDescTable);
                
                ID3D12DescriptorHeap* descriptorHeaps[] = { com->heap };
                current_cl.list->SetDescriptorHeaps(1, descriptorHeaps);
                current_cl.list->SetGraphicsRootDescriptorTable(com->index, com->gpu_handle);
                continue;
            }
            
            else if(command_type == D12CommandType_GraphicsRootConstant)
            {
                D12CommandGraphicsRoot32BitConstant* com = Pop(at,D12CommandGraphicsRoot32BitConstant);
                current_cl.list->SetGraphicsRoot32BitConstants(com->index, com->num_values, com->gpuptr, com->offset);
                continue;
            }
        }
        
        fmj_arena_deallocate(&render_com_buf.arena,false);
        render_com_buf.count = 0;
#if 0
        D12CommandAllocatorEntry* ae = D12RendererCode::GetFreeCommandAllocatorEntry(D3D12_COMMAND_LIST_TYPE_DIRECT);
        
        D12CommandListEntry cl = GetAssociatedCommandList(ae);
        bool fcgeo = IsFenceComplete(fence,ae->fence_value);
        ASSERT(fcgeo);
        ae->allocator->Reset();
        cl.list->Reset(ae->allocator, nullptr);
        
        //Render
        cl.list->OMSetRenderTargets(1, &rtv_cpu_handle, FALSE, &dsv_cpu_handle);
        
        cl.list->SetPipelineState(pipeline_state);
        cl.list->SetGraphicsRootSignature(root_sig);
        
        cl.list->RSSetViewports(1, &viewport);
        cl.list->RSSetScissorRects(1, &sis_rect);
        
        // set the descriptor heap
        ID3D12DescriptorHeap* descriptorHeaps[] = { draw_test->heap };
        cl.list->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
        
        // set the descriptor table to the descriptor heap (parameter 1, as constant buffer root descriptor is parameter index 0)
        cl.list->SetGraphicsRootDescriptorTable(1, draw_test->heap->GetGPUDescriptorHandleForHeapStart());
        
        f4x4 finalmat = f4x4::identity();
        f4x4 m_mat = draw_test->ot.m;
        f4x4 c_mat = draw_test->rc.matrix;
        f4x4 p_mat = draw_test->rc.projection_matrix;
        finalmat = mul(c_mat,m_mat);
        finalmat = mul(p_mat,finalmat);
        
        cl.list->SetGraphicsRoot32BitConstants(0, 16, &finalmat, 0);
        cl.list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cl.list->IASetVertexBuffers(0, 1, &draw_test->v_buffer.buffer_view);
        cl.list->DrawInstanced(6, 1, 0, 0);
        
        //End D12 Renderering
        EndCommandListEncodingAndExecute(ae,cl);
        ae->fence_value = D12RendererCode::Signal(command_queue, fence, fence_value);
        
        //wait for the gpu to execute up until this point before we procede this is the allocators..
        //current fence value which we got when we signaled. 
        //the fence value that we give to each allocator is based on the fence value for the queue.
        D12RendererCode::WaitForFenceValue(fence, ae->fence_value, fence_event);
        YoyoClearVector(&ae->used_list_indexes);
#endif
        D12CommandAllocatorEntry* final_allocator_entry = D12RendererCode::GetFreeCommandAllocatorEntry(D3D12_COMMAND_LIST_TYPE_DIRECT);
        
        D12CommandListEntry final_command_list = GetAssociatedCommandList(final_allocator_entry);
        
        bool final_fc = IsFenceComplete(fence,final_allocator_entry->fence_value);
        ASSERT(final_fc);
        final_allocator_entry->allocator->Reset();
        
        final_command_list.list->Reset(final_allocator_entry->allocator, nullptr);
        ID3D12Resource* cbb = D12RendererCode::GetCurrentBackBuffer();
        
        //tranistion the render target back to present mode. preparing for presentation.
        TransitionResource(final_command_list,cbb,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT);
        
        //finish up
        EndCommandListEncodingAndExecute(final_allocator_entry,final_command_list);
        //insert signal in queue to so we know when we have executed up to this point. 
        //which in this case is up to the command clear and tranition back to present transition 
        //for back buffer.
        final_allocator_entry->fence_value = D12RendererCode::Signal(command_queue, fence, fence_value);
        D12RendererCode::WaitForFenceValue(fence, final_allocator_entry->fence_value, fence_event);
        
        //execute the present flip
        UINT sync_interval = 0;
        UINT present_flags = DXGI_PRESENT_ALLOW_TEARING;
        D12RendererCode::swap_chain->Present(sync_interval, present_flags);
        
        //wait for the gpu to execute up until this point before we procede this is the allocators..
        //current fence value which we got when we signaled. 
        //the fence value that we give to each allocator is based on the fence value for the queue.
        //D12RendererCode::WaitForFenceValue(fence, allocator_entry->fence_value, fence_event);
        fmj_stretch_buffer_clear(&allocator_entry->used_list_indexes);

        is_resource_cl_recording = false;
        // NOTE(Ray Garner): Here we are doing bookkeeping for resuse of various resources.
        //If the allocators are not in flight add them to the free table
        D12RendererCode::CheckReuseCommandAllocators();
        fmj_thread_end_ticket_mutex(&upload_operations.ticket_mutex);
        
        //Reset state of constant buffer
        fmj_arena_deallocate(&constants_arena,false);
    }
};
