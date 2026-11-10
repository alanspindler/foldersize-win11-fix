#include "popup_window.h"

#include "pipe_client.h"

#include <commctrl.h>
#include <shlwapi.h>
#include <algorithm>
#include <vector>

namespace {

constexpr wchar_t kPopupClassName[] = L"FolderSizeShellWindow";

}  // namespace

void PopupWindow::RegisterWindowClass(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = PopupWindow::WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = kPopupClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);
}

void PopupWindow::UnregisterWindowClass(HINSTANCE instance) {
    ::UnregisterClassW(kPopupClassName, instance);
}

PopupWindow::~PopupWindow() {
    Destroy();
}

bool PopupWindow::Create(HWND explorerHwnd) {
    explorerHwnd_ = explorerHwnd;
    hwnd_ = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        kPopupClassName,
        L"Folder Size",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        420,
        380,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        this);
    if (!hwnd_) {
        return false;
    }

    listView_ = CreateWindowExW(
        0,
        WC_LISTVIEWW,
        L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
        0,
        0,
        100,
        100,
        hwnd_,
        reinterpret_cast<HMENU>(1),
        GetModuleHandle(nullptr),
        nullptr);
    ListView_SetExtendedListViewStyle(listView_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    LVCOLUMNW nameColumn{};
    nameColumn.mask = LVCF_TEXT | LVCF_WIDTH;
    nameColumn.pszText = const_cast<LPWSTR>(L"Name");
    nameColumn.cx = 240;
    ListView_InsertColumn(listView_, 0, &nameColumn);

    LVCOLUMNW sizeColumn{};
    sizeColumn.mask = LVCF_TEXT | LVCF_WIDTH;
    sizeColumn.pszText = const_cast<LPWSTR>(L"Size");
    sizeColumn.cx = 130;
    ListView_InsertColumn(listView_, 1, &sizeColumn);

    SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetTimer(hwnd_, kRefreshTimerId, 2000, nullptr);
    return true;
}

void PopupWindow::Destroy() {
    if (hwnd_) {
        KillTimer(hwnd_, kRefreshTimerId);
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
        listView_ = nullptr;
    }
    explorerHwnd_ = nullptr;
    folderPath_.clear();
}

void PopupWindow::Update(const std::wstring& folderPath, bool visible) {
    if (!hwnd_) {
        return;
    }

    if (!visible) {
        ShowWindow(hwnd_, SW_HIDE);
        return;
    }

    if (folderPath_ != folderPath) {
        folderPath_ = folderPath;
        RefreshContents();
    }

    PositionNearExplorer();
    ShowWindow(hwnd_, SW_SHOWNA);
}

void PopupWindow::PositionNearExplorer() {
    if (!explorerHwnd_ || !hwnd_) {
        return;
    }
    RECT explorerRect{};
    if (!GetWindowRect(explorerHwnd_, &explorerRect)) {
        return;
    }

    RECT popupRect{};
    GetWindowRect(hwnd_, &popupRect);
    const int width = popupRect.right - popupRect.left;
    const int height = popupRect.bottom - popupRect.top;

    const int x = explorerRect.right + 8;
    int y = explorerRect.top;

    const HMONITOR monitor = MonitorFromWindow(explorerHwnd_, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    if (GetMonitorInfoW(monitor, &monitorInfo)) {
        if (x + width > monitorInfo.rcWork.right) {
            const int leftPlacement = explorerRect.left - width - 8;
            SetWindowPos(hwnd_, HWND_TOPMOST, leftPlacement, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            return;
        }
        if (y + height > monitorInfo.rcWork.bottom) {
            y = monitorInfo.rcWork.bottom - height;
        }
        if (y < monitorInfo.rcWork.top) {
            y = monitorInfo.rcWork.top;
        }
    }

    SetWindowPos(hwnd_, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
}

void PopupWindow::RefreshContents() {
    if (!listView_) {
        return;
    }

    if (folderPath_.empty()) {
        ListView_DeleteAllItems(listView_);
        LVITEMW placeholder{};
        placeholder.mask = LVIF_TEXT;
        placeholder.iItem = 0;
        placeholder.pszText = const_cast<LPWSTR>(
            L"This folder has no filesystem path (e.g. Home). Open a disk folder to list sizes.");
        ListView_InsertItem(listView_, &placeholder);
        return;
    }

    ListView_DeleteAllItems(listView_);

    std::wstring searchPath = folderPath_;
    if (!searchPath.empty() && searchPath.back() != L'\\') {
        searchPath.push_back(L'\\');
    }
    searchPath.append(L"*");

    WIN32_FIND_DATAW findData{};
    const HANDLE findHandle = FindFirstFileW(searchPath.c_str(), &findData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    struct Row {
        std::wstring name;
        std::wstring displaySize;
        ULONGLONG sortKey = 0;
        bool isDir = false;
    };

    std::vector<Row> rows;
    do {
        if (lstrcmpW(findData.cFileName, L".") == 0 || lstrcmpW(findData.cFileName, L"..") == 0) {
            continue;
        }

        const bool isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        Row row;
        row.name = findData.cFileName;
        row.isDir = isDir;

        if (isDir) {
            std::wstring childPath = folderPath_;
            if (!childPath.empty() && childPath.back() != L'\\') {
                childPath.push_back(L'\\');
            }
            childPath.append(findData.cFileName);

            FolderSizeReply reply{};
            if (QueryFolderSize(childPath, reply)) {
                row.sortKey = reply.size;
                wchar_t formatted[64]{};
                StrFormatByteSizeW(static_cast<LONGLONG>(reply.size), formatted, 64);
                row.displaySize = formatted;
                if (reply.result & 2) {  // GFS_DIRTY
                    row.displaySize.append(L"+");
                }
            } else {
                row.displaySize = L"...";
            }
        } else {
            ULARGE_INTEGER fileSize{};
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            row.sortKey = fileSize.QuadPart;
            wchar_t formatted[64]{};
            StrFormatByteSizeW(static_cast<LONGLONG>(fileSize.QuadPart), formatted, 64);
            row.displaySize = formatted;
        }

        rows.push_back(std::move(row));
    } while (FindNextFileW(findHandle, &findData));
    FindClose(findHandle);

    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        if (a.isDir != b.isDir) {
            return a.isDir > b.isDir;
        }
        return a.sortKey > b.sortKey;
    });

    int itemIndex = 0;
    for (const Row& row : rows) {
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = itemIndex;
        item.pszText = const_cast<LPWSTR>(row.name.c_str());
        ListView_InsertItem(listView_, &item);
        ListView_SetItemText(listView_, itemIndex, 1, const_cast<LPWSTR>(row.displaySize.c_str()));
        ++itemIndex;
    }

    if (hwnd_) {
        RECT client{};
        GetClientRect(hwnd_, &client);
        SetWindowPos(listView_, nullptr, 0, 0, client.right, client.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

LRESULT CALLBACK PopupWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PopupWindow* self = reinterpret_cast<PopupWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        self = static_cast<PopupWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    if (!self) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
        case WM_SIZE:
            if (self->listView_) {
                RECT client{};
                GetClientRect(hwnd, &client);
                SetWindowPos(
                    self->listView_,
                    nullptr,
                    0,
                    0,
                    client.right,
                    client.bottom,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;
        case WM_TIMER:
            if (wParam == kRefreshTimerId && !self->folderPath_.empty()) {
                self->RefreshContents();
            }
            return 0;
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        case WM_DESTROY:
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}
