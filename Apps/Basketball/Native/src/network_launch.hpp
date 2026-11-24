#pragma once
#include <windows.h>
#include <filesystem>
#include <string>

namespace vantaNetwork {
// Create a hidden child with Vanta's existing token. Elevation is requested by
// Vanta's executable manifest at startup, never by the networking helper.
inline HANDLE startHidden(const std::filesystem::path& executable,const std::wstring& arguments){
 std::wstring command=L"\""+executable.wstring()+L"\" "+arguments;
 const auto directory=executable.parent_path().wstring();
 STARTUPINFOW startup{sizeof startup};startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;
 PROCESS_INFORMATION process{};
 if(!CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,directory.empty()?nullptr:directory.c_str(),&startup,&process))return nullptr;
 CloseHandle(process.hThread);return process.hProcess;
}
}
