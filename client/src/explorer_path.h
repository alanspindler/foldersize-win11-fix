#pragma once

#include <string>
#include <vector>
#include <windows.h>

struct ExplorerInstance {
    HWND hwnd = nullptr;
    std::wstring path;
};

// Returns normal file-explorer windows (HWND + current folder path).
std::vector<ExplorerInstance> EnumerateExplorerInstances();
