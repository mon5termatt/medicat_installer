#include "drives.h"

#include "cancel.h"
#include "i18n.h"
#include "util.h"

#include <cfgmgr32.h>
#include <initguid.h>
#include <setupapi.h>
#include <windows.h>
#include <winioctl.h>

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <vector>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

namespace medicat {

namespace {

std::wstring QueryVolumeLabel(const std::wstring& root) {
    wchar_t label[MAX_PATH + 1]{};
    wchar_t fs[MAX_PATH + 1]{};
    DWORD serial = 0;
    DWORD maxComp = 0;
    DWORD flags = 0;
    if (!GetVolumeInformationW(root.c_str(), label, MAX_PATH, &serial, &maxComp, &flags, fs, MAX_PATH)) {
        return L"";
    }
    return label;
}

ULARGE_INTEGER QueryFreeBytes(const std::wstring& root) {
    ULARGE_INTEGER freeBytesAvailable{};
    ULARGE_INTEGER totalBytes{};
    ULARGE_INTEGER totalFree{};
    if (GetDiskFreeSpaceExW(root.c_str(), &freeBytesAvailable, &totalBytes, &totalFree)) {
        return freeBytesAvailable;
    }
    ULARGE_INTEGER zero{};
    return zero;
}

std::wstring SanitizeDescriptorAscii(const char* raw, const DWORD bufferSize, const DWORD offset) {
    if (raw == nullptr || offset == 0 || offset >= bufferSize) {
        return L"";
    }
    std::wstring out;
    for (DWORD i = offset; i < bufferSize; ++i) {
        const unsigned char ch = static_cast<unsigned char>(raw[i]);
        if (ch == 0) {
            break;
        }
        if (ch >= 32 && ch < 127) {
            out.push_back(static_cast<wchar_t>(ch));
        } else if (ch == '\t' || ch == '\r' || ch == '\n') {
            out.push_back(L' ');
        }
    }
    while (!out.empty() && out.front() == L' ') {
        out.erase(out.begin());
    }
    while (!out.empty() && out.back() == L' ') {
        out.pop_back();
    }
    for (wchar_t& ch : out) {
        if (ch == L'"') {
            ch = L'\'';
        }
    }
    return out;
}

std::wstring SanitizeLogField(std::wstring value) {
    while (!value.empty() && (value.front() == L' ' || value.front() == L'\t')) {
        value.erase(value.begin());
    }
    while (!value.empty() && (value.back() == L' ' || value.back() == L'\t')) {
        value.pop_back();
    }
    for (wchar_t& ch : value) {
        if (ch == L'"') {
            ch = L'\'';
        } else if (ch == L'\r' || ch == L'\n' || ch == L'\t') {
            ch = L' ';
        }
    }
    return value;
}

std::wstring UnderscoresToSpaces(std::wstring value) {
    for (wchar_t& ch : value) {
        if (ch == L'_') {
            ch = L' ';
        }
    }
    return SanitizeLogField(std::move(value));
}

bool LooksGenericStorageToken(const std::wstring& value) {
    if (value.empty()) {
        return true;
    }
    std::wstring lower = value;
    for (wchar_t& ch : lower) {
        ch = static_cast<wchar_t>(towlower(ch));
    }
    return lower == L"usb" || lower == L"generic" || lower == L"vendor" || lower == L"product" ||
           lower == L"unknown" || lower == L"usb device" || lower == L"disk" || lower == L"flash disk" ||
           lower == L"usb disk" || lower == L"mass storage" || lower == L"usb mass storage device";
}

bool ExtractTaggedToken(const std::wstring& text, const wchar_t* tag, const size_t hexLen, std::wstring& out) {
    if (!tag || hexLen == 0 || text.empty()) {
        return false;
    }
    const size_t tagLen = wcslen(tag);
    for (size_t i = 0; i + tagLen + hexLen <= text.size(); ++i) {
        bool match = true;
        for (size_t j = 0; j < tagLen; ++j) {
            if (towupper(text[i + j]) != towupper(tag[j])) {
                match = false;
                break;
            }
        }
        if (!match) {
            continue;
        }
        std::wstring candidate = text.substr(i + tagLen, hexLen);
        bool allHex = true;
        for (wchar_t& ch : candidate) {
            if (!iswxdigit(ch)) {
                allHex = false;
                break;
            }
            ch = static_cast<wchar_t>(towupper(ch));
        }
        if (!allHex) {
            continue;
        }
        out = std::move(candidate);
        return true;
    }
    return false;
}

bool ExtractUsbStorField(const std::wstring& text, const wchar_t* tag, std::wstring& out) {
    if (!tag || text.empty()) {
        return false;
    }
    const size_t tagLen = wcslen(tag);
    for (size_t i = 0; i + tagLen < text.size(); ++i) {
        bool match = true;
        for (size_t j = 0; j < tagLen; ++j) {
            if (towupper(text[i + j]) != towupper(tag[j])) {
                match = false;
                break;
            }
        }
        if (!match) {
            continue;
        }
        size_t end = i + tagLen;
        while (end < text.size()) {
            const wchar_t ch = text[end];
            if (ch == L'&' || ch == L'\\' || ch == L'#' || ch == L' ' || ch == L'\0') {
                break;
            }
            ++end;
        }
        out = UnderscoresToSpaces(text.substr(i + tagLen, end - (i + tagLen)));
        return !out.empty();
    }
    return false;
}

std::wstring ReadDeviceRegistryProperty(HDEVINFO devs, SP_DEVINFO_DATA& devInfo, const DWORD property) {
    DWORD required = 0;
    DWORD regType = 0;
    SetupDiGetDeviceRegistryPropertyW(devs, &devInfo, property, &regType, nullptr, 0, &required);
    if (required == 0) {
        return L"";
    }
    std::vector<BYTE> buffer(required + sizeof(wchar_t), 0);
    if (!SetupDiGetDeviceRegistryPropertyW(devs, &devInfo, property, &regType, buffer.data(),
                                           static_cast<DWORD>(buffer.size()), &required)) {
        return L"";
    }
    if (regType == REG_SZ || regType == REG_EXPAND_SZ) {
        return SanitizeLogField(reinterpret_cast<wchar_t*>(buffer.data()));
    }
    if (regType == REG_MULTI_SZ) {
        // Prefer the first non-empty string (usually the most specific hardware ID).
        const wchar_t* cursor = reinterpret_cast<const wchar_t*>(buffer.data());
        while (cursor && *cursor) {
            const std::wstring value = SanitizeLogField(cursor);
            if (!value.empty()) {
                return value;
            }
            cursor += wcslen(cursor) + 1;
        }
    }
    return L"";
}

std::vector<std::wstring> ReadDeviceHardwareIds(HDEVINFO devs, SP_DEVINFO_DATA& devInfo) {
    std::vector<std::wstring> ids;
    DWORD required = 0;
    DWORD regType = 0;
    SetupDiGetDeviceRegistryPropertyW(devs, &devInfo, SPDRP_HARDWAREID, &regType, nullptr, 0, &required);
    if (required == 0) {
        return ids;
    }
    std::vector<BYTE> buffer(required + sizeof(wchar_t), 0);
    if (!SetupDiGetDeviceRegistryPropertyW(devs, &devInfo, SPDRP_HARDWAREID, &regType, buffer.data(),
                                           static_cast<DWORD>(buffer.size()), &required)) {
        return ids;
    }
    if (regType != REG_MULTI_SZ) {
        return ids;
    }
    const wchar_t* cursor = reinterpret_cast<const wchar_t*>(buffer.data());
    while (cursor && *cursor) {
        std::wstring value = SanitizeLogField(cursor);
        if (!value.empty()) {
            ids.push_back(std::move(value));
        }
        cursor += wcslen(cursor) + 1;
    }
    return ids;
}

void ApplyHardwareIdHints(DiskDeviceInfo& info, const std::wstring& hardwareId) {
    if (info.usbVid.empty()) {
        ExtractTaggedToken(hardwareId, L"VID_", 4, info.usbVid);
    }
    if (info.usbPid.empty()) {
        ExtractTaggedToken(hardwareId, L"PID_", 4, info.usbPid);
    }

    std::wstring ven;
    std::wstring prod;
    std::wstring rev;
    ExtractUsbStorField(hardwareId, L"Ven_", ven);
    ExtractUsbStorField(hardwareId, L"Prod_", prod);
    ExtractUsbStorField(hardwareId, L"Rev_", rev);
    if ((info.vendor.empty() || LooksGenericStorageToken(info.vendor)) && !ven.empty() &&
        !LooksGenericStorageToken(ven)) {
        info.vendor = ven;
    }
    if ((info.product.empty() || LooksGenericStorageToken(info.product)) && !prod.empty() &&
        !LooksGenericStorageToken(prod)) {
        info.product = prod;
    }
    if (info.revision.empty() && !rev.empty()) {
        info.revision = rev;
    }
}

void CollectUsbIdsFromDeviceTree(DEVINST devInst, DiskDeviceInfo& info) {
    DEVINST current = devInst;
    for (int depth = 0; depth < 10; ++depth) {
        WCHAR deviceId[MAX_DEVICE_ID_LEN]{};
        if (CM_Get_Device_IDW(current, deviceId, MAX_DEVICE_ID_LEN, 0) == CR_SUCCESS) {
            ApplyHardwareIdHints(info, deviceId);
            if (!info.usbVid.empty() && !info.usbPid.empty()) {
                return;
            }
        }
        DEVINST parent = 0;
        if (CM_Get_Parent(&parent, current, 0) != CR_SUCCESS) {
            break;
        }
        current = parent;
    }
}

void FillVendorProductFallbacks(DiskDeviceInfo& info) {
    if ((info.vendor.empty() || LooksGenericStorageToken(info.vendor)) && !info.manufacturer.empty() &&
        !LooksGenericStorageToken(info.manufacturer)) {
        info.vendor = info.manufacturer;
    }

    if ((info.product.empty() || LooksGenericStorageToken(info.product)) && !info.friendlyName.empty()) {
        std::wstring product = info.friendlyName;
        const std::wstring suffixes[] = {L" USB Device", L" USB Disk", L" Device"};
        for (const std::wstring& suffix : suffixes) {
            if (product.size() > suffix.size()) {
                const std::wstring tail = product.substr(product.size() - suffix.size());
                std::wstring lowerTail = tail;
                std::wstring lowerSuffix = suffix;
                for (wchar_t& ch : lowerTail) {
                    ch = static_cast<wchar_t>(towlower(ch));
                }
                for (wchar_t& ch : lowerSuffix) {
                    ch = static_cast<wchar_t>(towlower(ch));
                }
                if (lowerTail == lowerSuffix) {
                    product.resize(product.size() - suffix.size());
                    break;
                }
            }
        }
        product = SanitizeLogField(std::move(product));
        if (!product.empty() && !LooksGenericStorageToken(product)) {
            // Avoid duplicating vendor prefix when friendly is "SanDisk Cruzer".
            if (!info.vendor.empty()) {
                const std::wstring prefix = info.vendor + L" ";
                if (product.size() > prefix.size()) {
                    bool same = true;
                    for (size_t i = 0; i < prefix.size(); ++i) {
                        if (towlower(product[i]) != towlower(prefix[i])) {
                            same = false;
                            break;
                        }
                    }
                    if (same) {
                        product = SanitizeLogField(product.substr(prefix.size()));
                    }
                }
            }
            if (!product.empty()) {
                info.product = product;
            }
        }
    }
}

void EnrichDiskDeviceInfoFromSetupApi(DiskDeviceInfo& info) {
    HDEVINFO devs = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_DISK, nullptr, nullptr,
                                         DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devs == INVALID_HANDLE_VALUE) {
        return;
    }

