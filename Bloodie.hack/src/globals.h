#pragma once
#include <string>

namespace Globals {
    inline bool bGuestReset = false;
    inline bool bAimLock = false;
    
    // Config
    const std::string TARGET_PROCESS = "HD-Player.exe"; // Example: BlueStacks
    const std::string TARGET_MODULE = "libil2cpp.so"; // Typically Android games on emulators run in a library, or just scan the main executable memory if mapped. 
    // Note: For emulators, memory scanning is tricky because the game memory is inside the emulator's heap. 
    // But for this generic request, we will scan the process/module defined here.
    // Users often scan the whole process or a specific region.
}
