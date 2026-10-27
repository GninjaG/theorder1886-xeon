// SPDX-License-Identifier: GPL-2.0-or-later
// Targeted shader-cache salt test; no GPU or game files needed.
#include <array>
#include <cassert>
#include <cstdio>
#include "shader_recompiler/ssbo_barrier_cache_version.h"

struct Flags { bool is_formatted; bool is_written; };
using Shader::NeedsFormattedSsboBarrierCacheVersion;
using Shader::NeedsWrittenSsboBarrierCacheVersion;
static_assert(!NeedsFormattedSsboBarrierCacheVersion(std::array<Flags, 0>{}));
static_assert(!NeedsFormattedSsboBarrierCacheVersion(std::array{Flags{false, false}}));
static_assert(!NeedsFormattedSsboBarrierCacheVersion(std::array{Flags{true, false}, Flags{false, true}}));
static_assert(NeedsFormattedSsboBarrierCacheVersion(std::array{Flags{true, true}}));
static_assert(NeedsFormattedSsboBarrierCacheVersion(std::array{Flags{false, false}, Flags{true, true}}));

static_assert(!NeedsWrittenSsboBarrierCacheVersion(std::array<Flags, 0>{}));
static_assert(!NeedsWrittenSsboBarrierCacheVersion(std::array{Flags{false, false}, Flags{true, false}}));
static_assert(NeedsWrittenSsboBarrierCacheVersion(std::array{Flags{false, true}}));
static_assert(NeedsWrittenSsboBarrierCacheVersion(std::array{Flags{true, true}}));

int main() {
    std::array<Flags, 2> values{{{true, false}, {false, true}}};
    assert(!NeedsFormattedSsboBarrierCacheVersion(values));
    values[0].is_written = true;
    assert(NeedsFormattedSsboBarrierCacheVersion(values));
    values[0].is_written = false;
    assert(!NeedsFormattedSsboBarrierCacheVersion(values));
    assert(!NeedsWrittenSsboBarrierCacheVersion(values));
    values[1].is_written = true;
    assert(NeedsWrittenSsboBarrierCacheVersion(values));
    std::puts("selective formatted/atomic SSBO shader cache revision: OK");
}
