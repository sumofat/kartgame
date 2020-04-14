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

#include "sound_interface.h"
#include "sound_interface.cpp"

#include "physics.h"
#include "physics.cpp"

static SoundClip bgm_soundclip;
PhysicsScene scene;
PhysicsMaterial material;
f32 ground_friction = 0.991f;

struct Koma
{
    u64 inner_id;
    u64 outer_id;
//    f3 velocity;
    RigidBodyDynamic rigid_body;    
}typedef Koma;


bool prev_lmb_state = false;
struct FingerPull
{
    f2 start_pull_p;
    f2 end_pull_p;
    float pull_strength;
    bool pull_begin = false;
    Koma* koma;
};

static FingerPull finger_pull = {};

bool CheckValidFingerPull(FingerPull* fp)
{
    ASSERT(fp);
    fp->pull_strength = abs(f2_length(f2_sub(fp->start_pull_p,fp->end_pull_p)));
    if(fp->pull_strength > 2)
    {
        return true;
    }
    fp->pull_strength = 0;
    return false;
}

class GamePiecePhysicsCallback : public PxSimulationEventCallback
{
	void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override
	{
		int a = 0;
	}
	void onWake(PxActor** actors, PxU32 count) override
	{
		int a = 0;
	}
	void onSleep(PxActor** actors, PxU32 count) override
	{
		int a = 0;
	}
	void onTrigger(PxTriggerPair* pairs, PxU32 count) override
	{
		int a = 0;
	}
	void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override
	{
		int a = 0;
	}

	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override
	{
#if 0
		for (PxU32 i = 0; i < nbPairs; i++)
		{
			const PxContactPair& cp = pairs[i];
			if ((pairHeader.actors[0] == rigid_body) ||
				(pairHeader.actors[1] == rigid_body))
			{
				if (cp.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
				{
					PlatformOutput(true, "Touch is persisting\n");
				}
				if (cp.events & PxPairFlag::eNOTIFY_TOUCH_LOST)
				{
					PlatformOutput(true, "Touch is Lost\n");
				}
				if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
				{
					PlatformOutput(true, "Touch is Found\n");
				}
			}
		}
#endif
        //PlatformOutput(true, "OnContact");
	}
};


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


struct SpriteTrans
{
    u64 id;
    u64  sprite_id;
    u64 material_id;
    u64 model_matrix_id;
    FMJ3DTrans t;
}typedef SpriteTrans;

u64 sprite_trans_create(f3 p,f3 s,quaternion r,f4 color,u64 sprite_id,FMJStretchBuffer* matrix_buffer,FMJFixedBuffer* buffer,FMJMemoryArena* sb_arena)
{
    f2 stbl = f2_create(0.0f,0.0f);
    f2 stbr = f2_create(1.0f,0.0f);
    f2 sttr = f2_create(1.0f,1.0f);
    f2 sttl = f2_create(0.0f,1.0f);
    f2 uvs[4] = {stbl,stbr,sttr,sttl};
    
    SpriteTrans result = {0};
    FMJ3DTrans t = {0};
    t.p = p;
    t.s = s;
    t.r = r;
    fmj_3dtrans_update(&t);
    result.t = t;
    result.sprite_id = sprite_id;
    result.model_matrix_id = fmj_stretch_buffer_push(matrix_buffer,&result.t.m);
    u64 id = fmj_fixed_buffer_push(buffer,(void*)&result);
    //NOTE(Ray):PRS is not used here  just the matrix that is passed above.
    fmj_sprite_add_quad_notrans(sb_arena,p,r,s,color,uvs);
    return id;
}

struct AssetTables
{
    AnyCache materials;
    AnyCache geometries;
    FMJStretchBuffer sprites;
    FMJStretchBuffer vertex_buffers;
}typedef;

FMJSprite add_sprite_to_stretch_buffer(FMJStretchBuffer* sprites,u64 material_id,u64 tex_id,f2 uvs[],f4 color,bool is_visible)
{
    FMJSprite result = {0};
    result = fmj_sprite_init(tex_id,uvs,color,is_visible);
    result.material_id = material_id;
    u64 sprite_id = fmj_stretch_buffer_push(sprites,(void*)&result);
    result.id = sprite_id;
    return result;    
}

AssetTables asset_tables = {0};
u64 material_count = 0;
//draw nodes
void fmj_ui_commit_nodes_for_drawing(FMJMemoryArena* arena,FMJUINode base_node,FMJFixedBuffer* quad_buffer,f4 color,f2 uvs[])
{
    for(int i = 0;i < base_node.children.fixed.count;++i)
    {
        FMJUINode* child_node = fmj_stretch_buffer_check_out(FMJUINode,&base_node.children,i);
        if(child_node)
        {

//            f2 d = ps->window.dim;
//            f2 hd = f2_div_s(d,2.0f);

//            transform.p = f3_create(hd.x,hd.y,0);
//            transform.s = f3_create(ping_title.dim.x,ping_title.dim.y,1.0f);
//            transform.s = f3_create(ps->window.dim.x,ps->window.dim.y,1.0f);            
//            transform.r = f3_axis_angle(f3_create(0,0,1),0);
//            fmj_3dtrans_update(&transform);
        
//            st.t = transform;
//            st.s = fmj_sprite_init(3,uvs,white,true);
            if(child_node->type == fmj_ui_node_sprite)
            {
                SpriteTrans st= {0};
                st.sprite_id = child_node->sprite_id;
                fmj_fixed_buffer_push(quad_buffer,(void*)&st);
//fmj_sprite_add_quad_notrans(&sb.arena,transform.p,transform.r,transform.s,white,uvs);                    
                fmj_sprite_add_rect_with_dim(arena,child_node->rect.dim,0,child_node->rect.color,uvs);
            }
            else
            {
                ASSERT(false);//no othe rtypes supported yet.
            }
            fmj_ui_commit_nodes_for_drawing(arena,*child_node,quad_buffer,color,uvs);                            
        }
    }
}

FMJStretchBuffer komas;

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

void SetSpriteNonVisible(void* node)
{
    FMJUINode* a = (FMJUINode*)node;
    FMJSprite* s = fmj_stretch_buffer_check_out(FMJSprite,&asset_tables.sprites,a->sprite_id); 
    s->is_visible =false;
    fmj_stretch_buffer_check_in(&asset_tables.sprites);
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
            window->handle = created_window;
            WINSetScreenMode(ps,true);            
            ShowWindow(created_window, n_show_cmd);
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
    
    //init tables
    asset_tables.materials = fmj_anycache_init(4096,sizeof(FMJRenderMaterial),sizeof(u64),true);                                  
    asset_tables.sprites = fmj_stretch_buffer_init(1,sizeof(FMJSprite),8);
    //NOTE(RAY):If we want to make this platform independendt we would just make a max size of all platofrms
    //struct and put in this and get out the opaque pointer and cast it to what we need.
    asset_tables.vertex_buffers = fmj_stretch_buffer_init(1,sizeof(D3D12_VERTEX_BUFFER_VIEW),8);    
    asset_tables.geometries = fmj_anycache_init(4096,sizeof(FMJRenderGeometry),sizeof(u64),true);

    FMJStretchBuffer render_command_buffer = fmj_stretch_buffer_init(1,sizeof(FMJRenderCommand),8);
    FMJStretchBuffer matrix_buffer = fmj_stretch_buffer_init(1,sizeof(f4x4),8);

    //Set up file api options.
    ID3D12Device2* device = D12RendererCode::GetDevice();
        
    //Load a texture with all mip maps calculated
    //load texture to mem from disk
    LoadedTexture test_texture = get_loaded_image("test.png",4);
    LoadedTexture test_texture_2 = get_loaded_image("test2.png",4);    
    LoadedTexture koma_2 = get_loaded_image("koma_2.png",4);
    LoadedTexture ping_title = get_loaded_image("ping_title.png",4);
    LoadedTexture koma_outer_3 = get_loaded_image("koma_outer_3.png",4);    
    LoadedTexture koma_outer_2 = get_loaded_image("koma_outer_2.png",4);
    LoadedTexture koma_outer_1 = get_loaded_image("koma_outer_1.png",4);

    //court stuff
    LoadedTexture circle = get_loaded_image("circle.png",4);
    LoadedTexture line   = get_loaded_image("line.png",4);
    
    //upload texture data to gpu
    D12RendererCode::Texture2D(&test_texture,0);
    D12RendererCode::Texture2D(&test_texture_2,1);
    D12RendererCode::Texture2D(&koma_2,2);
    D12RendererCode::Texture2D(&ping_title,3);
    D12RendererCode::Texture2D(&koma_outer_1,5);
    D12RendererCode::Texture2D(&koma_outer_2,6);
    D12RendererCode::Texture2D(&koma_outer_3,7);        

    D12RendererCode::Texture2D(&circle,8);
    D12RendererCode::Texture2D(&line,9);        
    
    D12RendererCode::viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,ps->window.dim.x, ps->window.dim.y);
    D12RendererCode::sis_rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    FMJMemoryArena permanent_strings = fmj_arena_allocate(FMJMEGABYTES(1));
    FMJMemoryArena temp_strings = fmj_arena_allocate(FMJMEGABYTES(1));    
    FMJString base_path_to_data = fmj_string_create("../data/Desktop/",&permanent_strings);
    
