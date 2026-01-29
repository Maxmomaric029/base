#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <TlHelp32.h>
#include <iostream>

class Memory {
public:
    HANDLE hProcess;
    DWORD processId;
    uintptr_t moduleBase;

    Memory(const std::string& processName);
    ~Memory();

    bool Attach(const std::string& processName);
    uintptr_t GetModuleBaseAddress(const std::string& moduleName);
    
    // Read Memory
    template <typename T>
    T Read(uintptr_t address) {
        T value;
        ReadProcessMemory(hProcess, (LPCVOID)address, &value, sizeof(T), NULL);
        return value;
    }

    // Write Memory
    template <typename T>
    bool Write(uintptr_t address, T value) {
        return WriteProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T), NULL);
    }

    bool WriteBytes(uintptr_t address, const std::vector<unsigned char>& bytes);
    
    // AOB Scan
    uintptr_t FindPattern(const std::string& moduleName, const std::string& pattern);
    
    // Patch Helper
    bool Patch(uintptr_t address, const std::vector<unsigned char>& bytes);
};
