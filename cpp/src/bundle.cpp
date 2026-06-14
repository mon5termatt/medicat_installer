#include "bundle.h"

#include "util.h"

#include <vector>

namespace medicat {

namespace {

bool WriteFileBytes(const std::wstring& path, const void* data, DWORD size) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    const BOOL ok = WriteFile(file, data, size, &written, nullptr) && written == size;
    CloseHandle(file);
    return ok != FALSE;
}

bool ExtractResourceToFile(HINSTANCE instance, int resourceId, const std::wstring& outPath) {
    const HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    if (!resource) {
        return false;
    }

    const HGLOBAL loaded = LoadResource(instance, resource);
    if (!loaded) {
        return false;
    }

    const DWORD size = SizeofResource(instance, resource);
    const void* data = LockResource(loaded);
    if (!data || size == 0) {
        return false;
    }

    return WriteFileBytes(outPath, data, size);
}

DWORD EmbeddedResourceSize(HINSTANCE instance, int resourceId) {
    const HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    if (!resource) {
        return 0;
    }
    return SizeofResource(instance, resource);
}

DWORD FileSizeOnDisk(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA info{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &info)) {
        return 0;
    }
    ULARGE_INTEGER size;
    size.LowPart = info.nFileSizeLow;
    size.HighPart = info.nFileSizeHigh;
    if (size.QuadPart > MAXDWORD) {
        return MAXDWORD;
    }
    return size.LowPart;
}

bool EnsureExtracted(HINSTANCE instance, int resourceId, const std::wstring& outPath) {
    const DWORD embeddedSize = EmbeddedResourceSize(instance, resourceId);
    if (embeddedSize == 0) {
        return false;
    }

    if (FileExists(outPath) && FileSizeOnDisk(outPath) == embeddedSize) {
        return true;
    }

    return ExtractResourceToFile(instance, resourceId, outPath);
}

}  // namespace

BundledTools EnsureBundledTools(const HINSTANCE instance) {
    BundledTools tools;
    tools.dir = GetMedicatTempDir();

    tools.sevenZa = JoinPath(tools.dir, L"7za.exe");
    tools.sevenZ = JoinPath(tools.dir, L"7z.exe");

    if (!EnsureExtracted(instance, IDR_7ZA, tools.sevenZa)) {
        tools.error = L"Failed to extract bundled 7za.exe";
        return tools;
    }
    if (!EnsureExtracted(instance, IDR_7Z, tools.sevenZ)) {
        tools.error = L"Failed to extract bundled 7z.exe";
        return tools;
    }

    tools.ok = true;
    return tools;
}

}  // namespace medicat