    //setup audio system and load sound assets.
    SoundAssetCode::Init();
    SoundCode::Init();
    SoundCode::SetDefaultListener();
    PhysicsCode::Init();
    char* file_name = "Master.strings.bank";
    FMJString mat_final_path = fmj_string_append_char(base_path_to_data, file_name,&temp_strings);
        
    SoundAssetCode::CreateSoundBank(mat_final_path.string);
        
    char* bsfile_name = "Master.bank";
    mat_final_path = fmj_string_append_char(base_path_to_data, bsfile_name,&temp_strings);
    SoundAssetCode::CreateSoundBank(mat_final_path.string);
        
    char* afile_name = "bgm.bank";
    mat_final_path = fmj_string_append_char(base_path_to_data, afile_name, &temp_strings);
    SoundAssetCode::CreateSoundBank(mat_final_path.string);
    SoundAssetCode::LoadBankSampleData();
    //SoundAssetCode::GetBus("bus:/bgmgroup");
    SoundAssetCode::GetEvent("event:/bgm",&bgm_soundclip);

    //end audio setup

    //BEGIN Setup physics stuff
    material = PhysicsCode::CreateMaterial(0.0f, 0.0f, 1.0f);
    PhysicsCode::SetRestitutionCombineMode(material,physx::PxCombineMode::eMIN);
    PhysicsCode::SetFrictionCombineMode(material,physx::PxCombineMode::eMIN);
    scene = PhysicsCode::CreateScene(PhysicsCode::DefaultFilterShader);
    GamePiecePhysicsCallback* e = new GamePiecePhysicsCallback();
    PhysicsCode::SetSceneCallback(&scene, e);
    //GamePieceCode::SetPieces(scene, program, lt,life_lt, material, dim,true);
    //END PHYSICS SETUP
 
