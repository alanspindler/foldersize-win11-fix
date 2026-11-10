#include "pipe_client.h"

#include <windows.h>

namespace {

constexpr wchar_t kPipeName[] = L"\\\\.\\pipe\\FolderSize";

HANDLE ConnectToPipe() {
    for (int attempt = 0; attempt < 2; ++attempt) {
        HANDLE pipe = CreateFileW(
            kPipeName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr);
        if (pipe != INVALID_HANDLE_VALUE) {
            return pipe;
        }
        if (GetLastError() == ERROR_PIPE_BUSY) {
            if (!WaitNamedPipeW(kPipeName, 1000)) {
                return INVALID_HANDLE_VALUE;
            }
        } else {
            return INVALID_HANDLE_VALUE;
        }
    }
    return INVALID_HANDLE_VALUE;
}

bool WriteAll(HANDLE pipe, const void* data, DWORD size) {
    DWORD written = 0;
    return WriteFile(pipe, data, size, &written, nullptr) && written == size;
}

bool ReadAll(HANDLE pipe, void* data, DWORD size) {
    DWORD read = 0;
    return ReadFile(pipe, data, size, &read, nullptr) && read == size;
}

bool WriteRequest(HANDLE pipe, PipeClientRequest request) {
    const unsigned short value = static_cast<unsigned short>(request);
    return WriteAll(pipe, &value, sizeof(value));
}

bool WriteString(HANDLE pipe, const std::wstring& text) {
  if (text.size() >= 0xFFFF) {
    return false;
  }
  const unsigned short len = static_cast<unsigned short>(text.size());
  if (!WriteAll(pipe, &len, sizeof(len))) {
    return false;
  }
  if (len == 0) {
    return true;
  }
  return WriteAll(pipe, text.data(), len * sizeof(wchar_t));
}

bool ReadGetFolderSizeReply(HANDLE pipe, FolderSizeReply& reply) {
    return ReadAll(pipe, &reply.result, sizeof(reply.result)) &&
           ReadAll(pipe, &reply.size, sizeof(reply.size));
}

}  // namespace

bool QueryFolderSize(const std::wstring& folderPath, FolderSizeReply& reply) {
    reply = {};
    const HANDLE pipe = ConnectToPipe();
    if (pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool ok = WriteRequest(pipe, PcrGetFolderSize) && WriteString(pipe, folderPath) &&
              ReadGetFolderSizeReply(pipe, reply);
    CloseHandle(pipe);
    return ok;
}
