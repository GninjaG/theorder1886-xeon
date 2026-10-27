// SPDX-License-Identifier: GPL-2.0-or-later
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>
#include "port_texture_reject_diag.h"

using Vulkan::TextureRejectDiag::Reason;
using Vulkan::TextureRejectDiag::Sampler;
using Vulkan::TextureRejectDiag::ShouldLog;

int main() {
    static_assert(Vulkan::TextureRejectDiag::ReasonCount == 13);
    static_assert(!ShouldLog(0));
    static_assert(ShouldLog(1) && ShouldLog(8));
    static_assert(!ShouldLog(9) && !ShouldLog(15));
    static_assert(ShouldLog(16) && ShouldLog(32));
    static_assert(!ShouldLog(33));

    Sampler sample;
    assert(sample.Observe(Reason::LowAddress, 0x1234, false) == 0);
    assert(sample.Count(Reason::LowAddress) == 0);
    assert(sample.Observe(Reason::LowAddress, 0x1234, true, 0x5555) == 0);
    assert(sample.Count(Reason::LowAddress) == 0);
    for (std::uint64_t n = 1; n <= 32; ++n) {
        const auto logged = sample.Observe(Reason::LowAddress, 0x1234, true);
        assert(logged == (ShouldLog(n) ? n : 0));
    }
    assert(sample.Count(Reason::LowAddress) == 32);
    assert(sample.Count(Reason::ImplausibleImage) == 0);
    assert(sample.Observe(Reason::InvalidMipRange, 0x1234, true, 0x1234) == 1);
    assert(sample.Observe(Reason::Count, 0x1234, true) == 0);
    assert(sample.Count(Reason::Count) == 0);

    for (unsigned i = 0; i < Vulkan::TextureRejectDiag::ReasonCount; ++i) {
        auto reason = static_cast<Reason>(i);
        assert(Vulkan::TextureRejectDiag::Name(reason)[0] != '\0');
    }

    Sampler parallel;
    std::vector<std::thread> threads;
    for (int n = 0; n != 8; ++n) {
        threads.emplace_back([&] {
            for (int i = 0; i != 10000; ++i) {
                parallel.Observe(Reason::UnmappedAddress, 0x1234, true);
            }
        });
    }
    for (auto& thread : threads) thread.join();
    assert(parallel.Count(Reason::UnmappedAddress) == 80000);
    std::puts("texture rejection diagnostic: all tests passed");
    return 0;
}
