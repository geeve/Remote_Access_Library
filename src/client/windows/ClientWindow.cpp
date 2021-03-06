#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include "resource.h"
#include "ClientWindow.h"
#include "Configs.h"
#include "ClientDriver.h"
#include "IClientDriver.h"
#include "NetworkStructs.h"
#include "Clipboard_Lite.h"
#include "ScreenCapture.h"

const char WindowClass[] = "RAT_PROJECT";

namespace SL {
    namespace RAT_Client {

        struct MonitorInfo {
            Screen_Capture::Monitor Monitor;
            std::shared_ptr<HBITMAP__> Bitmap;
        };
        class ClientWindowImpl : public RAT::IClientDriver {
            RAT::ClientDriver ClientDriver_;
            Clipboard_Lite::Clipboard_Manager Clipboard_Manager_;
            std::vector<MonitorInfo> Monitors;

            std::shared_ptr<SL::WS_LITE::IWSocket> Socket_;

            HWND hWnd = nullptr;
            HWND ConnecthWnd = nullptr;
            HINSTANCE hInstance = nullptr;
            std::shared_ptr<SL::RAT::Client_Config> Config;
        public:

            static INT_PTR CALLBACK DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
            {
                //static so it lasts for ever!!
                static char Hostname[256];
                switch (Msg)
                {
                case WM_INITDIALOG:
                    return TRUE;
                case WM_COMMAND:
                    switch (wParam)
                    {
                    case ID_TRYCONNTECT:
                        GetDlgItemTextA(hWndDlg, IDC_HOSTTEXTBOX, Hostname, sizeof(Hostname));
                        PostMessage(GetParent(hWndDlg), WM_COMMAND, ID_TRYCONNTECT, (LPARAM)Hostname);
                        return TRUE;
                    case ID_CANCELCONNTECT:
                        PostMessage(GetParent(hWndDlg), WM_COMMAND, ID_TRYCONNTECTCLOSING, 0);
                        EndDialog(hWndDlg, 0);
                        return TRUE;
                    }
                    break;
                case WM_CLOSE:
                case WM_DESTROY:
                    PostMessage(GetParent(hWndDlg), WM_COMMAND, ID_TRYCONNTECTCLOSING, 0);
                    EndDialog(hWndDlg, 0);
                    return TRUE;
                default:
                    return FALSE;
                }
                return FALSE;
            }
            void SpecialKeyImpl(WPARAM wParam, bool pressed) {
                Input_Lite::SpecialKeyCodes k;
                switch (wParam)
                {
                case VK_BACK:
                    k = Input_Lite::SpecialKeyCodes::BACKSPACE;
                    break;
                case VK_TAB:
                    k = Input_Lite::SpecialKeyCodes::TAB;
                    break;
                case VK_RETURN:
                    k = Input_Lite::SpecialKeyCodes::ENTER;
                    break;
                case VK_SHIFT:
                    k = Input_Lite::SpecialKeyCodes::SHIFTLEFT;
                    break;
                case VK_CONTROL:
                    k = Input_Lite::SpecialKeyCodes::CONTROLLEFT;
                    break;
                case VK_CAPITAL:
                    k = Input_Lite::SpecialKeyCodes::CAPSLOCK;
                    break;
                case VK_ESCAPE:
                    k = Input_Lite::SpecialKeyCodes::ESCAPE;
                    break;
                case VK_SPACE:
                    k = Input_Lite::SpecialKeyCodes::SPACE;
                    break;
                case VK_PRIOR:
                    k = Input_Lite::SpecialKeyCodes::PAGEUP;
                    break;
                case VK_NEXT:
                    k = Input_Lite::SpecialKeyCodes::PAGEDOWN;
                    break;

                case VK_END:
                    k = Input_Lite::SpecialKeyCodes::END;
                    break;
                case VK_HOME:
                    k = Input_Lite::SpecialKeyCodes::HOME;
                    break;
                case VK_LEFT:
                    k = Input_Lite::SpecialKeyCodes::ARROWLEFT;
                    break;
                case VK_UP:
                    k = Input_Lite::SpecialKeyCodes::ARROWUP;
                    break;

                case VK_RIGHT:
                    k = Input_Lite::SpecialKeyCodes::ARROWRIGHT;
                    break;
                case VK_DOWN:
                    k = Input_Lite::SpecialKeyCodes::ARROWDOWN;
                    break;
                case VK_SNAPSHOT:
                    k = Input_Lite::SpecialKeyCodes::PRINTSCREEN;
                    break;
                case VK_INSERT:
                    k = Input_Lite::SpecialKeyCodes::INSERT;
                    break;
                case VK_DELETE:
                    k = Input_Lite::SpecialKeyCodes::DELETE;
                    break;

                case VK_SCROLL:
                    k = Input_Lite::SpecialKeyCodes::SCROLLLOCK;
                    break;
                case VK_PAUSE:
                    k = Input_Lite::SpecialKeyCodes::PAUSE;
                    break;
                case VK_LWIN:
                    k = Input_Lite::SpecialKeyCodes::OSLEFT;
                    break;
                case VK_F1:
                case VK_F2:
                case VK_F3:
                case VK_F4:
                case VK_F5:
                case VK_F6:
                case VK_F7:
                case VK_F8:
                case VK_F9:
                case VK_F10:
                case VK_F11:
                case VK_F12:
                case VK_F13:
                case VK_F14:
                case VK_F15:
                case VK_F16:
                case VK_F17:
                case VK_F18:
                    k = static_cast<Input_Lite::SpecialKeyCodes>(Input_Lite::SpecialKeyCodes::F1 + (wParam - VK_F1));
                    break;

                case VK_NUMLOCK:
                    k = Input_Lite::SpecialKeyCodes::NUMLOCK;
                    break;
                case VK_NUMPAD1:
                    k = Input_Lite::SpecialKeyCodes::NUMPAD1;
                    break;
                case VK_NUMPAD2:
                    k = Input_Lite::SpecialKeyCodes::NUMPAD2;
                    break;
                case VK_NUMPAD3:
                    k = Input_Lite::SpecialKeyCodes::NUMPAD3;
                    break;
                case VK_NUMPAD4:
                    k = Input_Lite::SpecialKeyCodes::NUMPAD4;
                    break;
                case VK_NUMPAD5:
                    k = Input_Lite::SpecialKeyCodes::NUMPAD5;
                    break;
                case VK_NUMPAD6:
                    k = Input_Lite::SpecialKeyCodes::NUMPAD6;
                    break;
                case VK_NUMPAD7:
                    k = Input_Lite::SpecialKeyCodes::NUMPAD7;
                    break;
                case VK_NUMPAD8:
                    k = Input_Lite::SpecialKeyCodes::NUMPAD8;
                    break;
                case VK_NUMPAD9:
                    k = Input_Lite::SpecialKeyCodes::NUMPAD9;
                    break;

                case VK_ADD:
                    k = Input_Lite::SpecialKeyCodes::NUMPADADD;
                    break;
                case VK_DECIMAL:
                    k = Input_Lite::SpecialKeyCodes::NUMPADDECIMAL;
                    break;
                case VK_DIVIDE:
                    k = Input_Lite::SpecialKeyCodes::NUMPADDIVIDE;
                    break;
                case VK_MULTIPLY:
                    k = Input_Lite::SpecialKeyCodes::NUMPADMULTIPLY;
                    break;
                case VK_SUBTRACT:
                    k = Input_Lite::SpecialKeyCodes::NUMPADSUBTRACT;
                    break;
                default:
                    return;
                }

                if (pressed) {
                    ClientDriver_.SendKeyDown(k);
                }
                else {
                    ClientDriver_.SendKeyUp(k);
                }
            }
            LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

