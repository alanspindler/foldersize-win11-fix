#include "explorer_tracker.h"

#include "explorer_path.h"

#include <shellapi.h>

namespace {

constexpr wchar_t kTrackerClassName[] = L"FolderSizeTrackerHost";

bool IsExplorerWindow(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }
    wchar_t className[64]{};
    if (GetClassNameW(hwnd, className, 64) == 0) {
        return false;
    }
    return lstrcmpiW(className, L"CabinetWClass") == 0;
}

}  // namespace

ExplorerTracker::ExplorerTracker(HINSTANCE instance) : instance_(instance) {}

ExplorerTracker::~ExplorerTracker() {
    Shutdown();
}

void ExplorerTracker::CreateMessageWindow() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MessageWndProc;
    wc.hInstance = instance_;
    wc.lpszClassName = kTrackerClassName;
    RegisterClassExW(&wc);

    messageHwnd_ = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        kTrackerClassName,
        L"FolderSizeTrackerHost",
        WS_POPUP,
        0,
        0,
        0,
        0,
        nullptr,
        nullptr,
        instance_,
        this);
}

bool ExplorerTracker::Initialize(HWND /*messageHwnd*/) {
    CreateMessageWindow();
    if (!messageHwnd_) {
        return false;
    }

    RegisterShellHookWindow(messageHwnd_);
    SetTimer(messageHwnd_, kPollTimerId, kPollIntervalMs, nullptr);
    SyncNow();
    return true;
}

void ExplorerTracker::Shutdown() {
    if (messageHwnd_) {
        KillTimer(messageHwnd_, kPollTimerId);
        DeregisterShellHookWindow(messageHwnd_);
        DestroyWindow(messageHwnd_);
        messageHwnd_ = nullptr;
    }
    popups_.clear();
    UnregisterClassW(kTrackerClassName, instance_);
}

void ExplorerTracker::SetPopupsEnabled(bool enabled) {
    popupsEnabled_ = enabled;
    if (!enabled) {
        for (auto& entry : popups_) {
            entry.second->Update(L"", false);
        }
        return;
    }
    SyncNow();
}

void ExplorerTracker::SyncNow() {
    ReconcilePopups();
}

void ExplorerTracker::ReconcilePopups() {
    const auto explorers = EnumerateExplorerInstances();

    std::map<HWND, ExplorerInstance> active;
    for (const ExplorerInstance& explorer : explorers) {
        active.emplace(explorer.hwnd, explorer);
    }

    for (auto it = popups_.begin(); it != popups_.end();) {
        if (!IsWindow(it->first) || active.find(it->first) == active.end()) {
            it = popups_.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto& [hwnd, explorer] : active) {
        auto it = popups_.find(hwnd);
        if (it == popups_.end()) {
            auto popup = std::make_unique<PopupWindow>();
            if (popup->Create(hwnd)) {
                popups_.emplace(hwnd, std::move(popup));
                it = popups_.find(hwnd);
            } else {
                continue;
            }
        }
        it->second->Update(explorer.path, popupsEnabled_);
    }
}

void ExplorerTracker::OnTimer() {
    ReconcilePopups();
    for (auto& [hwnd, popup] : popups_) {
        if (!IsWindow(hwnd)) {
            continue;
        }
        for (const ExplorerInstance& explorer : EnumerateExplorerInstances()) {
            if (explorer.hwnd == hwnd) {
                popup->Update(explorer.path, popupsEnabled_);
                break;
            }
        }
    }
}

LRESULT CALLBACK ExplorerTracker::MessageWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ExplorerTracker* tracker = reinterpret_cast<ExplorerTracker*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        tracker = static_cast<ExplorerTracker*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(tracker));
        return TRUE;
    }

    if (!tracker) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
        case WM_TIMER:
            if (wParam == kPollTimerId) {
                tracker->OnTimer();
            }
            return 0;
        case WM_DESTROY:
            DeregisterShellHookWindow(hwnd);
            return 0;
        default:
            if (msg >= WM_USER) {
                const UINT shellCode = msg - WM_USER;
                if (shellCode == HSHELL_WINDOWCREATED || shellCode == HSHELL_WINDOWDESTROYED ||
                    shellCode == HSHELL_WINDOWACTIVATED) {
                    tracker->SyncNow();
                    return 0;
                }
            }
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
