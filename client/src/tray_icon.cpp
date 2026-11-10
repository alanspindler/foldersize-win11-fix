#include "tray_icon.h"

#include "explorer_tracker.h"

namespace {

constexpr wchar_t kTrayClassName[] = L"FolderSizeTrayHost";
constexpr UINT kTrayCallbackMessage = WM_APP + 20;
constexpr UINT kCmdShowPopups = 1001;
constexpr UINT kCmdStartup = 1002;
constexpr UINT kCmdExit = 1003;
constexpr wchar_t kRunKeyPath[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kRunValueName[] = L"FolderSize";

std::wstring CurrentModulePath() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return path;
}

}  // namespace

TrayIcon::TrayIcon(HINSTANCE instance, ExplorerTracker& tracker)
    : instance_(instance), tracker_(tracker) {}

TrayIcon::~TrayIcon() {
    Destroy();
}

void TrayIcon::CreateHiddenWindow() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance_;
    wc.lpszClassName = kTrayClassName;
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(0, kTrayClassName, L"FolderSizeTrayHost", 0, 0, 0, 0, 0, nullptr, nullptr, instance_, this);
}

bool TrayIcon::Create() {
    CreateHiddenWindow();
    if (!hwnd_) {
        return false;
    }

    menu_ = CreatePopupMenu();
    AppendMenuW(menu_, MF_STRING | MF_CHECKED, kCmdShowPopups, L"Show Folder Size popups");
    MENUITEMINFOW startupInfo{sizeof(startupInfo)};
    startupInfo.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE;
    startupInfo.wID = kCmdStartup;
    startupInfo.fState = StartupEnabled() ? MFS_CHECKED : MFS_UNCHECKED;
    startupInfo.dwTypeData = const_cast<LPWSTR>(L"Start on Windows startup");
    InsertMenuItemW(menu_, 1, TRUE, &startupInfo);
    AppendMenuW(menu_, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu_, MF_STRING, kCmdExit, L"E&xit");

    iconData_.cbSize = sizeof(iconData_);
    iconData_.hWnd = hwnd_;
    iconData_.uID = 1;
    iconData_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    iconData_.uCallbackMessage = kTrayCallbackMessage;
    iconData_.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    lstrcpynW(iconData_.szTip, L"Folder Size", ARRAYSIZE(iconData_.szTip));
    return Shell_NotifyIconW(NIM_ADD, &iconData_) != FALSE;
}

void TrayIcon::Destroy() {
    if (iconData_.hWnd) {
        Shell_NotifyIconW(NIM_DELETE, &iconData_);
        iconData_.hWnd = nullptr;
    }
    if (menu_) {
        DestroyMenu(menu_);
        menu_ = nullptr;
    }
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    UnregisterClassW(kTrayClassName, instance_);
}

bool TrayIcon::StartupEnabled() const {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return false;
    }
    wchar_t buffer[MAX_PATH]{};
    DWORD size = sizeof(buffer);
    const LONG status = RegQueryValueExW(key, kRunValueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer), &size);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

void TrayIcon::UpdateStartupRegistry(bool enable) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
        return;
    }

    if (enable) {
        const std::wstring path = CurrentModulePath();
        RegSetValueExW(
            key,
            kRunValueName,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(path.c_str()),
            static_cast<DWORD>((path.size() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(key, kRunValueName);
    }
    RegCloseKey(key);
}

void TrayIcon::ShowContextMenu() {
    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(hwnd_);

    CheckMenuItem(
        menu_,
        kCmdShowPopups,
        MF_BYCOMMAND | (tracker_.PopupsEnabled() ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu_, kCmdStartup, MF_BYCOMMAND | (StartupEnabled() ? MF_CHECKED : MF_UNCHECKED));

    const UINT command = TrackPopupMenu(
        menu_,
        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
        cursor.x,
        cursor.y,
        0,
        hwnd_,
        nullptr);
    PostMessageW(hwnd_, WM_NULL, 0, 0);

    switch (command) {
        case kCmdShowPopups:
            tracker_.SetPopupsEnabled(!tracker_.PopupsEnabled());
            break;
        case kCmdStartup:
            UpdateStartupRegistry(!StartupEnabled());
            break;
        case kCmdExit:
            PostQuitMessage(0);
            break;
        default:
            break;
    }
}

LRESULT CALLBACK TrayIcon::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    TrayIcon* self = reinterpret_cast<TrayIcon*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        self = static_cast<TrayIcon*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return TRUE;
    }

    if (!self) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
        case kTrayCallbackMessage:
            if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_CONTEXTMENU) {
                self->ShowContextMenu();
            } else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
                self->tracker_.SetPopupsEnabled(true);
            }
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}
