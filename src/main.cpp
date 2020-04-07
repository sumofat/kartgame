#include<windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <xinput.h>

extern "C"{
#include "fmj/src/fmj_types.h"
}

#include "directx12.h"
#include "util.h"

enum RenderCameraProjectionType
{
    perspective,
    orthographic,
    screen_space
};

struct RenderCamera
{
    FMJ3DTrans ot;//perspective and ortho only
    f4x4 matrix;
    f4x4 projection_matrix;
    f4x4 spot_light_shadow_projection_matrix;
    f4x4 point_light_shadow_projection_matrix;
    RenderCameraProjectionType projection_type;
    f32 size;//ortho only
    f32 fov;//perspective only
    f2 near_far_planes;
};

#define MAX_KEYS 256
#define MAX_CONTROLLER_SUPPORT 2
struct Keys
{
    u32 w;
	u32 e;
	u32 r;
    u32 a;
    u32 s;
    u32 d;
    u32 f;
    u32 i;
    u32 j;
    u32 k;
    u32 l;
    u32 f1;
    u32 f2;
    u32 f3;
};

Keys keys;

struct Time
{
    u64 frame_index;
    f32 delta_seconds;
    u64 delta_ticks;
    u64 delta_nanoseconds;
    u64 delta_microseconds;
    u64 delta_miliseconds;

    u64 delta_samples;
    
    f64 time_seconds;
    u64 time_ticks;
    u64 time_nanoseconds;
    u64 time_microseconds;
    u64 time_miliseconds;

    u64 ticks_per_second;
    u64 initial_ticks;

    u64 prev_ticks;
};

struct DigitalButton
{
    bool down;
    bool pressed;
    bool released;
};

struct Keyboard
{
    DigitalButton keys[MAX_KEYS];    
};

struct AnalogButton
{
    f32 threshold;
    f32 value;
    bool down;
    bool pressed;
    bool released;
};

struct Axis
{
    f32 value;
    f32 threshold;
};

//TODO(Ray):Handle other types of axis like ruddder/throttle etc...
struct Stick
{
    Axis X;
    Axis Y;
};

struct GamePad
{
#if WINDOWS
    XINPUT_STATE state;    
#endif
    
    Stick left_stick;
    Stick right_stick;
    
    Axis left_shoulder;
    Axis right_shoulder;

    DigitalButton up;
    DigitalButton down;
    DigitalButton left;
    DigitalButton right;

    DigitalButton a;
    DigitalButton b;
    DigitalButton x;
    DigitalButton y;

    DigitalButton l;
    DigitalButton r;

    DigitalButton select;
    DigitalButton start;    
};

struct Mouse
{
    f2 p;
    f2 prev_p;
    f2 delta_p;
    f2 uv;
    f2 prev_uv;
    f2 delta_uv;

    DigitalButton lmb;//left_mouse_button
    DigitalButton rmb;
	bool wrap_mode;
};

struct Input
{
    Keyboard keyboard;
    Mouse mouse;
    GamePad game_pads[MAX_CONTROLLER_SUPPORT];
};

struct SystemInfo
{
#if WINDOWS
    SYSTEM_INFO system_info;
#endif
    int a;
} typedef SystemInfo;

struct Window
{
#if WINDOWS
    WNDCLASS w_class;
    HWND handle;
    HDC device_context;
    WINDOWPLACEMENT global_window_p;
#endif
	f2 dim;
    f2 p;
    bool is_full_screen_mode;
} typedef Window;

struct PlatformState
{
    Time time;
    Input input;
//    Renderer renderer;
//    Audio audio;
    Window window;
//    Memory memory;
    SystemInfo info;
    bool is_running;
} typedef PlatformState;

struct GlobalMemory
{
    PlatformState ps;  
} typedef GlobalMemory;

GlobalMemory gm;

//Functions

f2 GetWin32WindowDim(PlatformState* ps)
{
    RECT client_rect;
    GetClientRect(ps->window.handle, &client_rect);
	return f2_create(client_rect.right - client_rect.left, client_rect.bottom - client_rect.top);
}

