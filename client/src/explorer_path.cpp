#include "explorer_path.h"

#include <ole2.h>
#include <oleauto.h>
#include <exdisp.h>
#include <shlobj.h>
#include <shlguid.h>
#include <shobjidl.h>
#include <shlwapi.h>

namespace {

bool IsExplorerTopLevelWindow(const HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }

    wchar_t className[64]{};
    if (GetClassNameW(hwnd, className, 64) == 0) {
        return false;
    }
    if (lstrcmpiW(className, L"CabinetWClass") != 0) {
        return false;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) {
        return false;
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) {
        return false;
    }
    wchar_t imagePath[MAX_PATH]{};
    DWORD imageSize = MAX_PATH;
    const BOOL gotName = QueryFullProcessImageNameW(process, 0, imagePath, &imageSize);
    CloseHandle(process);
    return gotName && lstrcmpiW(PathFindFileNameW(imagePath), L"explorer.exe") == 0;
}

bool PathFromShellBrowser(IShellBrowser* shellBrowser, std::wstring& pathOut) {
    pathOut.clear();
    if (!shellBrowser) {
        return false;
    }

    IShellView* shellView = nullptr;
    if (FAILED(shellBrowser->QueryActiveShellView(&shellView)) || !shellView) {
        return false;
    }

    IFolderView* folderView = nullptr;
    if (SUCCEEDED(shellView->QueryInterface(IID_IFolderView, (void**)&folderView)) && folderView) {
        IShellFolder2* shellFolder = nullptr;
        if (SUCCEEDED(folderView->GetFolder(IID_IShellFolder2, (void**)&shellFolder)) && shellFolder) {
            IPersistFolder2* persistFolder = nullptr;
            if (SUCCEEDED(shellFolder->QueryInterface(IID_IPersistFolder2, (void**)&persistFolder)) && persistFolder) {
                LPITEMIDLIST pidl = nullptr;
                if (SUCCEEDED(persistFolder->GetCurFolder(&pidl)) && pidl) {
                    wchar_t pathBuffer[MAX_PATH]{};
                    if (SHGetPathFromIDListW(pidl, pathBuffer) && pathBuffer[0] != L'\0') {
                        pathOut = pathBuffer;
                    }
                    CoTaskMemFree(pidl);
                }
                persistFolder->Release();
            }
            shellFolder->Release();
        }
        folderView->Release();
    }

    shellView->Release();
    return true;
}

bool HwndAlreadyListed(const std::vector<ExplorerInstance>& explorers, const HWND hwnd) {
    for (const ExplorerInstance& explorer : explorers) {
        if (explorer.hwnd == hwnd) {
            return true;
        }
    }
    return false;
}

bool TryAddExplorerWindow(
    IDispatch* dispatch,
    std::vector<ExplorerInstance>& explorers) {
    if (!dispatch) {
        return false;
    }

    HWND explorerHwnd = nullptr;
    std::wstring folderPath;

    IWebBrowser2* browser = nullptr;
    if (SUCCEEDED(dispatch->QueryInterface(IID_IWebBrowser2, (void**)&browser)) && browser) {
        SHANDLE_PTR handle = 0;
        if (SUCCEEDED(browser->get_HWND(&handle))) {
            explorerHwnd = reinterpret_cast<HWND>(handle);
        }
        browser->Release();
    }

    IServiceProvider* provider = nullptr;
    if (SUCCEEDED(dispatch->QueryInterface(IID_IServiceProvider, (void**)&provider)) && provider) {
        IShellBrowser* shellBrowser = nullptr;
        if (SUCCEEDED(provider->QueryService(SID_STopLevelBrowser, IID_IShellBrowser, (void**)&shellBrowser)) &&
            shellBrowser) {
            if (!explorerHwnd) {
                shellBrowser->GetWindow(&explorerHwnd);
            }
            PathFromShellBrowser(shellBrowser, folderPath);
            shellBrowser->Release();
        }
        provider->Release();
    }

    if (explorerHwnd) {
        HWND ancestor = GetAncestor(explorerHwnd, GA_ROOT);
        if (ancestor) {
            explorerHwnd = ancestor;
        }
    }

    if (!explorerHwnd || HwndAlreadyListed(explorers, explorerHwnd)) {
        return false;
    }
    if (!IsExplorerTopLevelWindow(explorerHwnd)) {
        return false;
    }

    ExplorerInstance instance;
    instance.hwnd = explorerHwnd;
    instance.path = std::move(folderPath);
    explorers.push_back(std::move(instance));
    return true;
}

}  // namespace

std::vector<ExplorerInstance> EnumerateExplorerInstances() {
    std::vector<ExplorerInstance> explorers;

    IShellWindows* shellWindows = nullptr;
    if (FAILED(CoCreateInstance(
            CLSID_ShellWindows, nullptr, CLSCTX_ALL, IID_IShellWindows, (void**)&shellWindows))) {
        return explorers;
    }

    IUnknown* unknownEnum = nullptr;
    if (SUCCEEDED(shellWindows->_NewEnum(&unknownEnum)) && unknownEnum) {
        IEnumVARIANT* enumVariant = nullptr;
        if (SUCCEEDED(unknownEnum->QueryInterface(IID_IEnumVARIANT, (void**)&enumVariant)) && enumVariant) {
            VARIANT variant;
            VariantInit(&variant);
            while (enumVariant->Next(1, &variant, nullptr) == S_OK) {
                if (V_VT(&variant) == VT_DISPATCH && variant.pdispVal != nullptr) {
                    TryAddExplorerWindow(variant.pdispVal, explorers);
                }
                VariantClear(&variant);
            }
            enumVariant->Release();
        }
        unknownEnum->Release();
    } else {
        long count = 0;
        shellWindows->get_Count(&count);
        VARIANT index{};
        VariantInit(&index);
        index.vt = VT_I4;
        for (long i = 0; i < count; ++i) {
            index.lVal = i;
            IDispatch* dispatch = nullptr;
            if (SUCCEEDED(shellWindows->Item(index, &dispatch)) && dispatch) {
                TryAddExplorerWindow(dispatch, explorers);
                dispatch->Release();
            }
        }
    }

    shellWindows->Release();
    return explorers;
}
