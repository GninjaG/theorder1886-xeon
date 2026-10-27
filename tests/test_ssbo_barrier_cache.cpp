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


static_assert(Shader::SsboBarrierCacheSalt(false, std::array<Flags, 0>{}) == 0);
static_assert(Shader::SsboBarrierCacheSalt(false, std::array{Flags{true, true}}) == 0);
static_assert(Shader::SsboBarrierCacheSalt(false, std::array{Flags{false, true}}) == 0);
static_assert(Shader::SsboBarrierCacheSalt(true, std::array<Flags, 0>{}) == 0);
static_assert(Shader::SsboBarrierCacheSalt(true, std::array{Flags{false, false}}) == 0);
static_assert(Shader::SsboBarrierCacheSalt(true, std::array{Flags{true, false}}) == 0);
static_assert(Shader::SsboBarrierCacheSalt(true, std::array{Flags{false, true}}) ==
              (1u << 19));
static_assert(Shader::SsboBarrierCacheSalt(true, std::array{Flags{true, true}}) ==
              ((1u << 18) | (1u << 19)));
static_assert(Shader::SsboBarrierCacheSalt(
                  true, std::array{Flags{false, true}, Flags{true, false}}) == (1u << 19));

int main() {
    std::array<Flags, 2> values{{{true, false}, {false, true}}};
    assert(!NeedsFormattedSsboBarrierCacheVersion(values));
    values[0].is_written = true;
    assert(NeedsFormattedSsboBarrierCacheVersion(values));
    values[0].is_written = false;
    assert(!NeedsFormattedSsboBarrierCacheVersion(values));
    values[1].is_written = false;
    assert(!NeedsWrittenSsboBarrierCacheVersion(values));
    values[1].is_written = true;
    assert(NeedsWrittenSsboBarrierCacheVersion(values));
    assert(Shader::SsboBarrierCacheSalt(false, values) == 0);
    assert(Shader::SsboBarrierCacheSalt(true, values) == (1u << 19));
    std::puts("profile-gated formatted/atomic SSBO shader cache revision: OK");
}
