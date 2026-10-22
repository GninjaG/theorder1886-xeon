// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>

namespace PortProcessMemory {

struct Usage {
    std::uint64_t private_bytes; ///< private commit, not reserved address space
    std::uint64_t working_set;   ///< in RAM now
    std::uint64_t peak_working_set;
    bool private_available = false;
    bool resident_available = false;
};

Usage Get();

} // namespace PortProcessMemory
