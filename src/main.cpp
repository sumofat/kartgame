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
    

    
    while(ps->is_running)
    {
        PullDigitalButtons(ps);
        PullMouseState(ps);
        PullGamePads(ps);

        if(ps->input.keyboard.keys[keys.s].down)
        {
            ps->is_running = false;
        }
        HandleWindowsMessages(ps);        
    }
    return 0;
}
