#pragma once

#include <windows.h>

#include <string>

enum PipeClientRequest : unsigned short {
    PcrGetFolderSize = 0,
    PcrGetUpdatedFolders = 1,
};

struct FolderSizeReply {
    DWORD result = 0;
    ULONGLONG size = 0;
};

bool QueryFolderSize(const std::wstring& folderPath, FolderSizeReply& reply);