                switch (uMsg)
                {
                case WM_COMMAND:
                    switch (wParam)
                    {
                    case ID_TRYCONNTECT:
                        TryConnect(reinterpret_cast<const char*>(lParam));
                        break;
                    case IDD_CONNECTTODIALOG:
                        if (!IsWindow(ConnecthWnd))
                        {
                            ConnecthWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CONNECTTO), hwnd, DlgProc);
                            ShowWindow(ConnecthWnd, SW_SHOW);
                        }
                        break;
                    case ID_TRYCONNTECTCLOSING:
                        ConnecthWnd = nullptr;
                        if (IsWindowVisible(hwnd) == FALSE) {
                            PostQuitMessage(0);
                            break;
                        }
                        break;
                    }
                    break;
                case WM_MOUSEMOVE:
                    ClientDriver_.SendMousePosition(RAT::Point(LOWORD(lParam), HIWORD(lParam)));
                    break;
                case WM_LBUTTONDOWN:
                    ClientDriver_.SendMouseDown(Input_Lite::MouseButtons::LEFT);
                    break;
                case WM_LBUTTONUP:
                    ClientDriver_.SendMouseUp(Input_Lite::MouseButtons::LEFT);
                    break;
                case WM_RBUTTONDOWN:
                    ClientDriver_.SendMouseDown(Input_Lite::MouseButtons::RIGHT);
                    break;
                case WM_RBUTTONUP:
                    ClientDriver_.SendMouseUp(Input_Lite::MouseButtons::RIGHT);
                    break;
                case WM_MBUTTONDOWN:
                    ClientDriver_.SendMouseDown(Input_Lite::MouseButtons::MIDDLE);
                    break;
                case WM_MBUTTONUP:
                    ClientDriver_.SendMouseUp(Input_Lite::MouseButtons::MIDDLE);
                    break;
                case WM_MOUSEWHEEL:
                    ClientDriver_.SendMouseScroll(GET_WHEEL_DELTA_WPARAM(wParam) / 120);
                    break;

