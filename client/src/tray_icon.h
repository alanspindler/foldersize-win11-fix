#pragma once

#include <windows.h>
#include <shellapi.h>

class ExplorerTracker;

class TrayIcon {
public:
    TrayIcon(HINSTANCE instance, ExplorerTracker& tracker);
    ~TrayIcon();

    bool Create();
    void Destroy();
    HWND Hwnd() const { return hwnd_; }

private:
    void CreateHiddenWindow();
    void ShowContextMenu();
    void UpdateStartupRegistry(bool enable);
    bool StartupEnabled() const;
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HINSTANCE instance_;
    ExplorerTracker& tracker_;
    HWND hwnd_ = nullptr;
    HMENU menu_ = nullptr;
    NOTIFYICONDATAW iconData_{};
};