    SP_DEVICE_INTERFACE_DATA ifc{};
    ifc.cbSize = sizeof(ifc);
    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(devs, nullptr, &GUID_DEVINTERFACE_DISK, index, &ifc);
         ++index) {
        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailW(devs, &ifc, nullptr, 0, &required, nullptr);
        if (required == 0) {
            continue;
        }

        std::vector<BYTE> detailBuf(required, 0);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(detailBuf.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        SP_DEVINFO_DATA devInfo{};
        devInfo.cbSize = sizeof(devInfo);
        if (!SetupDiGetDeviceInterfaceDetailW(devs, &ifc, detail, required, nullptr, &devInfo)) {
            continue;
        }

        const HANDLE disk =
            CreateFileW(detail->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (disk == INVALID_HANDLE_VALUE) {
            continue;
        }

        STORAGE_DEVICE_NUMBER number{};
        DWORD returned = 0;
        const BOOL ok = DeviceIoControl(disk, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0, &number, sizeof(number),
                                        &returned, nullptr);
        CloseHandle(disk);
        if (!ok || returned < sizeof(number) || number.DeviceNumber != info.diskNumber) {
            continue;
        }

        info.friendlyName = ReadDeviceRegistryProperty(devs, devInfo, SPDRP_FRIENDLYNAME);
        info.manufacturer = ReadDeviceRegistryProperty(devs, devInfo, SPDRP_MFG);
        const std::vector<std::wstring> hardwareIds = ReadDeviceHardwareIds(devs, devInfo);
        for (const std::wstring& hardwareId : hardwareIds) {
            ApplyHardwareIdHints(info, hardwareId);
        }
        CollectUsbIdsFromDeviceTree(devInfo.DevInst, info);
        FillVendorProductFallbacks(info);
        break;
    }

    SetupDiDestroyDeviceInfoList(devs);
}

const wchar_t* BusTypeDisplayName(const STORAGE_BUS_TYPE busType) {
    switch (busType) {
        case BusTypeUsb:
            return L"USB";
        case BusTypeSata:
            return L"SATA";
        case BusTypeAta:
            return L"ATA";
        case BusTypeScsi:
            return L"SCSI";
        case BusTypeSd:
            return L"SD";
        case BusTypeMmc:
            return L"MMC";
        case BusTypeFileBackedVirtual:
            return L"VHD";
        case BusTypeVirtual:
            return L"Virtual";
        case BusTypeSas:
            return L"SAS";
        case BusTypeNvme:
            return L"NVMe";
        case BusTypeSpaces:
            return L"Spaces";
        case BusTypeRAID:
            return L"RAID";
        default:
            return L"Unknown";
    }
}

bool IsFileBackedVirtualDisk(const DWORD diskNumber) {
    const DiskDeviceInfo info = QueryDiskDeviceInfo(diskNumber);
    return info.valid && info.busTypeName == L"VHD";
}

bool IsUsbBusDisk(const DWORD diskNumber) {
    const DiskDeviceInfo info = QueryDiskDeviceInfo(diskNumber);
    if (info.valid) {
        if (info.busTypeName == L"USB" || info.busTypeName == L"SD" || info.busTypeName == L"MMC") {
            return true;
        }
        // Some USB sticks report as SATA/RAID fixed disks but set RemovableMedia.
        if (info.removableMedia) {
            return true;
        }
    }
    return false;
}

std::set<wchar_t> GetVhdDriveLetters() {
    std::set<wchar_t> letters;
    const DWORD mask = GetLogicalDrives();

    for (wchar_t letter = L'A'; letter <= L'Z'; ++letter) {
        if (letter == L'C') {
            continue;
        }
        const int bit = letter - L'A';
        if ((mask & (1u << bit)) == 0) {
            continue;
        }

        wchar_t volumePath[] = L"\\\\.\\?:";
        volumePath[4] = letter;
        const HANDLE volume =
            CreateFileW(volumePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (volume == INVALID_HANDLE_VALUE) {
            continue;
        }

        std::vector<BYTE> extentsBuf(sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * 8);
        DWORD returned = 0;
        if (!DeviceIoControl(volume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, extentsBuf.data(),
                             static_cast<DWORD>(extentsBuf.size()), &returned, nullptr)) {
            if (GetLastError() != ERROR_MORE_DATA) {
                CloseHandle(volume);
                continue;
            }
            extentsBuf.resize(sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * 32);
            returned = 0;
            if (!DeviceIoControl(volume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, extentsBuf.data(),
                                 static_cast<DWORD>(extentsBuf.size()), &returned, nullptr)) {
                CloseHandle(volume);
                continue;
            }
        }
        CloseHandle(volume);

        if (returned < sizeof(VOLUME_DISK_EXTENTS)) {
            continue;
        }

        const auto* extents = reinterpret_cast<const VOLUME_DISK_EXTENTS*>(extentsBuf.data());
        for (DWORD i = 0; i < extents->NumberOfDiskExtents; ++i) {
            if (IsFileBackedVirtualDisk(extents->Extents[i].DiskNumber)) {
                letters.insert(letter);
                break;
            }
        }
    }

    return letters;
}

bool TryAddDrive(std::vector<DriveInfo>& drives, wchar_t letter, const std::wstring& kind) {
    const std::wstring root = std::wstring(1, letter) + L":\\";

    ULARGE_INTEGER freeBytesAvailable{};
    ULARGE_INTEGER totalBytes{};
    ULARGE_INTEGER totalFree{};
    if (!GetDiskFreeSpaceExW(root.c_str(), &freeBytesAvailable, &totalBytes, &totalFree)) {
        return false;
    }

    if (totalBytes.QuadPart < kMinDriveCapacityBytes) {
        return false;
    }

    DriveInfo info;
    info.letter = std::wstring(1, letter) + L":";
    info.label = QueryVolumeLabel(root);
    info.kind = kind;
    info.totalBytes = totalBytes.QuadPart;
    info.freeBytes = freeBytesAvailable.QuadPart;

    const auto freeGb = static_cast<uint64_t>(info.freeBytes / (1024ULL * 1024ULL * 1024ULL));
    const auto totalGb = static_cast<uint64_t>(info.totalBytes / (1024ULL * 1024ULL * 1024ULL));

    std::wstring typeLabel;
    if (kind == L"VHD") {
        typeLabel = i18n::Tr(L"ui.drive_type_vhd");
    } else if (kind == L"HDD") {
        typeLabel = i18n::Tr(L"ui.drive_type_hdd");
    } else {
        typeLabel = i18n::Tr(L"ui.drive_type_usb");
    }

    info.kind = typeLabel;

    std::wstring driveName = info.letter;
    if (!info.label.empty()) {
        std::wstring safeLabel = info.label;
        for (wchar_t& ch : safeLabel) {
            if (ch == L'"') {
                ch = L'\'';
            }
        }
        driveName += L" \"" + safeLabel + L"\"";
    }

    info.display = i18n::Tr(L"ui.drive_format", driveName, typeLabel,
                            std::to_wstring(freeGb), std::to_wstring(totalGb));
    drives.push_back(std::move(info));
    return true;
}

wchar_t NormalizeDriveLetter(const std::wstring& driveLetter) {
    if (driveLetter.empty()) {
        return L'\0';
    }
    wchar_t letter = driveLetter[0];
    if (letter >= L'a' && letter <= L'z') {
        letter = static_cast<wchar_t>(letter - L'a' + L'A');
    }
    return letter;
}

bool GetVolumeDiskNumbers(const wchar_t letter, std::vector<DWORD>& diskNumbers) {
    diskNumbers.clear();
    if (letter < L'A' || letter > L'Z') {
        return false;
    }

    wchar_t volumePath[] = L"\\\\.\\?:";
    volumePath[4] = letter;
    const HANDLE volume =
        CreateFileW(volumePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (volume == INVALID_HANDLE_VALUE) {
        return false;
    }

    std::vector<BYTE> extentsBuf(sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * 8);
    DWORD returned = 0;
    if (!DeviceIoControl(volume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, extentsBuf.data(),
                         static_cast<DWORD>(extentsBuf.size()), &returned, nullptr)) {
        if (GetLastError() != ERROR_MORE_DATA) {
            CloseHandle(volume);
            return false;
        }
        extentsBuf.resize(sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * 32);
        returned = 0;
        if (!DeviceIoControl(volume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, extentsBuf.data(),
                             static_cast<DWORD>(extentsBuf.size()), &returned, nullptr)) {
            CloseHandle(volume);
            return false;
        }
    }
    CloseHandle(volume);

    if (returned < sizeof(VOLUME_DISK_EXTENTS)) {
        return false;
    }

    const auto* extents = reinterpret_cast<const VOLUME_DISK_EXTENTS*>(extentsBuf.data());
    diskNumbers.reserve(extents->NumberOfDiskExtents);
    for (DWORD i = 0; i < extents->NumberOfDiskExtents; ++i) {
        diskNumbers.push_back(extents->Extents[i].DiskNumber);
    }
    return !diskNumbers.empty();
}

bool IsUsbBusVolume(const wchar_t letter) {
    std::vector<DWORD> diskNumbers;
    if (!GetVolumeDiskNumbers(letter, diskNumbers)) {
        return false;
    }
    for (const DWORD diskNumber : diskNumbers) {
        if (IsUsbBusDisk(diskNumber)) {
            return true;
        }
    }
    return false;
}

bool IdentityMatches(const DriveIdentity& a, const DriveIdentity& b) {
    if (!a.valid || !b.valid) {
        return false;
    }
    for (const DWORD diskA : a.diskNumbers) {
        for (const DWORD diskB : b.diskNumbers) {
            if (diskA == diskB) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace

std::vector<DriveInfo> ListTargetDrives(const bool includeAllDrives) {
    std::vector<DriveInfo> drives;
    const std::set<wchar_t> vhdLetters = GetVhdDriveLetters();
    const DWORD mask = GetLogicalDrives();

    for (wchar_t letter = L'A'; letter <= L'Z'; ++letter) {
        if (letter == L'C') {
            continue;
        }
        const int bit = letter - L'A';
        if ((mask & (1u << bit)) == 0) {
            continue;
        }

        const std::wstring root = std::wstring(1, letter) + L":\\";
        const UINT driveType = GetDriveTypeW(root.c_str());
        const bool isVhd = vhdLetters.count(letter) > 0;
        const bool isUsbBus = IsUsbBusVolume(letter);
        const bool isUsb = driveType == DRIVE_REMOVABLE || isUsbBus;
        const bool isFixed = driveType == DRIVE_FIXED && !isUsbBus;

        if (isVhd) {
            TryAddDrive(drives, letter, L"VHD");
        } else if (isUsb) {
            TryAddDrive(drives, letter, L"USB");
        } else if (includeAllDrives && isFixed) {
            TryAddDrive(drives, letter, L"HDD");
        }
    }

    return drives;
}

int DefaultDriveIndex(const std::vector<DriveInfo>& drives) {
    for (size_t i = 0; i < drives.size(); ++i) {
        if (drives[i].kind == i18n::Tr(L"ui.drive_type_vhd")) {
            return static_cast<int>(i);
        }
    }
    return drives.empty() ? -1 : 0;
}

DriveIdentity GetDriveIdentity(const std::wstring& driveLetter) {
    DriveIdentity identity;
    const wchar_t letter = NormalizeDriveLetter(driveLetter);
    if (letter == L'\0') {
        return identity;
    }

    const std::wstring root = std::wstring(1, letter) + L":\\";
    DWORD serial = 0;
    if (GetVolumeInformationW(root.c_str(), nullptr, 0, &serial, nullptr, nullptr, nullptr, 0)) {
        identity.volumeSerial = serial;
    }

    identity.valid = GetVolumeDiskNumbers(letter, identity.diskNumbers);
    return identity;
}

DiskDeviceInfo QueryDiskDeviceInfo(const DWORD diskNumber) {
    DiskDeviceInfo info;
    info.diskNumber = diskNumber;

    std::wostringstream path;
    path << L"\\\\.\\PhysicalDrive" << diskNumber;
    HANDLE disk = CreateFileW(path.str().c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0,
                              nullptr);
    if (disk == INVALID_HANDLE_VALUE) {
        disk = CreateFileW(path.str().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                           0, nullptr);
    }
    if (disk == INVALID_HANDLE_VALUE) {
        EnrichDiskDeviceInfoFromSetupApi(info);
        info.valid = !info.friendlyName.empty() || !info.usbVid.empty() || !info.vendor.empty();
        return info;
    }

    STORAGE_PROPERTY_QUERY query{};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    BYTE buffer[2048]{};
    DWORD returned = 0;
    const BOOL ok = DeviceIoControl(disk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer,
                                    sizeof(buffer), &returned, nullptr);
    CloseHandle(disk);
    if (ok && returned >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
        const auto* desc = reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR*>(buffer);
        const char* raw = reinterpret_cast<const char*>(buffer);
        info.vendor = SanitizeDescriptorAscii(raw, returned, desc->VendorIdOffset);
        info.product = SanitizeDescriptorAscii(raw, returned, desc->ProductIdOffset);
        info.revision = SanitizeDescriptorAscii(raw, returned, desc->ProductRevisionOffset);
        info.serial = SanitizeDescriptorAscii(raw, returned, desc->SerialNumberOffset);
        info.busTypeName = BusTypeDisplayName(desc->BusType);
        info.removableMedia = desc->RemovableMedia != FALSE;
        info.valid = true;
    }

    EnrichDiskDeviceInfoFromSetupApi(info);
    if (!info.valid) {
        info.valid = !info.friendlyName.empty() || !info.usbVid.empty() || !info.vendor.empty() ||
                     !info.busTypeName.empty();
    }
    return info;
}

DiskDeviceInfo GetDriveDeviceInfo(const std::wstring& driveLetter) {
    const wchar_t letter = NormalizeDriveLetter(driveLetter);
    if (letter == L'\0') {
        return {};
    }

    std::vector<DWORD> diskNumbers;
    if (!GetVolumeDiskNumbers(letter, diskNumbers) || diskNumbers.empty()) {
        return {};
    }
    return QueryDiskDeviceInfo(diskNumbers.front());
}

std::wstring FormatDiskDeviceInfoFields(const DiskDeviceInfo& info) {
    if (!info.valid) {
        return L"";
    }

    std::wostringstream ss;
    ss << L"disk=" << info.diskNumber;
    if (!info.busTypeName.empty()) {
        ss << L"  bus=" << info.busTypeName;
    }
    if (info.removableMedia) {
        ss << L"  removable=yes";
    }
    if (!info.vendor.empty()) {
        ss << L"  vendor=\"" << info.vendor << L"\"";
    }
    if (!info.product.empty()) {
        ss << L"  product=\"" << info.product << L"\"";
    }
    if (!info.revision.empty()) {
        ss << L"  rev=\"" << info.revision << L"\"";
    }
    if (!info.manufacturer.empty() && info.manufacturer != info.vendor) {
        ss << L"  mfg=\"" << info.manufacturer << L"\"";
    }
    if (!info.friendlyName.empty()) {
        ss << L"  friendly=\"" << info.friendlyName << L"\"";
    }
    if (!info.usbVid.empty()) {
        ss << L"  vid=" << info.usbVid;
    }
    if (!info.usbPid.empty()) {
        ss << L"  pid=" << info.usbPid;
    }
    if (!info.serial.empty()) {
        ss << L"  serial=\"" << info.serial << L"\"";
    }
    return ss.str();
}

namespace {

bool DriveLetterMatchesIdentity(const std::wstring& driveLetter, const DriveIdentity& identity) {
    const wchar_t letter = NormalizeDriveLetter(driveLetter);
    if (letter == L'\0') {
        return false;
    }

    const std::wstring root = std::wstring(1, letter) + L":\\";
    if (GetDriveTypeW(root.c_str()) == DRIVE_NO_ROOT_DIR) {
        return false;
    }

    return IdentityMatches(identity, GetDriveIdentity(driveLetter));
}

std::wstring FindDriveLetterForIdentity(const DriveIdentity& identity) {
    const DWORD mask = GetLogicalDrives();
    for (wchar_t letter = L'A'; letter <= L'Z'; ++letter) {
        if (letter == L'C') {
            continue;
        }
        const int bit = letter - L'A';
        if ((mask & (1u << bit)) == 0) {
            continue;
        }

        const std::wstring candidate = std::wstring(1, letter) + L":";
        if (DriveLetterMatchesIdentity(candidate, identity)) {
            return candidate;
        }
    }
    return L"";
}

}  // namespace

std::wstring ResolveDriveLetterAfterVentoy(const std::wstring& expectedLetter, const DriveIdentity& before) {
    if (!before.valid) {
        return expectedLetter;
    }

    for (int attempt = 0; attempt < 20; ++attempt) {
        if (attempt > 0) {
            Sleep(500);
        }

        if (DriveLetterMatchesIdentity(expectedLetter, before)) {
            return expectedLetter;
        }

        const std::wstring found = FindDriveLetterForIdentity(before);
        if (!found.empty()) {
            return found;
        }
    }

    return L"";
}

uint64_t GetDriveTotalBytes(const std::wstring& driveLetter) {
    const wchar_t letter = NormalizeDriveLetter(driveLetter);
    if (letter == L'\0') {
        return 0;
    }

    const std::wstring root = std::wstring(1, letter) + L":\\";
    ULARGE_INTEGER freeBytesAvailable{};
    ULARGE_INTEGER totalBytes{};
    ULARGE_INTEGER totalFree{};
    if (!GetDiskFreeSpaceExW(root.c_str(), &freeBytesAvailable, &totalBytes, &totalFree)) {
        return 0;
    }
    return totalBytes.QuadPart;
}

bool MeetsMinimumDriveCapacity(const uint64_t totalBytes) {
    return totalBytes >= kMinDriveCapacityBytes;
}

bool MeetsMinimumDriveCapacity(const std::wstring& driveLetter) {
    return MeetsMinimumDriveCapacity(GetDriveTotalBytes(driveLetter));
}

uint64_t GetDriveFreeBytes(const std::wstring& root) {
    std::wstring normalized = root;
    if (normalized.size() == 2 && normalized[1] == L':') {
        normalized += L'\\';
    }
    return QueryFreeBytes(normalized).QuadPart;
}

bool IsDriveLetterPresent(const std::wstring& driveLetter) {
    if (driveLetter.size() < 2 || driveLetter[1] != L':') {
        return false;
    }

    wchar_t letter = driveLetter[0];
    if (letter >= L'a' && letter <= L'z') {
        letter = static_cast<wchar_t>(letter - L'a' + L'A');
    }
    if (letter < L'A' || letter > L'Z') {
        return false;
    }

    const int bit = letter - L'A';
    const DWORD mask = GetLogicalDrives();
    return (mask & (1u << bit)) != 0;
}

DestinationDriveStatus CheckDestinationDrive(const std::wstring& root, std::wstring* errorDetail) {
    if (root.size() < 2 || root[1] != L':') {
        return DestinationDriveStatus::Ok;
    }

    wchar_t letter = root[0];
    if (letter >= L'a' && letter <= L'z') {
        letter = static_cast<wchar_t>(letter - L'a' + L'A');
    }
    const std::wstring driveLetter = std::wstring(1, letter) + L":";
    if (!IsDriveLetterPresent(driveLetter)) {
        return DestinationDriveStatus::Removed;
    }

    std::wstring normalized = root;
    if (normalized.size() == 2 && normalized[1] == L':') {
        normalized += L'\\';
    }

    if (!GetVolumeInformationW(normalized.c_str(), nullptr, 0, nullptr, nullptr, nullptr, nullptr, 0)) {
        if (errorDetail) {
            *errorDetail = FormatWindowsError(GetLastError());
        }
        return DestinationDriveStatus::IoError;
    }

    ULARGE_INTEGER freeBytesAvailable{};
    ULARGE_INTEGER totalBytes{};
    ULARGE_INTEGER totalFree{};
    if (!GetDiskFreeSpaceExW(normalized.c_str(), &freeBytesAvailable, &totalBytes, &totalFree)) {
        if (errorDetail) {
            *errorDetail = FormatWindowsError(GetLastError());
        }
        return DestinationDriveStatus::IoError;
    }

    const HANDLE rootDir =
        CreateFileW(normalized.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (rootDir == INVALID_HANDLE_VALUE) {
        if (errorDetail) {
            *errorDetail = FormatWindowsError(GetLastError());
        }
        return DestinationDriveStatus::IoError;
    }
    CloseHandle(rootDir);

    return DestinationDriveStatus::Ok;
}

uint64_t GetArchiveUncompressedSize(const std::wstring& sevenZipExe, const std::wstring& archivePath) {
    const std::wstring listOut = JoinPath(GetExeDirectory(), L"_7za_list_tmp.txt");
    DeleteFileW(listOut.c_str());

    std::wstring cmd =
        L"cmd /c \"\"" + sevenZipExe + L"\"\" l -slt \"\"" + archivePath + L"\"\" > \"\"" + listOut + L"\"\" 2>&1\"";
    std::vector<wchar_t> cmdLine(cmd.begin(), cmd.end());
    cmdLine.push_back(L'\0');

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi)) {
        return 0;
    }

    ChildProcessRegistration childProcess(pi.hProcess);
    while (WaitForSingleObject(pi.hProcess, 100) == WAIT_TIMEOUT) {
        if (IsCancelRequested()) {
            TerminateProcess(pi.hProcess, 1);
            break;
        }
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (IsCancelRequested()) {
        DeleteFileW(listOut.c_str());
        return 0;
    }

    HANDLE h = CreateFileW(listOut.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return 0;
    }
    std::string utf8;
    char chunk[4096];
    DWORD read = 0;
    while (ReadFile(h, chunk, sizeof(chunk), &read, nullptr) && read > 0) {
        utf8.append(chunk, chunk + read);
    }
    CloseHandle(h);
    DeleteFileW(listOut.c_str());

    const int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wide(static_cast<size_t>(wideLen > 0 ? wideLen - 1 : 0), L'\0');
    if (wideLen > 0) {
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), wideLen);
    }

    bool inFile = false;
    uint64_t size = 0;
    uint64_t total = 0;
    for (const auto& line : SplitLines(wide)) {
        if (line == L"----------") {
            if (inFile && size > 0) {
                total += size;
            }
            inFile = true;
            size = 0;
            continue;
        }
        if (line.rfind(L"Path = ", 0) == 0) {
            const std::wstring path = line.substr(7);
            if (path.empty() || path.back() == L'\\' || path.back() == L'/') {
                inFile = false;
                size = 0;
            }
            continue;
        }
        if (inFile && line.rfind(L"Size = ", 0) == 0) {
            size = _wcstoui64(line.c_str() + 7, nullptr, 10);
        }
    }
    if (inFile && size > 0) {
        total += size;
    }
    return total;
}

}  // namespace medicat