void UpdateDigitalButton(DigitalButton* button,u32 state)
{
    bool was_down = button->down;
    bool down = state >> 7;
    button->pressed = !was_down && down;
    button->released = was_down && !down;
    button->down = down;    
}

void PullMouseState(PlatformState* ps)
{
    Input* input = &ps->input;
    if(input)
    {
         POINT MouseP;
         GetCursorPos(&MouseP);
         ScreenToClient(ps->window.handle, &MouseP);
         //TODO(Ray):Account for non full screen mode header 
         f2 window_dim = GetWin32WindowDim(ps);
		 f2 current_mouse_p = f2_create(MouseP.x, (window_dim.y) - MouseP.y);
         f2 delta_mouse_p = f2_sub(input->mouse.prev_p,current_mouse_p);
		if(ps->input.mouse.wrap_mode)
		{
			if (MouseP.x > window_dim.x - 1)
			{
				POINT new_p;
				new_p.x = 1;
				new_p.y = MouseP.y;
				if (ClientToScreen(ps->window.handle, &new_p))
				{
					SetCursorPos(new_p.x, new_p.y);
					ScreenToClient(ps->window.handle, &new_p);
					current_mouse_p.x = 1;
					delta_mouse_p.x = 1;
				}
			}
			if (MouseP.y > window_dim.y - 1)
			{
				POINT new_p;
				new_p.x = current_mouse_p.x;
				new_p.y = 1;
				if (ClientToScreen(ps->window.handle, &new_p))
				{
					SetCursorPos(new_p.x, new_p.y);
					ScreenToClient(ps->window.handle, &new_p);
					current_mouse_p.y = ((window_dim.y) - new_p.y);
					delta_mouse_p.y = -1;
				}
			}
			if (MouseP.x < 1)
			{
				POINT new_p;
				new_p.x = window_dim.x - 1;
				new_p.y = MouseP.y;
				if (ClientToScreen(ps->window.handle, &new_p))
				{
					SetCursorPos(new_p.x, new_p.y);
					ScreenToClient(ps->window.handle, &new_p);
					current_mouse_p.x = window_dim.x - 1;
					delta_mouse_p.x = -1;
				}
			}
			if (MouseP.y < 1)
			{
				POINT new_p;
				new_p.x = (int)current_mouse_p.x;
				new_p.y = window_dim.y - 1;
				if (ClientToScreen(ps->window.handle, &new_p))
				{
					SetCursorPos(new_p.x, new_p.y);
					ScreenToClient(ps->window.handle, &new_p);
					current_mouse_p.y = window_dim.y - new_p.y;
					delta_mouse_p.y = -1;
				}
			}
		}
        input->mouse.p = current_mouse_p;
        input->mouse.delta_p = delta_mouse_p;
        input->mouse.prev_uv = input->mouse.uv;
        input->mouse.uv = f2_create(input->mouse.p.x / ps->window.dim.x, input->mouse.p.y / ps->window.dim.y);
        input->mouse.delta_uv = f2_sub(input->mouse.prev_uv,input->mouse.uv);
        input->mouse.prev_p = input->mouse.p;

        u32 lmbstate = GetAsyncKeyState(VK_LBUTTON);
        UpdateDigitalButton(&input->mouse.lmb,lmbstate);
        u32 rmbstate = GetAsyncKeyState(VK_RBUTTON);
        UpdateDigitalButton(&input->mouse.rmb,rmbstate);         
    }
}

void PullDigitalButtons(PlatformState* ps)
{
    Input* input = &ps->input;
    BYTE keyboard_state[MAX_KEYS];
    if(GetKeyboardState(keyboard_state))
    {
        for(int i = 0;i < MAX_KEYS;++i)
        {
            DigitalButton* button = &input->keyboard.keys[i];
            bool was_down = button->down;
            bool down = keyboard_state[i] >> 7;
            button->pressed = !was_down && down;
            button->released = was_down && !down;
            button->down = down;
        }
    }
}

