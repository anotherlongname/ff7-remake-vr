#pragma once
// [debug] dump_game_image = 1: writes the game's executable image, as mapped in memory, to a file
// beside the log. Used to find the signatures for a game build the mod does not know yet (the
// Microsoft Store / Xbox app copy cannot be read from disk). Off by default.

#include <filesystem>
#include <string>

namespace ff7vr::loader {

// Writes the main module's image to `out` as a PE file whose sections sit at their memory
// offsets (raw = virtual), so disassemblers load it at the module's own addresses. Unreadable
// pages are written as zeros. Returns a one-line result for the log.
std::string dump_game_image(const std::filesystem::path& out);

}  // namespace ff7vr::loader
