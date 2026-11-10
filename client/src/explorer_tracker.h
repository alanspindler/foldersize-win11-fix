#pragma once

#include "popup_window.h"

#include <map>
#include <memory>
#include <windows.h>

class ExplorerTracker {
public:
    explicit ExplorerTracker(HINSTANCE instance);
    ~ExplorerTracker();

    bool Initialize(HWND messageHwnd);
    void Shutdown();

    void SetPopupsEnabled(bool enabled);
    bool PopupsEnabled() const { return popupsEnabled_; }

    void SyncNow();

private:
    static LRESULT CALLBACK MessageWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void CreateMessageWindow();
    void OnTimer();
    void ReconcilePopups();

    HINSTANCE instance_;
    HWND messageHwnd_ = nullptr;
    bool popupsEnabled_ = true;
    std::map<HWND, std::unique_ptr<PopupWindow>> popups_;

    static constexpr UINT kPollTimerId = 100;
    static constexpr UINT kPollIntervalMs = 750;
};
