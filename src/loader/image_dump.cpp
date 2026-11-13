#include "image_dump.h"

#include <windows.h>

#include <cstring>
#include <format>
#include <vector>

namespace ff7vr::loader {

std::string dump_game_image(const std::filesystem::path& out) {
    auto* base = reinterpret_cast<const unsigned char*>(GetModuleHandleW(nullptr));
    if (!base) return "image dump: no main module";
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    const SIZE_T size = nt->OptionalHeader.SizeOfImage;

    // Copy page range by page range; anything not readable stays zero.
    std::vector<unsigned char> img(size, 0);
    SIZE_T unreadable = 0;
    for (SIZE_T off = 0; off < size;) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(base + off, &mbi, sizeof(mbi))) break;
        const SIZE_T end = (std::min)(size, static_cast<SIZE_T>(static_cast<const unsigned char*>(mbi.BaseAddress) + mbi.RegionSize - base));
        const DWORD p = mbi.Protect & 0xff;
        const bool readable = mbi.State == MEM_COMMIT && !(mbi.Protect & PAGE_GUARD) && p != PAGE_NOACCESS && p != 0;
        if (readable) std::memcpy(img.data() + off, base + off, end - off);
        else unreadable += end - off;
        off = end;
    }

    // Raw layout = memory layout, so the file maps 1:1 to the running image.
    auto* ndos = reinterpret_cast<IMAGE_DOS_HEADER*>(img.data());
    auto* nnt = reinterpret_cast<IMAGE_NT_HEADERS64*>(img.data() + ndos->e_lfanew);
    nnt->OptionalHeader.FileAlignment = nnt->OptionalHeader.SectionAlignment;
    auto* sec = IMAGE_FIRST_SECTION(nnt);
    for (WORD i = 0; i < nnt->FileHeader.NumberOfSections; ++i) {
        sec[i].PointerToRawData = sec[i].VirtualAddress;
        sec[i].SizeOfRawData = (std::max)(sec[i].Misc.VirtualSize, sec[i].SizeOfRawData);
    }

    HANDLE f = CreateFileW(out.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return std::format("image dump: cannot create the file ({})", GetLastError());
    SIZE_T done = 0;
    bool ok = true;
    while (done < size && ok) {
        DWORD wrote = 0;
        const DWORD chunk = static_cast<DWORD>((std::min)(size - done, static_cast<SIZE_T>(1) << 24));
        ok = WriteFile(f, img.data() + done, chunk, &wrote, nullptr) && wrote == chunk;
        done += wrote;
    }
    CloseHandle(f);
    if (!ok) return std::format("image dump: write failed after {} bytes ({})", done, GetLastError());
    return std::format("image dump: {} bytes of the game image (base {:#x}) written, {} unreadable bytes as zeros",
                       size, reinterpret_cast<std::uintptr_t>(base), unreadable);
}

}  // namespace ff7vr::loader
