#include <vector>

#include "bundle.h"
#include "cancel.h"
#include "util.h"

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

bool RunHiddenProcess(const std::wstring& commandLine, DWORD& exitCode) {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> cmd(commandLine.begin(), commandLine.end());
    cmd.push_back(L'\0');
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si,
                        &pi)) {
        return false;
    }

    ChildProcessRegistration childProcess(pi.hProcess);
    while (WaitForSingleObject(pi.hProcess, 100) == WAIT_TIMEOUT) {
        if (IsCancelRequested()) {
            TerminateProcess(pi.hProcess, 1);
            break;
        }
    }

    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

bool DecompressGzipWith7za(const std::wstring& sevenZa, const std::wstring& gzPath, const std::wstring& outDir,
                           const std::wstring& outPath) {
    if (!FileExists(gzPath) || !FileExists(sevenZa) || outDir.empty()) {
        return false;
    }

    std::wstring cmd = L"\"" + sevenZa + L"\" e -y -o\"" + outDir + L"\" \"" + gzPath + L"\"";
    DWORD exitCode = 1;
    if (!RunHiddenProcess(cmd, exitCode) || exitCode != 0) {
        return false;
    }

    if (!FileExists(outPath)) {
        return false;
    }

    WIN32_FILE_ATTRIBUTE_DATA gzInfo{};
    if (GetFileAttributesExW(gzPath.c_str(), GetFileExInfoStandard, &gzInfo)) {
        HANDLE md5File = CreateFileW(outPath.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (md5File != INVALID_HANDLE_VALUE) {
            SetFileTime(md5File, nullptr, nullptr, &gzInfo.ftLastWriteTime);
            CloseHandle(md5File);
        }
    }

    return true;
}

bool EnsureMd5Manifest(HINSTANCE instance, const std::wstring& dir, const std::wstring& sevenZa,
                       std::wstring& md5Path) {
    const std::wstring gzPath = JoinPath(dir, L"MedicatFiles.md5.gz");
    md5Path = JoinPath(dir, L"MedicatFiles.md5");

    if (!EnsureExtracted(instance, IDR_MEDICAT_MD5, gzPath)) {
        return false;
    }

    if (FileExists(md5Path)) {
        WIN32_FILE_ATTRIBUTE_DATA gzInfo{};
        WIN32_FILE_ATTRIBUTE_DATA md5Info{};
        if (GetFileAttributesExW(gzPath.c_str(), GetFileExInfoStandard, &gzInfo) &&
            GetFileAttributesExW(md5Path.c_str(), GetFileExInfoStandard, &md5Info) &&
            CompareFileTime(&md5Info.ftLastWriteTime, &gzInfo.ftLastWriteTime) >= 0) {
            return true;
        }
    }

    DeleteFileW(md5Path.c_str());
    return DecompressGzipWith7za(sevenZa, gzPath, dir, md5Path);
}

bool EnsureAria2c(HINSTANCE instance, const std::wstring& dir, const std::wstring& sevenZa, std::wstring& aria2cPath) {
    const std::wstring gzPath = JoinPath(dir, L"aria2c.exe.gz");
    aria2cPath = JoinPath(dir, L"aria2c.exe");

    if (!EnsureExtracted(instance, IDR_ARIA2C, gzPath)) {
        aria2cPath.clear();
        return false;
    }

    if (FileExists(aria2cPath)) {
        WIN32_FILE_ATTRIBUTE_DATA gzInfo{};
        WIN32_FILE_ATTRIBUTE_DATA exeInfo{};
        if (GetFileAttributesExW(gzPath.c_str(), GetFileExInfoStandard, &gzInfo) &&
            GetFileAttributesExW(aria2cPath.c_str(), GetFileExInfoStandard, &exeInfo) &&
            CompareFileTime(&exeInfo.ftLastWriteTime, &gzInfo.ftLastWriteTime) >= 0) {
            return true;
        }
    }

    DeleteFileW(aria2cPath.c_str());
    if (!DecompressGzipWith7za(sevenZa, gzPath, dir, aria2cPath)) {
        aria2cPath.clear();
        return false;
    }
    return true;
}

}  // namespace

BundledTools EnsureBundledTools(const HINSTANCE instance) {
    BundledTools tools;
    tools.dir = GetMedicatTempDir();

    tools.sevenZa = JoinPath(tools.dir, L"7za.exe");

    if (!EnsureExtracted(instance, IDR_7ZA, tools.sevenZa)) {
        tools.error = L"Failed to extract bundled 7za.exe";
        return tools;
    }
    if (!EnsureMd5Manifest(instance, tools.dir, tools.sevenZa, tools.md5Manifest)) {
        tools.error = L"Failed to extract bundled MedicatFiles.md5";
        return tools;
    }

    EnsureAria2c(instance, tools.dir, tools.sevenZa, tools.aria2c);

    tools.ok = true;
    return tools;
}

}  // namespace medicat