     // Create the descriptor heap for the depth-stencil view.
    D3D12_DESCRIPTOR_HEAP_DESC dsv_h_d = {};
    dsv_h_d.NumDescriptors = 1;
    dsv_h_d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_h_d.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HRESULT r = device->CreateDescriptorHeap(&dsv_h_d, IID_PPV_ARGS(&D12RendererCode::dsv_heap));
    ASSERT(SUCCEEDED(r));
    
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
    descriptorTableRanges[0].NumDescriptors = -1;//MAX_SRV_DESC_HEAP_COUNT; 
    descriptorTableRanges[0].BaseShaderRegister = 0; 
    descriptorTableRanges[0].RegisterSpace = 0;
    descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; 
    descriptorTableRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

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
    
    D3D12_ROOT_CONSTANTS rc_4 = {};
    rc_4.RegisterSpace = 0;
    rc_4.ShaderRegister = 0;
    rc_4.Num32BitValues = 4;

    D3D12_ROOT_PARAMETER1  root_params[5];
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
    
    root_params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_params[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    root_params[4].Constants = rc_4;

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

    D3D12_INPUT_ELEMENT_DESC input_layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR"   , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    
    uint32_t input_layout_count = _countof(input_layout);
    // Create a root/shader signature.

    char* vs_file_name = "vs_test.hlsl";
    char* fs_file_name = "ps_test.hlsl";
    RenderShader rs = D12RendererCode::CreateRenderShader(vs_file_name,fs_file_name);

    char* fs_color_file_name = "fs_color.hlsl";
    RenderShader rs_color = D12RendererCode::CreateRenderShader(vs_file_name,fs_color_file_name);
    
    PipelineStateStream ppss = D12RendererCode::CreateDefaultPipelineStateStreamDesc(input_layout,input_layout_count,rs.vs_blob,rs.fs_blob);
    ID3D12PipelineState* pipeline_state = D12RendererCode::CreatePipelineState(ppss);    

    PipelineStateStream color_ppss = D12RendererCode::CreateDefaultPipelineStateStreamDesc(input_layout,input_layout_count,rs.vs_blob,rs_color.fs_blob);
    ID3D12PipelineState* color_pipeline_state = D12RendererCode::CreatePipelineState(color_ppss);    

    FMJRenderMaterial base_render_material = {0};
    base_render_material.pipeline_state = (void*)pipeline_state;
    base_render_material.viewport_rect = f4_create(0,0,ps->window.dim.x,ps->window.dim.y);
    base_render_material.scissor_rect = f4_create(0,0,LONG_MAX,LONG_MAX);
    base_render_material.id = material_count;
    fmj_anycache_add_to_free_list(&asset_tables.materials,(void*)&material_count,&base_render_material);
    material_count++;

    FMJRenderMaterial color_render_material = base_render_material;
    color_render_material.pipeline_state = (void*)color_pipeline_state;
    color_render_material.viewport_rect = f4_create(0,0,ps->window.dim.x,ps->window.dim.y);
    color_render_material.scissor_rect = f4_create(0,0,LONG_MAX,LONG_MAX);    
    color_render_material.id = material_count;
    fmj_anycache_add_to_free_list(&asset_tables.materials,(void*)&material_count,&color_render_material);    
    material_count++;
    
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
    //create the descriptor heap that will store our srv
    //ID3D12DescriptorHeap* test_desc_heap;
    // = D12RendererCode::CreateDescriptorHeap(2,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    
    RenderCamera rc = {};
    rc.ot.p = f3_create(0,0,0);
    rc.ot.r = f3_axis_angle(f3_create(0,0,1),0);
    rc.ot.s = f3_create(1,1,1);

    f32 aspect_ratio = ps->window.dim.x / ps->window.dim.y;
    f2 size = f2_create_f(300);
    size.x = size.x * aspect_ratio;
    rc.projection_matrix = init_ortho_proj_matrix(size,0.0f,1.0f);
    
    rc.fov = 80;
    rc.near_far_planes = f2_create(0,1);
    rc.matrix = f4x4_identity();
    u64 ortho_matrix_id = fmj_stretch_buffer_push(&matrix_buffer,(void*)&rc.projection_matrix);
    u64 rc_matrix_id = fmj_stretch_buffer_push(&matrix_buffer,(void*)&rc.matrix);
    RenderCamera rc_ui = {};
    rc_ui.ot.p = f3_create(0,0,0);
    rc_ui.ot.r = f3_axis_angle(f3_create(0,0,1),0);
    rc_ui.ot.s = f3_create(1,1,1);
    rc_ui.projection_matrix = init_screen_space_matrix(ps->window.dim);
    rc_ui.matrix = f4x4_identity();
    
    u64 screen_space_matrix_id = fmj_stretch_buffer_push(&matrix_buffer,(void*)&rc_ui.projection_matrix);
    u64 identity_matrix_id = fmj_stretch_buffer_push(&matrix_buffer,(void*)&rc_ui.matrix);
    
    u64 quad_mem_size = SIZE_OF_SPRITE_IN_BYTES * 100;
    u64 matrix_mem_size = (sizeof(f32) * 16) * 100;    
    GPUArena quad_gpu_arena = D12RendererCode::AllocateStaticGPUArena(quad_mem_size);
    GPUArena ui_quad_gpu_arena = D12RendererCode::AllocateStaticGPUArena(quad_mem_size);    
    GPUArena matrix_gpu_arena = D12RendererCode::AllocateGPUArena(matrix_mem_size);

    FMJFixedBuffer fixed_quad_buffer = fmj_fixed_buffer_init(200,sizeof(SpriteTrans),8);
    FMJFixedBuffer ui_fixed_quad_buffer = fmj_fixed_buffer_init(200,sizeof(SpriteTrans),8);    
    FMJFixedBuffer matrix_quad_buffer = fmj_fixed_buffer_init(200,sizeof(f4x4),0);
    
    FMJSpriteBatch sb = {0};
    FMJSpriteBatch sb_ui = {0};
    sb.arena = fmj_arena_allocate(quad_mem_size);
    sb_ui.arena = fmj_arena_allocate(quad_mem_size);
    
    uint64_t stride = sizeof(float) * (3 + 4 + 2);
    f2 stbl = f2_create(0.0f,0.0f);
    f2 stbr = f2_create(1.0f,0.0f);
    f2 sttr = f2_create(1.0f,1.0f);
    f2 sttl = f2_create(0.0f,1.0f);
    f2 uvs[4] = {stbl,stbr,sttr,sttl};
    f4 white = f4_create(1,1,1,1);

    FMJUIState ui_state = {0};
    FMJUINode base_node = {0};
    base_node.rect.dim = f4_create(0,0,ps->window.dim.x,ps->window.dim.y);
    base_node.children = fmj_stretch_buffer_init(1,sizeof(FMJUINode),8);

    ui_state.parent_node = base_node;
    FMJSprite ui_sprite = {0};
    ui_sprite = fmj_sprite_init(3,uvs,white,true);

    FMJUINode bkg_child = {0};
    bkg_child.use_anchor = true;
    bkg_child.rect.anchor = f4_create(0.0f, 0.0f, 1.0f, 1.0f);
    bkg_child.rect.offset = f4_create(0,0,0,0);
    bkg_child.rect.color = f4_create(0,0,1,1);
    bkg_child.rect.highlight_color = f4_create(1,0,1,1);
    bkg_child.rect.current_color = bkg_child.rect.color;
    bkg_child.interactable = false;
    bkg_child.rect.z = 0.1f;
    bkg_child.children = fmj_stretch_buffer_init(1,sizeof(FMJUINode),8);
    bkg_child.type = fmj_ui_node_sprite;

    ui_sprite.material_id = color_render_material.id;
    bkg_child.sprite_id = fmj_stretch_buffer_push(&asset_tables.sprites,(void*)&ui_sprite);
    
    FMJUINode title_child = {0};
    title_child.use_anchor = true;
    title_child.rect.anchor = f4_create(0.25f, 0.25f, 0.75f, 0.75f);
    title_child.rect.offset = f4_create(0,0,0,0);
    title_child.rect.color = f4_create(1,1,1,1);
    title_child.rect.highlight_color = f4_create(1,0,1,1);
    title_child.rect.current_color = title_child.rect.color;
    title_child.interactable = false;
    title_child.rect.z = 0;
    title_child.type = fmj_ui_node_sprite;
    ui_sprite.tex_id = 3;
    ui_sprite.material_id = base_render_material.id;
    title_child.sprite_id = fmj_stretch_buffer_push(&asset_tables.sprites,(void*)&ui_sprite);
    
    u64 ping_ui_id = fmj_stretch_buffer_push(&bkg_child.children,&title_child);
    u64 bkg_ui_id = fmj_stretch_buffer_push(&base_node.children,&bkg_child);    

//pieces/komas
    komas = fmj_stretch_buffer_init(1,sizeof(Koma),8);
    
    FMJSprite base_koma_sprite_2 = add_sprite_to_stretch_buffer(&asset_tables.sprites,base_render_material.id,2,uvs,white,true);
    FMJSprite koma_sprite_outer_3 = add_sprite_to_stretch_buffer(&asset_tables.sprites,base_render_material.id,7,uvs,white,true);

    FMJSprite circle_sprite = fmj_sprite_init(8,uvs,white,true);
    FMJSprite line_sprite = fmj_sprite_init(9,uvs,white,true);
    FMJSprite court_sprite = fmj_sprite_init(0,uvs,white,true);        
    circle_sprite.material_id = base_render_material.id;
    line_sprite.material_id = base_render_material.id;
    court_sprite.material_id = color_render_material.id;
    

    u64 circle_sprite_id = fmj_stretch_buffer_push(&asset_tables.sprites,(void*)&circle_sprite);
    u64 line_sprite_id   = fmj_stretch_buffer_push(&asset_tables.sprites,(void*)&line_sprite);
    u64 court_sprite_id   = fmj_stretch_buffer_push(&asset_tables.sprites,(void*)&court_sprite);    

    quaternion def_r = f3_axis_angle(f3_create(0,0,1),0);
    f32 court_size_y = 500;
    f32 court_size_x = line.dim.x;
    
    u64  court_id = sprite_trans_create(f3_create_f(0),f3_create(line.dim.x,court_size_y,1),def_r,white,court_sprite_id,&matrix_buffer,&fixed_quad_buffer,&sb.arena);
//    u64  court_id = sprite_trans_create(f3_create_f(0),f3_create(100,100,1),def_r,white,court_sprite_id,&matrix_buffer,&fixed_quad_buffer,&sb.arena);        
    u64  circle_id = sprite_trans_create(f3_create_f(0),f3_create(30,30,1),def_r,white,circle_sprite_id,&matrix_buffer,&fixed_quad_buffer,&sb.arena);    
    u64  line_id = sprite_trans_create(f3_create_f(0),f3_create(line.dim.x,line.dim.y,1),def_r,white,line_sprite_id,&matrix_buffer,&fixed_quad_buffer,&sb.arena);

    f2  top_right_screen_xy = f2_create(ps->window.dim.x,ps->window.dim.y);
    f2  bottom_left_xy = f2_create(0,0);

    f3 max_screen_p = f3_screen_to_world_point(rc.projection_matrix,rc.matrix,ps->window.dim,top_right_screen_xy,0);
    f3 lower_screen_p = f3_screen_to_world_point(rc.projection_matrix,rc.matrix,ps->window.dim,bottom_left_xy,0);

    f32 half_court_x =  (f32)line.dim.x / 2.0f;
    f32 half_court_y = court_size_y / 2.0f;
    f32 court_slot_x = court_size_x / 6.0f;

    f32 x_offset = 0;
    u64 rot_test_st_id;

    f3 koma_bottom_left = f3_create(-half_court_x + court_slot_x,-half_court_y + court_slot_x,0);
    f3 koma_top_right = f3_create(-half_court_x + court_slot_x,half_court_y - court_slot_x,0);        
    f3 start_pos = f3_create_f(0);
    f3 current_pos = f3_create(0,0,-5);
    for(int o = 0;o < 2;++o)
    {
        if(o == 0)
            start_pos = koma_bottom_left;
        else
            start_pos = koma_top_right;

        f3 current_pos = start_pos;
        for(int i = 0;i < 10;++i)
        {
            f32 scale = 14;
            f3 next_p = current_pos;        
            Koma k = {0};

            u64  inner_id = sprite_trans_create(next_p,f3_create(scale,scale,1),def_r,white,base_koma_sprite_2.id,&matrix_buffer,&fixed_quad_buffer,&sb.arena);
//        k.velocity = f3_s_mul(0.05,f3_normalize(f3_create(f32_random_range(0,1),f32_random_range(0,1),0)));        
            k.inner_id = inner_id;
        
            scale = 18;

            u64  outer_id = sprite_trans_create(next_p,f3_create(scale,scale,1),def_r,white,koma_sprite_outer_3.id,&matrix_buffer,&fixed_quad_buffer,&sb.arena);
            k.outer_id = outer_id;
        
            PhysicsShapeSphere sphere_shape = PhysicsCode::CreateSphere(scale/2,material);
            RigidBodyDynamic rbd = PhysicsCode::CreateDynamicRigidbody(next_p, sphere_shape.shape, false);
            PhysicsCode::AddActorToScene(scene, rbd);
            PhysicsCode::DisableGravity(rbd.state,true);
            k.rigid_body = rbd;
            PhysicsCode::UpdateRigidBodyMassAndInertia(k.rigid_body,1);
            PhysicsCode::SetMass(k.rigid_body,1);

            fmj_stretch_buffer_push(&komas,(void*)&k);
            scale = scale * 2;
        
            if(0 == (i + 1) % 5)
            {
                if(o == 0)
                    current_pos = f3_add(current_pos,f3_create(0,court_slot_x,0));
                else
                    current_pos = f3_sub(current_pos,f3_create(0,court_slot_x,0));
            
                current_pos = f3_create(start_pos.x,current_pos.y,0);
            }
            else
            {
                current_pos = f3_add(current_pos,f3_create(court_slot_x,0,0));
            }        
        }
    }    

    fmj_ui_evaluate_node(&base_node,&ui_state.hot_node_state);
    fmj_ui_commit_nodes_for_drawing(&sb_ui.arena,base_node,&ui_fixed_quad_buffer,white,uvs);

    D12RendererCode::SetArenaToVertexBufferView(&quad_gpu_arena,quad_mem_size,stride);
    D12RendererCode::SetArenaToVertexBufferView(&ui_quad_gpu_arena,quad_mem_size,stride);    
    FMJRenderGeometry geo  = {0};
    geo.id = fmj_stretch_buffer_push(&asset_tables.vertex_buffers,(void*)&quad_gpu_arena.buffer_view);    

    FMJRenderGeometry ui_geo  = {0};    
    ui_geo.id = fmj_stretch_buffer_push(&asset_tables.vertex_buffers,(void*)&ui_quad_gpu_arena.buffer_view);

    fmj_anycache_add_to_free_list(&asset_tables.geometries,(void*)&geo.id,(void*)&geo);
    fmj_anycache_add_to_free_list(&asset_tables.geometries,(void*)&ui_geo.id,(void*)&ui_geo);

    D12RendererCode::UploadBufferData(&quad_gpu_arena,sb.arena.base,quad_mem_size);    
    D12RendererCode::UploadBufferData(&ui_quad_gpu_arena,sb_ui.arena.base,quad_mem_size);
    
    u64 m_stride = sizeof(f32) * 16;
    D12RendererCode::SetArenaToConstantBuffer(&matrix_gpu_arena,4);
    void* mapped_matrix_data;
    matrix_gpu_arena.resource->Map(0,NULL,&mapped_matrix_data);
    
#if 1
    quad_gpu_arena.resource->SetName(L"QUADS_GPU_ARENA");
    ui_quad_gpu_arena.resource->SetName(L"UI_QUADS_GPU_ARENA");    
#endif
    u32 tex_index = 0;
    f32 angle = 0;    
    f2 cam_pitch_yaw = f2_create(0.0f,0.0f);
    f32 x_move = 0;
    bool show_title = true;
    f32 outer_angle_rot = 0;    
    u32 tex_id;
    f32 size_sin = 0;
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

        if(ps->input.mouse.lmb.down)
        {
            //Begin the pull
            finger_pull.pull_begin = true;
            if(!finger_pull.koma)
            {
                //find the hovered unit
                PxScene* cs = scene.state;
                f3 origin_3 = f3_screen_to_world_point(rc.projection_matrix,rc.matrix,ps->window.dim,ps->input.mouse.p,0);                
                PxVec3 origin = PxVec3(origin_3.x,origin_3.y,origin_3.z);
                //PxVec3 origin = PxVec3(ps->input.mouse.p.x,ps->input.mouse.p.y,0);
                f3 unit_dir_ = f3_create(0,0,-1);
                PxVec3 unitDir = PxVec3(unit_dir_.x,unit_dir_.y,unit_dir_.z);
                PxReal maxDistance = 5.0f;                // [in] Raycast max distance
                PxRaycastBuffer hit;                      // [out] Raycast results
                // Raycast against all static & dynamic objects (no filtering)
                // The main result from this call is the closest hit, stored in the 'hit.block' structure
                bool status = cs->raycast(origin, unitDir, maxDistance, hit);
                if(status)
                {
                    for(int i = 0;i < komas.fixed.count;++i)
                    {
                        Koma* koma = fmj_stretch_buffer_check_out(Koma,&komas,i);
                        //For each piece check inside sphere radius if start touch is in side piece radius
//                        Koma* gp = &GamePieceCode::game_pieces[i];
                        if( hit.block.actor == koma->rigid_body.state)
                        {
                            finger_pull.koma = koma;
                            finger_pull.start_pull_p = ps->input.mouse.p;
                            break;
                        }
                        //set gp
                        fmj_stretch_buffer_check_in(&komas);
                    }
                }
            }
            //if no unit hovered ignore pull
            //else show pull graphic over piece
        }
        else
        {

            if(finger_pull.pull_begin)
            {
                finger_pull.end_pull_p  = ps->input.mouse.p;
                if(!CheckValidFingerPull(&finger_pull))
                {
                    finger_pull.pull_begin = false;
                }
                if(finger_pull.pull_begin && finger_pull.koma)
                {
                    //Pull was valid
                    f2 pull_dif = f2_sub(finger_pull.end_pull_p,finger_pull.start_pull_p);
                    f3 flick_dir = f3_create(pull_dif.x,pull_dif.y,0);
                    flick_dir = f3_create(flick_dir.x,flick_dir.y,0);
                    PxVec3 pulldirpx3 = PxVec3(flick_dir.x,flick_dir.y,flick_dir.z);
                    finger_pull.koma->rigid_body.state->setLinearVelocity(pulldirpx3);
                    finger_pull.koma = nullptr;
                    finger_pull.start_pull_p = f2_create(0,0);
                    finger_pull.end_pull_p = f2_create(0,0);
                    finger_pull.pull_begin = false;
                }
            }
        }
        //Set friction on moving bodies


#if 0
        f4x4* p_mat = fmj_stretch_buffer_check_out(f4x4,&matrix_buffer,ortho_matrix_id);
        size.x = abs(sin(size_sin)) * 300;
        size.y = abs(cos(size_sin)) * 300;
        
        size.x = clamp(size.x,150,300);
        size.y = clamp(size.x,150,300);
        size.x = size.x * aspect_ratio;
        fmj_stretch_buffer_check_in(&matrix_buffer);
        size_sin += 0.001f;        
        *p_mat = init_ortho_proj_matrix(size,0.0f,1.0f);
#endif
        
        for(int i = 0;i < komas.fixed.count;++i)
        {
            Koma* k = fmj_stretch_buffer_check_out(Koma,&komas,i);


            
            PxVec3 lv = k->rigid_body.state->getLinearVelocity();
            lv *= ground_friction;
            k->rigid_body.state->setLinearVelocity(lv);
            
            SpriteTrans* inner_koma_st = fmj_fixed_buffer_get_ptr(SpriteTrans,&fixed_quad_buffer,k->inner_id);
            SpriteTrans* outer_koma_st = fmj_fixed_buffer_get_ptr(SpriteTrans,&fixed_quad_buffer,k->outer_id);
            FMJ3DTrans  new_trans =  inner_koma_st->t;

            f4x4 c_mat = fmj_stretch_buffer_get(f4x4,&matrix_buffer,rc_matrix_id);
            
            f3 p = inner_koma_st->t.p;
//            f3 new_vel = k->velocity;
            /*
            f3 v = k->velocity;
            {
                if(p.x > max_screen_p.x)
                {
                    new_vel = f3_reflect(v,f3_create(-1,0,0));
                }
                else if(p.y > max_screen_p.y)
                {
                    new_vel = f3_reflect(v,f3_create(0,-1,0));
                }else if(p.x < lower_screen_p.x)
                {
                    new_vel = f3_reflect(v,f3_create(1,0,0));
                }
                else if(p.y < lower_screen_p.y)
                {
                    new_vel = f3_reflect(v,f3_create(0,1,0));                    
                }
            }
            */
            PxTransform pxt = k->rigid_body.state->getGlobalPose();
            f3 new_p = f3_create(pxt.p.x,pxt.p.y,pxt.p.z);
            
//            k->velocity = f3_s_mul(0.05,f3_normalize(new_vel));
            inner_koma_st->t.p = new_p;//f3_add(inner_koma_st->t.p,k->velocity);
            outer_koma_st->t.p = inner_koma_st->t.p;
            
            outer_koma_st->t.r = f3_axis_angle(f3_create(0,0,1),outer_angle_rot);
            outer_angle_rot += 0.01f;
            fmj_3dtrans_update(&outer_koma_st->t);
            fmj_3dtrans_update(&inner_koma_st->t);
            
            FMJSprite* s = fmj_stretch_buffer_check_out(FMJSprite,&asset_tables.sprites,outer_koma_st->sprite_id);
            u32 id = s->tex_id;
            f4x4* outer_model_matrix = fmj_stretch_buffer_check_out(f4x4,&matrix_buffer,outer_koma_st->model_matrix_id);
            *outer_model_matrix = outer_koma_st->t.m;
            f4x4* inner_model_matrix = fmj_stretch_buffer_check_out(f4x4,&matrix_buffer,inner_koma_st->model_matrix_id);
            *inner_model_matrix = inner_koma_st->t.m;
            
            if(ps->input.mouse.lmb.pressed)
            {
                show_title = false;
                fmj_ui_evaluate_on_node_recursively(&base_node,SetSpriteNonVisible);

                id =  (id - 1);
                if(id < 5)id = 7;
                s->tex_id = id;
            }
        }

        SoundCode::ContinousPlay(&bgm_soundclip);
//Render camera stated etc..  is finalized        
 //Free cam
#if 0
        cam_pitch_yaw = f2_add(cam_pitch_yaw,ps->input.mouse.delta_p);
        quaternion pitch = f3_axis_angle(f3_create(1, 0, 0), cam_pitch_yaw.y);
        quaternion yaw   = f3_axis_angle(f3_create(0, 1, 0), cam_pitch_yaw.x * -1);
        quaternion turn_qt = quaternion_mul(pitch,yaw);        
        rc.ot.r = turn_qt;
#endif
        rc.matrix = fmj_3dtrans_set_cam_view(&rc.ot);
        f4x4* rc_mat = fmj_stretch_buffer_check_out(f4x4,&matrix_buffer,rc_matrix_id);
        *rc_mat = rc.matrix;
        fmj_stretch_buffer_check_in(&matrix_buffer);
        
        for(int i = 0;i < fixed_quad_buffer.count;++i)
        {
            FMJRenderCommand com = {};
            SpriteTrans st = fmj_fixed_buffer_get(SpriteTrans,&fixed_quad_buffer,i);
            FMJSprite s = fmj_stretch_buffer_get(FMJSprite,&asset_tables.sprites,st.sprite_id);
            if(s.is_visible)
            {
                geo.offset = (i * 6);
                com.geometry = geo;
                com.material_id = s.material_id;
                com.texture_id = s.tex_id;
                com.model_matrix_id = st.model_matrix_id;
                com.camera_matrix_id = rc_matrix_id;
                com.perspective_matrix_id = ortho_matrix_id;
                fmj_stretch_buffer_push(&render_command_buffer,(void*)&com);
            }            
        }

/*
        for(int i = 0;i < ui_fixed_quad_buffer.count;++i)
        {
            FMJRenderCommand com = {};
            SpriteTrans st = fmj_fixed_buffer_get(SpriteTrans,&ui_fixed_quad_buffer,i);
            FMJSprite s = fmj_stretch_buffer_get(FMJSprite,&asset_tables.sprites,st.sprite_id);
            if(s.is_visible)
            {
                ui_geo.offset = (i * 6);
                com.geometry = ui_geo;
                com.material_id = s.material_id;
                com.texture_id = s.tex_id;
                com.model_matrix_id = identity_matrix_id;
                com.camera_matrix_id = identity_matrix_id;
                com.perspective_matrix_id = screen_space_matrix_id;
                fmj_stretch_buffer_push(&render_command_buffer,(void*)&com);
            }
        }
*/
        
//Render commands are issued to the api

        //Render API CODE

        bool has_update = false;

        if(render_command_buffer.fixed.count > 0)
        {
            D12RendererCode::AddStartCommandListCommand();                            
            //screen space cam
            for(int i = 0;i < render_command_buffer.fixed.count;++i)
            {
                FMJRenderCommand command = fmj_stretch_buffer_get(FMJRenderCommand,&render_command_buffer,i);
                {
                    f4x4 m_mat = fmj_stretch_buffer_get(f4x4,&matrix_buffer,command.model_matrix_id);
                    f4x4 c_mat = fmj_stretch_buffer_get(f4x4,&matrix_buffer,command.camera_matrix_id);
                    f4x4 p_mat = fmj_stretch_buffer_get(f4x4,&matrix_buffer,command.perspective_matrix_id);
                    f4x4 world_mat = f4x4_mul(c_mat,m_mat);
                    f4x4 finalmat = f4x4_mul(p_mat,world_mat);
                    m_mat.c0.x = (f32)(matrix_quad_buffer.count * sizeof(f4x4));

                    fmj_fixed_buffer_push(&matrix_quad_buffer,(void*)&finalmat);        

                    D12RendererCode::AddRootSignatureCommand(D12RendererCode::root_sig);
                    D12RendererCode::AddViewportCommand(f4_create(0,0,ps->window.dim.x,ps->window.dim.y));
                    D12RendererCode::AddScissorRectCommand(CD3DX12_RECT(0,0,LONG_MAX,LONG_MAX));

                    FMJRenderMaterial material = fmj_anycache_get(FMJRenderMaterial,&asset_tables.materials,(void*)&command.material_id);
                    D12RendererCode::AddPipelineStateCommand((ID3D12PipelineState*)material.pipeline_state);                    
                    D12RendererCode::AddGraphicsRoot32BitConstant(0,16,&m_mat,0);                    
                    D12RendererCode::AddGraphicsRoot32BitConstant(2,16,&finalmat,0);
                    
                    tex_index = command.texture_id;
                    D12RendererCode::AddGraphicsRoot32BitConstant(4,4,&tex_index,0);
                    D12RendererCode::AddGraphicsRootDescTable(1,D12RendererCode::default_srv_desc_heap,D12RendererCode::default_srv_desc_heap->GetGPUDescriptorHandleForHeapStart());
                    D3D12_VERTEX_BUFFER_VIEW bv = fmj_stretch_buffer_get(D3D12_VERTEX_BUFFER_VIEW,&asset_tables.vertex_buffers,command.geometry.id);
                    D12RendererCode::AddDrawCommand(command.geometry.offset,6,D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,bv);
                    has_update = true;
                }
            }
            D12RendererCode::AddEndCommandListCommand();                            
        }

        if(has_update)
        {
            memcpy((void*)mapped_matrix_data,(void*)matrix_quad_buffer.mem_arena.base,matrix_quad_buffer.mem_arena.used);
            fmj_fixed_buffer_clear(&matrix_quad_buffer);
        }

//        fmj_fixed_buffer_clear(&ui_fixed_quad_buffer);        
//Command are finalized and rendering is started.        
 // TODO(Ray Garner): Add render targets commmand
//        if(show_title)
 
//        DrawTest test = {cube,rc,gpu_arena,test_desc_heap};
//Post frame work is done.
        D12RendererCode::EndFrame();

        fmj_stretch_buffer_clear(&render_command_buffer);
//Handle windows message and do it again.
        PhysicsCode::Update(&scene,1,0.016f);                
//        SoundCode::Update();
        HandleWindowsMessages(ps);        
    }
    return 0;
}