void SetButton(DigitalButton* button,u32 button_type,XINPUT_STATE state)
{
    bool was_down = button->down;
    bool down = ((state.Gamepad.wButtons & button_type) != 0);
    button->pressed = !was_down && down;
    button->released = was_down && !down;
    button->down = down;
}

void PullGamePads(PlatformState* ps)
{
    for (DWORD i = 0; i < MAX_CONTROLLER_SUPPORT; i++)
    {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));

        GamePad* game_pad = &ps->input.game_pads[i];
        if (XInputGetState(i, &state) == ERROR_SUCCESS)
        {
            game_pad[i].state = state;
            SetButton(&game_pad[i].a,XINPUT_GAMEPAD_A,state);
            SetButton(&game_pad[i].b,XINPUT_GAMEPAD_B,state);
            SetButton(&game_pad[i].x,XINPUT_GAMEPAD_X,state);
            SetButton(&game_pad[i].y,XINPUT_GAMEPAD_Y,state);
            SetButton(&game_pad[i].l,XINPUT_GAMEPAD_LEFT_SHOULDER,state);
            SetButton(&game_pad[i].r,XINPUT_GAMEPAD_RIGHT_SHOULDER,state);
            SetButton(&game_pad[i].select,XINPUT_GAMEPAD_BACK,state);
            SetButton(&game_pad[i].start,XINPUT_GAMEPAD_START,state);

            game_pad[i].left_shoulder.value = (f32)state.Gamepad.bLeftTrigger / 255;
            game_pad[i].right_shoulder.value = (f32) state.Gamepad.bRightTrigger / 255;

            game_pad[i].left_stick.X.value = clamp((float)state.Gamepad.sThumbLX / 32767 ,-1,1);
            game_pad[i].left_stick.Y.value = clamp((float)state.Gamepad.sThumbLY / 32767 ,-1,1);

            game_pad[i].right_stick.X.value = clamp((float)state.Gamepad.sThumbRX / 32767 ,-1,1);
            game_pad[i].right_stick.Y.value = clamp((float)state.Gamepad.sThumbRY / 32767 ,-1,1);
        }
    } 
}

