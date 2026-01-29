#include "memory.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <windows.h>
#include <TlHelp32.h>
#include <vector>

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

    // Scan ALL valid memory, but be smart about alignment
    while (VirtualQueryEx(this->hProcess, pRemote, &mbi, sizeof(mbi))) {
        // Emulator RAM is usually ReadWrite (Data) or ExecuteReadWrite.
        // We MUST scan PAGE_READWRITE because that's where the Android Heap lives.
        bool isValid = (mbi.State == MEM_COMMIT) &&
                       ((mbi.Protect & PAGE_GUARD) == 0) &&
                       ((mbi.Protect & PAGE_NOACCESS) == 0);

        // Filter: Optimization. Usually game memory is Private or Mapped.
        // We skip Image (DLLs) unless mapped, to avoid patching Windows DLLs by mistake.
        if (isValid && (mbi.Type == MEM_PRIVATE || mbi.Type == MEM_MAPPED)) {
            
            std::vector<unsigned char> buffer(mbi.RegionSize);
            SIZE_T bytesRead;
            
            if (ReadProcessMemory(this->hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {
                for (size_t i = 0; i < bytesRead - pattern.size(); ++i) {
                    
                    // CRITICAL: ARM Alignment Check
                    // Instructions are always 4-byte aligned (0, 4, 8, C).
                    // If we match at offset 1, 2, or 3, it's garbage/data, NOT code.
                    if (((uintptr_t)mbi.BaseAddress + i) % 4 != 0) continue;

                    bool found = true;
                    for (size_t j = 0; j < pattern.size(); ++j) {
                        if (pattern[j] != -1 && pattern[j] != buffer[i + j]) {
                            found = false;
                            break;
                        }
                    }
                    if (found) {
                        uintptr_t result = (uintptr_t)mbi.BaseAddress + i;
                        std::cout << "[SCAN] Found Pattern at: " << std::hex << result << std::dec << " (Region: " << mbi.RegionSize << " bytes)" << std::endl;
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
    // Attempt to make it writable and executable (just in case)
    if (!VirtualProtectEx(this->hProcess, (LPVOID)address, bytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        // If that fails, try just ReadWrite
        if (!VirtualProtectEx(this->hProcess, (LPVOID)address, bytes.size(), PAGE_READWRITE, &oldProtect))
            return false;
    }

    SIZE_T written;
    bool success = WriteProcessMemory(this->hProcess, (LPVOID)address, bytes.data(), bytes.size(), &written);

    // Restore original protection
    VirtualProtectEx(this->hProcess, (LPVOID)address, bytes.size(), oldProtect, &oldProtect);
    
    // IMPORTANT: Flush CPU cache so it executes the new instruction, not the old cached one
    FlushInstructionCache(this->hProcess, (LPCVOID)address, bytes.size());

    return success;
}

uintptr_t Memory::GetModuleBaseAddress(const std::string& moduleName) { return 0; }
bool Memory::WriteBytes(uintptr_t address, const std::vector<unsigned char>& bytes) { return Patch(address, bytes); }