                case WM_CHAR:
                    ClientDriver_.SendKeyDown(static_cast<char>(wParam));
                    ClientDriver_.SendKeyUp(static_cast<char>(wParam));
                    break;
                case WM_KEYUP:
                    SpecialKeyImpl(wParam, false);
                    break;
                case WM_KEYDOWN:
                    SpecialKeyImpl(wParam, true);
                    break;
                case WM_PAINT:
                {
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hwnd, &ps);

                    for (size_t i = 0; i < Monitors.size(); i++) {
                        if (!Monitors[i].Bitmap) continue;
                        auto b = Monitors[i].Bitmap.get();
                        auto hdcMem = CreateCompatibleDC(hdc);
                        auto oldBitmap = SelectObject(hdcMem, b);
                        BITMAP          bitmap;

                        GetObject(b, sizeof(bitmap), &bitmap);
                        BitBlt(hdc, Monitors[i].Monitor.OffsetX, Monitors[i].Monitor.OffsetY, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

                        SelectObject(hdcMem, oldBitmap);
                        DeleteDC(hdcMem);

                    }
                    EndPaint(hwnd, &ps);
                }
                break;
                case WM_DESTROY:
                    PostQuitMessage(0);
                    break;
                default:
                    return DefWindowProc(hwnd, uMsg, wParam, lParam);
                }
                return 0;


            }
            static LRESULT CALLBACK WindowProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

                if (uMsg == WM_NCCREATE)
                {
                    auto cs = (CREATESTRUCT*)lParam;
                    auto p = (ClientWindowImpl*)cs->lpCreateParams;
                    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)p);
                    return TRUE;
                }
                else
                {
                    auto thisclass = (ClientWindowImpl*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
                    if (thisclass) {
                        return thisclass->WindowProc(hwnd, uMsg, wParam, lParam);
                    }
                }
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }
            ClientWindowImpl(std::shared_ptr<SL::RAT::Client_Config>& config, const std::string& host) :Config(config), ClientDriver_(this) {

                hInstance = GetModuleHandle(NULL);
                WNDCLASSEXA wcex;
                wcex.cbSize = sizeof(wcex);

                wcex.style = CS_HREDRAW | CS_VREDRAW;
                wcex.lpfnWndProc = WindowProcStatic;
                wcex.cbClsExtra = 0;
                wcex.cbWndExtra = 0;
                wcex.hInstance = hInstance;
                wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROJECTICON));
                wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
                wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
                wcex.lpszMenuName = 0;
                wcex.lpszClassName = WindowClass;
                wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_PROJECTICON));

                RegisterClassExA(&wcex);
                hWnd = CreateWindowA(WindowClass, WindowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, this);
                if (hWnd)
                {
                    if (host.empty() || host.size() < 2) {
                        PostMessage(hWnd, WM_COMMAND, IDD_CONNECTTODIALOG, 0);
                    }
                    else {
                        TryConnect(host.c_str());
                    }
                }
                Clipboard_Manager_ = Clipboard_Lite::CreateClipboard().onText([&](const std::string& text) {

                }).run();

            }
            virtual ~ClientWindowImpl() {
                Clipboard_Manager_.destroy();//make sure no race conditions occur
            }
            void Run() {

                MSG msg;
                BOOL bRet;
                while ((bRet = GetMessage(&msg, 0, 0, 0)) != 0)
                {
                    if (bRet == -1)
                    {
                        break;//get out of the loop
                    }
                    else
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            }
            void TryConnect(const char* hostname) {
                Monitors.clear();// free memory
                ClientDriver_.Connect(Config, hostname);
            }
            virtual void onConnection(const std::shared_ptr<SL::WS_LITE::IWSocket>& socket) override {
                Socket_ = socket;
                //make sure to show the window
                ShowWindow(hWnd, SW_SHOW);
                UpdateWindow(hWnd);
                PostMessage(ConnecthWnd, WM_CLOSE, 0, 0);
                ConnecthWnd = nullptr;
                std::cout << "onConnection" << std::endl;
            }
            virtual void onMessage(const std::shared_ptr<SL::WS_LITE::IWSocket>& socket, const WS_LITE::WSMessage& msg) override {
                std::cout << "onMessage" << std::endl;
            }
            virtual void onDisconnection(const std::shared_ptr<SL::WS_LITE::IWSocket>& socket, unsigned short code, const std::string& msg) override {

                ShowWindow(hWnd, SW_HIDE);
                UpdateWindow(hWnd);
                std::cout << msg << std::endl;
            }
            virtual void onMonitorsChanged(const std::vector<Screen_Capture::Monitor>& monitors) override {
                Monitors.clear();// free memory
                Monitors.resize(monitors.size());
                for (size_t i = 0; i < Monitors.size(); i++) {
                    Monitors[i].Monitor = monitors[i];
                }
                std::cout << "onMonitorsChanged" << std::endl;
            }
            virtual void onFrameChanged(const RAT::Image& img, const SL::Screen_Capture::Monitor& monitor) override {
                std::cout << "onFrameChanged" << std::endl;
                HDC hdc = GetDC(hWnd);

                BITMAPINFO info;
                ZeroMemory(&info, sizeof(BITMAPINFO));
                info.bmiHeader.biBitCount = 32;
                info.bmiHeader.biWidth = img.Rect_.Width;
                info.bmiHeader.biHeight = -img.Rect_.Height;
                info.bmiHeader.biPlanes = 1;
                info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                info.bmiHeader.biSizeImage = img.Length;
                info.bmiHeader.biCompression = BI_RGB;

                auto mon = std::find_if(begin(Monitors), end(Monitors), [&](const MonitorInfo& m) { return monitor.Id == m.Monitor.Id; });
                if (mon != end(Monitors)) {

                    auto bitmaptodraw = CreateDIBitmap(hdc, &info.bmiHeader, CBM_INIT, (void*)img.Data, &info, DIB_RGB_COLORS);

                    auto srcmem = CreateCompatibleDC(hdc);
                    auto srcbitmap = SelectObject(srcmem, bitmaptodraw);

                    auto hdcMem = CreateCompatibleDC(hdc);
                    auto oldBitmap = SelectObject(hdcMem, mon->Bitmap.get());

                    BitBlt(hdcMem, img.Rect_.left(), img.Rect_.top(), img.Rect_.Width, img.Rect_.Height, srcmem, 0, 0, SRCCOPY);

                    SelectObject(srcmem, srcbitmap);
                    DeleteObject(bitmaptodraw);

                    SelectObject(hdcMem, oldBitmap);
                    DeleteDC(hdcMem);
                    RECT RECT_ImageUpdate_Window;
                    RECT_ImageUpdate_Window.left = img.Rect_.left();
                    RECT_ImageUpdate_Window.top = img.Rect_.top();
                    RECT_ImageUpdate_Window.bottom = img.Rect_.bottom();
                    RECT_ImageUpdate_Window.right = img.Rect_.right();

                    InvalidateRect(hWnd, &RECT_ImageUpdate_Window, FALSE);
                    UpdateWindow(hWnd);
                }

                // StretchDIBits(hdc, img.Rect_.left(), img.Rect_.top(), img.Rect_.Width, img.Rect_.Height, 0, 0, img.Rect_.Width, img.Rect_.Height, img.Data, &info, DIB_RGB_COLORS, SRCCOPY);
                ReleaseDC(hWnd, hdc);
            }
            virtual void onNewFrame(const RAT::Image& img, const SL::Screen_Capture::Monitor& monitor) override {
                std::cout << "onNewFrame" << std::endl;
                HDC hdc = GetDC(hWnd);

                BITMAPINFO info;
                ZeroMemory(&info, sizeof(BITMAPINFO));
                info.bmiHeader.biBitCount = 32;
                info.bmiHeader.biWidth = img.Rect_.Width;
                info.bmiHeader.biHeight = -img.Rect_.Height;
                info.bmiHeader.biPlanes = 1;
                info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                info.bmiHeader.biSizeImage = img.Length;
                info.bmiHeader.biCompression = BI_RGB;
                auto mon = std::find_if(begin(Monitors), end(Monitors), [&](const MonitorInfo& m) { return monitor.Id == m.Monitor.Id; });
                if (mon != end(Monitors)) {
                    mon->Bitmap = std::shared_ptr<HBITMAP__>(CreateDIBitmap(hdc, &info.bmiHeader, CBM_INIT, (void*)img.Data, &info, DIB_RGB_COLORS), [](HBITMAP__* p) {  DeleteObject(p);  });
                    RECT RECT_ImageUpdate_Window;
                    RECT_ImageUpdate_Window.left = img.Rect_.left();
                    RECT_ImageUpdate_Window.top = img.Rect_.top();
                    RECT_ImageUpdate_Window.bottom = img.Rect_.bottom();
                    RECT_ImageUpdate_Window.right = img.Rect_.right();
                    InvalidateRect(hWnd, &RECT_ImageUpdate_Window, FALSE);
                    UpdateWindow(hWnd);
                }
                ReleaseDC(hWnd, hdc);
            }
            virtual void onMouseImageChanged(const RAT::Image& img) override {
                std::cout << "onMouseImageChanged" << std::endl;
            }
            virtual void onMousePositionChanged(const RAT::Point& mevent) override {
                std::cout << "onMousePositionChanged" << std::endl;
            }
            virtual void onClipboardChanged(const std::string& text) override {
                std::cout << "onClipboardChanged" << std::endl;
            }

        };


        ClientWindow::ClientWindow(std::shared_ptr<SL::RAT::Client_Config>& config, const std::string& host) {
            ClientWindowImpl_ = new ClientWindowImpl(config, host);
        }
        ClientWindow::~ClientWindow() {
            delete ClientWindowImpl_;
        }

        void ClientWindow::Run() {
            ClientWindowImpl_->Run();
        }
    }
}