void WINSetScreenMode(PlatformState* ps,bool is_full_screen)
{
    HWND Window = ps->window.handle;
    LONG Style = GetWindowLong(Window, GWL_STYLE);
    ps->window.is_full_screen_mode = is_full_screen;
    if(ps->window.is_full_screen_mode)
    {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(Window, &ps->window.global_window_p) &&
            GetMonitorInfo(MonitorFromWindow(Window,
                                             MONITOR_DEFAULTTOPRIMARY), &mi))
        {
            SetWindowLong(Window, GWL_STYLE,Style& ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            ps->window.is_full_screen_mode = true;
			ps->window.dim = f2_create(mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top);
        }
    }
    else
    {
        ps->window.is_full_screen_mode = false;
        SetWindowLong(Window, GWL_STYLE,Style& ~WS_OVERLAPPEDWINDOW);
        //SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &ps->window.global_window_p);
        SetWindowPos(Window,
nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

LRESULT CALLBACK MainWindowCallbackFunc(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{

/*
#if IMGUI
    if (ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam))
        return true;
#endif
*/
    
    LRESULT Result = 0;
    switch(Message)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hDc = BeginPaint(Window, &ps);
//            FillRect(hDc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW + 1));
            EndPaint(Window, &ps);
            return 0;
        }break;
//NOTE(Ray): We do not allow resizing at the moment.
//        case WM_SIZE:
//        {
//            if (dev != NULL && WParam != SIZE_MINIMIZED)
//            {
//            ImGui_ImplDX11_InvalidateDeviceObjects();
//            CleanupRenderTarget();
//            swapchain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
//            CreateRenderTarget();
//            ImGui_ImplDX11_CreateDeviceObjects();
//            }
//            return 0;
//        }
        case WM_CLOSE:
        case WM_DESTROY:
        {
            gm.ps.is_running = false;
        } break;
        case WM_ACTIVATEAPP:
        {
            
        } break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            //u32 VKCode = (u32)Message.wParam;
            
        }break;
        default:
        {
//            OutputDebugStringA("default\n");
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    return Result;
}

void HandleWindowsMessages(PlatformState* ps)
{
    MSG message;
    while(PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
    {
        if(message.message == WM_QUIT)
        {
            ps->is_running = false;
        }
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
}

int WINAPI WinMain(HINSTANCE h_instance,HINSTANCE h_prev_instance, LPSTR lp_cmd_line, int n_show_cmd )
{
    printf("Starting Window...");
    PlatformState* ps = &gm.ps;
    ps->is_running  = true;
//    GetSystemInfo(&platform_state->info.system_info);
    
    f2 dim = f2_create(1024,1024);
    f2 p = f2_create(0,0);

    Window *window = &ps->window;
	window->dim = dim;
    window->p = p;
    //HINSTANCE HInstance = GetModuleHandle(NULL);
    WNDCLASS WindowClass =  {};

    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = MainWindowCallbackFunc;
    WindowClass.hInstance = h_instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    WindowClass.lpszClassName = "Chip8Class";    
    
    DWORD ErrorCode = GetLastError();
    //ImGuiIO& io;
    if(RegisterClass(&WindowClass))
    {
        HWND created_window =
            CreateWindowExA(
                0,//WS_EX_TOPMOST|WS_EX_LAYERED,
                WindowClass.lpszClassName,
                "Chip8",
                WS_OVERLAPPEDWINDOW,
                window->p.x,
                window->p.y,
                window->dim.x,
                window->dim.y,
                0,
                0,
                h_instance,
                0);
        ErrorCode = GetLastError();
        if(created_window)
        {
            ShowWindow(created_window, n_show_cmd);
            window->handle = created_window;

        }
        else
        {
            //TODO(Ray):Could not attain window gracefully let the user know and shut down.
            //Could not start window.
            MessageBox(NULL, "Window Creation Failed!", "Error!",
                        MB_ICONEXCLAMATION | MB_OK);
             return 0;
        }
    }

//time
    LARGE_INTEGER li;
    QueryPerformanceFrequency((LARGE_INTEGER*)&li);
    ps->time.ticks_per_second = li.QuadPart;
    QueryPerformanceCounter((LARGE_INTEGER*)&li);
    ps->time.initial_ticks = li.QuadPart;
    ps->time.prev_ticks = ps->time.initial_ticks;

//keys
    //TODO(Ray):Propery check for layouts
    HKL layout =  LoadKeyboardLayout((LPCSTR)"00000409",0x00000001);
    SHORT  code = VkKeyScanEx('s',layout);
    keys.s = code;
    code = VkKeyScanEx('w',layout);
    keys.w = code;
    code = VkKeyScanEx('a',layout);
	keys.a = code;
	code = VkKeyScanEx('e', layout);
	keys.e = code;
	code = VkKeyScanEx('r', layout);
    keys.r = code;
    code = VkKeyScanEx('d',layout);
    keys.d = code;
    code = VkKeyScanEx('f',layout);
    keys.f = code;
    code = VkKeyScanEx('i',layout);
    keys.i = code;
    code = VkKeyScanEx('j',layout);
    keys.j = code;
    code = VkKeyScanEx('k',layout);
    keys.k = code;
    code = VkKeyScanEx('l',layout);
    keys.l = code;
    keys.f1 = VK_F1;
    keys.f2 = VK_F2;
	keys.f3 = VK_F3;

    //Lets init directx12
    CreateDeviceResult result = D12RendererCode::Init(&ps->window.handle,ps->window.dim);
    if (result.is_init)
    {
        //Do some setup.
        //device = result.device;
    }
    else
    {
        MessageBoxA(NULL, "Could not initialize your graphics device!", "Error!",
                    MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    //Set up file api options.
    ID3D12Device2* device = D12RendererCode::GetDevice();
        
    //Load a texture with all mip maps calculated
    //load texture to mem from disk
    LoadedTexture test_texture = get_loaded_image("test.png",4);
    LoadedTexture test_texture_2 = get_loaded_image("test2.png",4);    

     //upload texture data to gpu
    D12RendererCode::Texture2D(&test_texture);
    D12RendererCode::Texture2D(&test_texture_2);
    D12RendererCode::viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,ps->window.dim.x, ps->window.dim.y);
    D12RendererCode::sis_rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

     // Create the descriptor heap for the depth-stencil view.
    D3D12_DESCRIPTOR_HEAP_DESC dsv_h_d = {};
    dsv_h_d.NumDescriptors = 1;
    dsv_h_d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_h_d.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HRESULT r = device->CreateDescriptorHeap(&dsv_h_d, IID_PPV_ARGS(&D12RendererCode::dsv_heap));
    ASSERT(SUCCEEDED(r));

    char* vs_file_name = "vs_test.hlsl";
    char* fs_file_name = "ps_test.hlsl";
    
    FMJFileReadResult vs_result = fmj_file_platform_read_entire_file(vs_file_name);
    ASSERT(vs_result.content_size > 0);
    
    FMJFileReadResult fs_result = fmj_file_platform_read_entire_file(fs_file_name);
    ASSERT(fs_result.content_size > 0);

    ID3DBlob* vs_blob;
    ID3DBlob* vs_blob_errors;
    
    ID3DBlob* fs_blob;
    ID3DBlob* fs_blob_errors;
    
    r = D3DCompile2(
        vs_result.content,
        vs_result.content_size,
        vs_file_name,
        0,
        0,
        "main",
        "vs_5_1",
        SHADER_DEBUG_FLAGS,
        0,
        0,
        0,
        0,
        &vs_blob,
        &vs_blob_errors);
    if ( vs_blob_errors )
    {
        OutputDebugStringA( (const char*)vs_blob_errors->GetBufferPointer());
    }
    
    ASSERT(SUCCEEDED(r));
    
    r = D3DCompile2(
        fs_result.content,
        fs_result.content_size,
        fs_file_name,
        0,
        0,
        "main",
        "ps_5_1",
        SHADER_DEBUG_FLAGS,
        0,
        0,
        0,
        0,
        &fs_blob,
        &fs_blob_errors);
    if ( fs_blob_errors )
    {
        OutputDebugStringA( (char*)fs_blob_errors->GetBufferPointer() );
    }

    ASSERT(SUCCEEDED(r));

    D3D12_INPUT_ELEMENT_DESC input_layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        //{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    
    uint32_t input_layout_count = _countof(input_layout);
    // Create a root/shader signature.
    
    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
    {
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    //|D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    // create a descriptor range (descriptor table) and fill it out
    // this is a range of descriptors inside a descriptor heap
    D3D12_DESCRIPTOR_RANGE1  descriptorTableRanges[1];
    descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorTableRanges[0].NumDescriptors = 2; 
    descriptorTableRanges[0].BaseShaderRegister = 0; 
    descriptorTableRanges[0].RegisterSpace = 0;
    descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; 
    descriptorTableRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

    // create a descriptor table
    D3D12_ROOT_DESCRIPTOR_TABLE1 descriptorTable;
    descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges);
    descriptorTable.pDescriptorRanges = &descriptorTableRanges[0];

    D3D12_ROOT_CONSTANTS rc_1 = {};
    rc_1.RegisterSpace = 0;
    rc_1.ShaderRegister = 0;
    rc_1.Num32BitValues = 16;
    
    D3D12_ROOT_CONSTANTS rc_2 = {};
    rc_2.RegisterSpace = 0;
    rc_2.ShaderRegister = 1;
    rc_2.Num32BitValues = 16;
    
    D3D12_ROOT_CONSTANTS rc_3 = {};
    rc_3.RegisterSpace = 0;
    rc_3.ShaderRegister = 2;
    rc_3.Num32BitValues = 4;

    D3D12_ROOT_PARAMETER1  root_params[4];
    root_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    root_params[0].Constants = rc_1;

    // fill out the parameter for our descriptor table. Remember it's a good idea to sort parameters by frequency of change. Our constant
    // buffer will be changed multiple times per frame, while our descriptor table will not be changed at all.
    root_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_params[1].DescriptorTable = descriptorTable;
    root_params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    
    root_params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    root_params[2].Constants = rc_2;
    
    root_params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_params[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    root_params[3].Constants = rc_3;

    D3D12_STATIC_SAMPLER_DESC vs;
    vs.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    vs.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    vs.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    vs.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    vs.MipLODBias = 0.0f;
    vs.MaxAnisotropy = 1;
    vs.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    vs.MinLOD = 0;
    vs.MaxLOD = D3D12_FLOAT32_MAX;
    vs.ShaderRegister = 1;
    vs.RegisterSpace = 0;
    vs.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_STATIC_SAMPLER_DESC tex_static_samplers[2];
    tex_static_samplers[0] = vs;

    D3D12_STATIC_SAMPLER_DESC ss;
    ss.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    ss.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    ss.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    ss.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    ss.MipLODBias = 0.0f;
    ss.MaxAnisotropy = 1;
    ss.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    ss.MinLOD = 0;
    ss.MaxLOD = D3D12_FLOAT32_MAX;
    ss.ShaderRegister = 0;
    ss.RegisterSpace = 0;
    ss.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
    tex_static_samplers[1] = ss;

    D12RendererCode::root_sig = D12RendererCode::CreatRootSignature(root_params,_countof(root_params),tex_static_samplers,2,root_sig_flags);

    D12RendererCode::pipeline_state = D12RendererCode::CreatePipelineState(input_layout,input_layout_count,vs_blob,fs_blob);    

    //Create the depth buffer
    // TODO(Ray Garner): FLUSH
    uint32_t width =  max(1, ps->window.dim.x);
    uint32_t height = max(1, ps->window.dim.y);
    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = { 1.0f, 0 };
    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
                                      1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimizedClearValue,
        IID_PPV_ARGS(&D12RendererCode::depth_buffer)
        );
    
    // Update the depth-stencil view.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    dsv.Flags = D3D12_DSV_FLAG_NONE;
    device->CreateDepthStencilView(D12RendererCode::depth_buffer, &dsv,
                                   D12RendererCode::dsv_heap->GetCPUDescriptorHandleForHeapStart());

    //Descriptor heap for our resources
    // create the descriptor heap that will store our srv
    ID3D12DescriptorHeap* test_desc_heap = D12RendererCode::CreateDescriptorHeap(2,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    RenderCamera rc = {};
    rc.ot.p = f3_create(0,0,0);
    rc.ot.r = f3_axis_angle(f3_create(0,0,1),0);
    //rc.projection_matrix = init_ortho_proj_matrix(f2_create(100.0f,100.0f),0.0f,1.0f);
    rc.projection_matrix = init_pers_proj_matrix(ps->window.dim,80,f2_create(0.1f,100000));
    
    rc.fov = 80;
    rc.near_far_planes = f2_create(0,1);
    
    rc.matrix = f4x4_identity();

    FMJ3DTrans cube = {0};
    //cube.p = float3(-hmlt.dim.x() * 0.5f,-255,-hmlt.dim.y()*0.5f);
    cube.p = f3_create(0,0,5);
    cube.s = f3_create(10.0f,10.0f,10.0f);
    cube.r = f3_axis_angle(f3_create(0,0,1),0);    
    //float3 ter_r_axis = float3(1,0,0);
    //angle = 90;
    //cube.r = axis_angle(ter_r_axis,angle);
    fmj_3dtrans_update(&cube);    

    u64 quad_mem_size = SIZE_OF_SPRITE_IN_BYTES;
    
    GPUArena quad_gpu_arena = D12RendererCode::AllocateGPUArena(quad_mem_size);

    FMJSpriteBatch sb = {0};
    sb.arena = fmj_arena_allocate(quad_mem_size);
    uint64_t stride = sizeof(float) * (3 + 4 + 2);

    f3 bl = f3_create(0,0,0);
    f3 br = f3_create(1,0,0);
    f3 tr = f3_create(1,1,0);
    f3 tl = f3_create(0,1,0);

    f4 white = f4_create(1,1,1,1);

    f2 st = f2_create(0,0);
    
    f3 poss[4] = {bl,br,tr,tl};
    f4 cs[4] = {white,white,white,white};
    f2 uvs[4] = {st,st,st,st};
    fmj_sprite_add_rect(&sb.arena,poss,cs,uvs);
    
    //Set data
    //...
    D12RendererCode::SetArenaToVertexBufferView(&quad_gpu_arena,quad_mem_size,stride);
    D12RendererCode::UploadBufferData(&quad_gpu_arena,sb.arena.base,quad_mem_size);
       
#if 1
    quad_gpu_arena.resource->SetName(L"QUADS_GPU_ARENA");
#endif
    f2 cam_pitch_yaw = f2_create(0.0f,0.0f);    
    while(ps->is_running)
    {
//Get input
        PullDigitalButtons(ps);
        PullMouseState(ps);
        PullGamePads(ps);

//Game stuff happens 
        if(ps->input.keyboard.keys[keys.s].down)
        {
            ps->is_running = false;
        }
//        cube.p.x += 0.001f;
//Render camera stated etc..  is finalized        
 //Free cam
#if 1
        cam_pitch_yaw = f2_add(cam_pitch_yaw,ps->input.mouse.delta_p);
        quaternion pitch = f3_axis_angle(f3_create(1, 0, 0), cam_pitch_yaw.y);
        quaternion yaw   = f3_axis_angle(f3_create(0, 1, 0), cam_pitch_yaw.x * -1);
//        quaternion turn_qt = quaternion_mul(yaw,pitch);
        quaternion turn_qt = quaternion_mul(pitch,yaw);        
        rc.ot.r = turn_qt;
#endif
        rc.matrix = fmj_3dtrans_set_cam_view(&rc.ot);

//Render commands are issued to the api
        //Render API CODE
        D12RendererCode::AddStartCommandListCommand();
        
        D12RendererCode::AddPipelineStateCommand(D12RendererCode::pipeline_state);
        D12RendererCode::AddRootSignatureCommand(D12RendererCode::root_sig);
        D12RendererCode::AddViewportCommand(f4_create(0,0,ps->window.dim.x,ps->window.dim.y));
        D12RendererCode::AddScissorRectCommand(CD3DX12_RECT(0,0,LONG_MAX,LONG_MAX));
        D12RendererCode::AddGraphicsRootDescTable(1,D12RendererCode::default_srv_desc_heap,D12RendererCode::default_srv_desc_heap->GetGPUDescriptorHandleForHeapStart());

        fmj_3dtrans_update(&cube);    
        FMJ3DTrans t = cube;
        f4x4 m_mat = t.m;
        f4x4 c_mat = rc.matrix;
        f4x4 p_mat = rc.projection_matrix;
        f4x4 world_mat = f4x4_mul(c_mat,m_mat);
        f4x4 finalmat = f4x4_mul(p_mat,world_mat);
//        D12RendererCode::AddGraphicsRoot32BitConstant(0,16,&finalmat,0);
        D12RendererCode::AddGraphicsRoot32BitConstant(0,16,&m_mat,0);        
        D12RendererCode::AddGraphicsRoot32BitConstant(2,16,&finalmat,0);

        f4 clip_map_data = f4_create(0,(float)1,(float)1,(float)3);
        D12RendererCode::AddGraphicsRoot32BitConstant(3,4,&clip_map_data,0);
                    
//        D12RendererCode::AddDrawIndexedCommand(tile_index_count * 2,1,test_desc_heap,D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
//                                               tile_uv_gpu_arena.buffer_view,
//                                               tile_gpu_arena.buffer_view,tile_index_gpu_arena.index_buffer_view);
        D12RendererCode::AddDrawCommand(6,1,test_desc_heap,D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,quad_gpu_arena.buffer_view);

//Command are finalized and rendering is started.        
 // TODO(Ray Garner): Add render targets commmand
        D12RendererCode::AddEndCommandListCommand();

//        DrawTest test = {cube,rc,gpu_arena,test_desc_heap};
        DrawTest test = {};
//Post frame work is done.
        D12RendererCode::EndFrame(&test);
        
//Handle windows message and do it again.
        HandleWindowsMessages(ps);        
    }
    return 0;
}
