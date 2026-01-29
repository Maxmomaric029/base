#include "memory.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

// Helper for case-insensitive comparison
bool IsSameString(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    return std::equal(a.begin(), a.end(), b.begin(),
                      [](unsigned char a, unsigned char b) {
                          return std::tolower(a) == std::tolower(b);
                      });
}

Memory::Memory(const std::string& processName) {
    this->hProcess = NULL;
    this->processId = 0;
    this->moduleBase = 0;
    Attach(processName);
}

Memory::~Memory() {
    if (this->hProcess) {
        CloseHandle(this->hProcess);
    }
}

bool Memory::Attach(const std::string& processName) {
    // Explicitly use ANSI version (TH32CS_SNAPPROCESS)
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32A pe; // Force ANSI struct
    pe.dwSize = sizeof(PROCESSENTRY32A);

    if (Process32FirstA(hSnap, &pe)) { // Force ANSI function
        do {
            if (IsSameString(processName, pe.szExeFile)) {
                this->processId = pe.th32ProcessID;
                this->hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, this->processId);
                
                if (this->hProcess) {
                    std::cout << "[SUCCESS] Attached to: " << pe.szExeFile << " (PID: " << this->processId << ")" << std::endl;
                } else {
                    std::cout << "[ERROR] Found process but failed to OpenProcess. RUN AS ADMIN!" << std::endl;
                }
                break;
            }
        } while (Process32NextA(hSnap, &pe)); // Force ANSI function
    }
    CloseHandle(hSnap);
    return (this->hProcess != NULL);
}

uintptr_t Memory::GetModuleBaseAddress(const std::string& moduleName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->processId);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;

    MODULEENTRY32A me; // Force ANSI struct
    me.dwSize = sizeof(MODULEENTRY32A);
    uintptr_t baseAddr = 0;

    if (Module32FirstA(hSnap, &me)) { // Force ANSI function
        do {
            if (IsSameString(moduleName, me.szModule)) {
                baseAddr = (uintptr_t)me.modBaseAddr;
                std::cout << "[DEBUG] Found Module: " << moduleName << " @ " << std::hex << baseAddr << std::dec << std::endl;
                break;
            }
        } while (Module32NextA(hSnap, &me));
    }
    CloseHandle(hSnap);
    return baseAddr;
}

bool Memory::WriteBytes(uintptr_t address, const std::vector<unsigned char>& bytes) {
    return WriteProcessMemory(this->hProcess, (LPVOID)address, bytes.data(), bytes.size(), NULL);
}

bool Memory::Patch(uintptr_t address, const std::vector<unsigned char>& bytes) {
    DWORD oldProtect;
    if (!VirtualProtectEx(this->hProcess, (LPVOID)address, bytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    bool success = WriteBytes(address, bytes);

    VirtualProtectEx(this->hProcess, (LPVOID)address, bytes.size(), oldProtect, &oldProtect);
    return success;
}

// Convert "AA BB CC" to vector of ints (-1 for ?)
std::vector<int> ParsePattern(const std::string& pattern) {
    std::vector<int> bytes;
    std::stringstream ss(pattern);
    std::string byteStr;
    
    while (ss >> byteStr) {
        if (byteStr == "?" || byteStr == "??")
            bytes.push_back(-1);
        else
            bytes.push_back(std::stoi(byteStr, nullptr, 16));
    }
    return bytes;
}

uintptr_t Memory::FindPattern(const std::string& moduleName, const std::string& patternStr) {
    uintptr_t moduleBase = GetModuleBaseAddress(moduleName);
    
    // For Emulators: libil2cpp.so might not be listed as a Windows Module because it's inside the VM memory.
    // If GetModuleBaseAddress returns 0, we can't easily scan it without full memory dump scanning (which is slow).
    // But let's assume the user knows it's mapped or we scan the main process.
    
    if (moduleBase == 0) {
        std::cout << "[DEBUG] Module " << moduleName << " not found. Skipping scan." << std::endl;
        return 0; 
    }

    // Get Module Size
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->processId);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32A me;
    me.dwSize = sizeof(MODULEENTRY32A);
    DWORD moduleSize = 0;
    if (Module32FirstA(hSnap, &me)) {
        do {
            if (IsSameString(moduleName, me.szModule)) {
                moduleSize = me.modBaseSize;
                break;
            }
        } while (Module32NextA(hSnap, &me));
    }
    CloseHandle(hSnap);

    if (moduleSize == 0) return 0;

    std::vector<unsigned char> moduleBytes(moduleSize);
    if (!ReadProcessMemory(this->hProcess, (LPCVOID)moduleBase, moduleBytes.data(), moduleSize, NULL))
        return 0;

    std::vector<int> pattern = ParsePattern(patternStr);
    
    for (size_t i = 0; i < moduleSize - pattern.size(); ++i) {
        bool found = true;
        for (size_t j = 0; j < pattern.size(); ++j) {
            if (pattern[j] != -1 && pattern[j] != moduleBytes[i + j]) {
                found = false;
                break;
            }
        }
        if (found) {
            return moduleBase + i;
        }
    }
    return 0;
}