#pragma once
#include <string>
#include <vector>

namespace Globals {
    inline bool bGuestReset = false;
    inline bool bAimLock = false;
    
    // Detected process info
    inline std::string currentProcess = "None";
    inline bool isAttached = false;

    // Supported Emulators
    const std::vector<std::string> SUPPORTED_EMULATORS = {
        "HD-Player.exe",          // BlueStacks 5 / MSI App Player
        "dnplayer.exe",           // LDPlayer
        "LdVBoxHeadless.exe",     // LDPlayer (VirtualBox mode)
        "AndroidEmulatorEx.exe"   // GameLoop
    };

    const std::string TARGET_MODULE = "libil2cpp.so"; 
}
