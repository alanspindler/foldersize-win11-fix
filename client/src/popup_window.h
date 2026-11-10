#pragma once

#include <string>
#include <windows.h>

class PopupWindow {
public:
    PopupWindow() = default;
    ~PopupWindow();

    PopupWindow(const PopupWindow&) = delete;
    PopupWindow& operator=(const PopupWindow&) = delete;

    bool Create(HWND explorerHwnd);
    void Destroy();
    void Update(const std::wstring& folderPath, bool visible);

    HWND ExplorerHwnd() const { return explorerHwnd_; }
    HWND Hwnd() const { return hwnd_; }

    static void RegisterWindowClass(HINSTANCE instance);
    static void UnregisterWindowClass(HINSTANCE instance);

private:
    void RefreshContents();
    void PositionNearExplorer();
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND explorerHwnd_ = nullptr;
    HWND hwnd_ = nullptr;
    HWND listView_ = nullptr;
    std::wstring folderPath_;
    static constexpr UINT kRefreshTimerId = 1;
};
