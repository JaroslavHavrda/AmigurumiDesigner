module;

#define NOMINMAX
#include <Windows.h>

export module hwnd_wrapper;
import std;

export struct hwnd_wrapper
{
    HWND hwnd;

    hwnd_wrapper(WNDCLASSEXW& wc)
    {
        hwnd = ::CreateWindowW(wc.lpszClassName, L"Amigurumi designer", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
        if (hwnd == NULL)
        {
            throw std::runtime_error("could not creatw window");
        }
    }
    ~hwnd_wrapper()
    {
        ::DestroyWindow(hwnd);
    }
    void show_window() const
    {
        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);
    }
};