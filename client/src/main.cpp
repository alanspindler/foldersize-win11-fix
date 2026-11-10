#include "explorer_tracker.h"
#include "popup_window.h"
#include "tray_icon.h"

#include <commctrl.h>
#include <shellapi.h>

namespace {

constexpr wchar_t kSingleInstanceMutex[] = L"Local\\BrioFolderSizeMutex";

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    const HANDLE mutex = CreateMutexW(nullptr, TRUE, kSingleInstanceMutex);
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(
            nullptr,
            L"Folder Size is already running.",
            L"Folder Size",
            MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES};
    InitCommonControlsEx(&controls);

    HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comResult)) {
        MessageBoxW(
            nullptr,
            L"Failed to initialize COM.",
            L"Folder Size",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    PopupWindow::RegisterWindowClass(instance);

    ExplorerTracker tracker(instance);
    if (!tracker.Initialize(nullptr)) {
        MessageBoxW(
            nullptr,
            L"Failed to initialize Explorer tracking.",
            L"Folder Size",
            MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    TrayIcon tray(instance, tracker);
    if (!tray.Create()) {
        MessageBoxW(
            nullptr,
            L"Failed to create tray icon.",
            L"Folder Size",
            MB_OK | MB_ICONERROR);
        PopupWindow::UnregisterWindowClass(instance);
        CoUninitialize();
        return 1;
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    tray.Destroy();
    tracker.Shutdown();
    PopupWindow::UnregisterWindowClass(instance);
    CoUninitialize();
    if (mutex) {
        CloseHandle(mutex);
    }
    return static_cast<int>(message.wParam);
}
