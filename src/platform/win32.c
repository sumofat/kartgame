
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

float2 GetWin32WindowDim(PlatformState* ps)
{
    RECT client_rect;
    GetClientRect(ps->window.handle, &client_rect);
	return float2(client_rect.right - client_rect.left, client_rect.bottom - client_rect.top);
}


int WINAPI WinMain(HINSTANCE h_instance,HINSTANCE h_prev_instance, LPSTR lp_cmd_line, int n_show_cmd )
{
    float2 screen_size = float2(DEFAULT_SCREEN_WIDTH,DEFAULT_SCREEN_WIDTH);

}
