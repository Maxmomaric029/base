#include "memory.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <windows.h>
#include <TlHelp32.h>
#include <vector>

// Helper for case-insensitive comparison
bool IsSameString(const std::string& a, const char* b) {
    std::string bStr(b);
    if (a.size() != bStr.size()) return false;
    return std::equal(a.begin(), a.end(), bStr.begin(),
                      [](unsigned char a, unsigned char b) {
                          return std::tolower(a) == std::tolower(b);
                      });
}

Memory::Memory(const std::string& processName) {
    this->hProcess = NULL;
    this->processId = 0;
    this->moduleBase = 0;
}

Memory::~Memory() {
    if (this->hProcess) CloseHandle(this->hProcess);
}

bool Memory::Attach(const std::string& processName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe; 
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnap, &pe)) {
        do {
#ifdef UNICODE
            std::wstring wStr(pe.szExeFile);
            std::string sExeFile(wStr.begin(), wStr.end());
#else
            std::string sExeFile(pe.szExeFile);
#endif
            if (IsSameString(processName, sExeFile.c_str())) {
                this->processId = pe.th32ProcessID;
                this->hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, this->processId);
                if (this->hProcess) {
                    std::cout << "[SUCCESS] Attached to: " << sExeFile << " (PID: " << this->processId << ")" << std::endl;
                }
                break;
            }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return (this->hProcess != NULL);
}

// Emulators don't expose Android libs as Windows Modules. We must scan ALL memory.
// Returns 0 if not found.
uintptr_t Memory::FindPattern(const std::string& ignoredModuleName, const std::string& patternStr) {
    if (!this->hProcess) return 0;

    std::vector<int> pattern;
    std::stringstream ss(patternStr);
    std::string byteStr;
    while (ss >> byteStr) {
        if (byteStr == "?" || byteStr == "??") pattern.push_back(-1);
        else pattern.push_back(std::stoi(byteStr, nullptr, 16));
    }

    if (pattern.empty()) return 0;

    unsigned char* pRemote = nullptr;
    MEMORY_BASIC_INFORMATION mbi;

    // Iterate over process memory regions
    while (VirtualQueryEx(this->hProcess, pRemote, &mbi, sizeof(mbi))) {
        // Filter for readable/writable/executable memory and committed pages
        if (mbi.State == MEM_COMMIT && 
           (mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_READWRITE | PAGE_READONLY))) {
            
            // Optimization: Skip very small regions or typically system mapped regions if known, 
            // but for safety we scan everything relevant.
            
            std::vector<unsigned char> buffer(mbi.RegionSize);
            SIZE_T bytesRead;
            if (ReadProcessMemory(this->hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {
                for (size_t i = 0; i < bytesRead - pattern.size(); ++i) {
                    bool found = true;
                    for (size_t j = 0; j < pattern.size(); ++j) {
                        if (pattern[j] != -1 && pattern[j] != buffer[i + j]) {
                            found = false;
                            break;
                        }
                    }
                    if (found) {
                        uintptr_t result = (uintptr_t)mbi.BaseAddress + i;
                        std::cout << "[SCAN] Found Pattern at: " << std::hex << result << std::dec << std::endl;
                        return result;
                    }
                }
            }
        }
        pRemote += mbi.RegionSize;
    }
    return 0;
}

bool Memory::Patch(uintptr_t address, const std::vector<unsigned char>& bytes) {
    if (!this->hProcess || address == 0) return false;
    
    DWORD oldProtect;
    if (!VirtualProtectEx(this->hProcess, (LPVOID)address, bytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    SIZE_T written;
    bool success = WriteProcessMemory(this->hProcess, (LPVOID)address, bytes.data(), bytes.size(), &written);

    VirtualProtectEx(this->hProcess, (LPVOID)address, bytes.size(), oldProtect, &oldProtect);
    return success;
}

// Unused but kept for interface compatibility
uintptr_t Memory::GetModuleBaseAddress(const std::string& moduleName) { return 0; }
bool Memory::WriteBytes(uintptr_t address, const std::vector<unsigned char>& bytes) { return Patch(address, bytes); }