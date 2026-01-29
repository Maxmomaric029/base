#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include "memory.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <windows.h>
#include <TlHelp32.h>

// Helper for case-insensitive comparison (Works with std::string which is ANSI)
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
    if (this->hProcess) {
        CloseHandle(this->hProcess);
    }
}

bool Memory::Attach(const std::string& processName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe; 
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnap, &pe)) {
        do {
            // Convert pe.szExeFile to std::string for comparison
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

uintptr_t Memory::GetModuleBaseAddress(const std::string& moduleName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->processId);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;

    MODULEENTRY32 me;
    me.dwSize = sizeof(MODULEENTRY32);
    uintptr_t baseAddr = 0;

    if (Module32First(hSnap, &me)) {
        do {
#ifdef UNICODE
            std::wstring wStr(me.szModule);
            std::string sModule(wStr.begin(), wStr.end());
#else
            std::string sModule(me.szModule);
#endif
            if (IsSameString(moduleName, sModule.c_str())) {
                baseAddr = (uintptr_t)me.modBaseAddr;
                std::cout << "[DEBUG] Found Module: " << moduleName << " @ " << std::hex << baseAddr << std::dec << std::endl;
                break;
            }
        } while (Module32Next(hSnap, &me));
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
    
    if (moduleBase == 0) return 0;

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->processId);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32 me;
    me.dwSize = sizeof(MODULEENTRY32);
    DWORD moduleSize = 0;
    if (Module32First(hSnap, &me)) {
        do {
#ifdef UNICODE
            std::wstring wStr(me.szModule);
            std::string sModule(wStr.begin(), wStr.end());
#else
            std::string sModule(me.szModule);
#endif
            if (IsSameString(moduleName, sModule.c_str())) {
                moduleSize = me.modBaseSize;
                break;
            }
        } while (Module32Next(hSnap, &me));
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
