module;

#define NOMINMAX
#include <Windows.h>

export module hwnd_wrapper;
import std;

export struct HwndWrapper
{
    HWND hwnd;

    HwndWrapper(WNDCLASSEXW& wc)
    {
        hwnd = ::CreateWindowW(wc.lpszClassName, L"Amigurumi designer", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
        if (hwnd == NULL)
        {
            throw std::runtime_error("could not creatw window");
        }
    }
    ~HwndWrapper()
    {
        ::DestroyWindow(hwnd);
    }
    void show_window() const
    {
        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);
    }
};