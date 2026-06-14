#pragma once

#include <string>

namespace medicat {

bool HttpGet(const std::wstring& url, std::wstring& body, std::wstring& error);
bool HttpDownloadFile(const std::wstring& url, const std::wstring& outputPath, std::wstring& error);
bool TestInternetConnection(std::wstring& error);

}  // namespace medicat
