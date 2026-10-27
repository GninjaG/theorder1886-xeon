// SPDX-License-Identifier: GPL-2.0-or-later
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>
#include "port_texture_reject_diag.h"
#include "port_texture_size_ab.h"

using Vulkan::TextureRejectDiag::Reason;
using Vulkan::TextureRejectDiag::Sampler;
using Vulkan::TextureRejectDiag::ShouldLog;

int main() {
    using namespace Vulkan::TextureSizeAb;
    static_assert(SelectLimit("") == CurrentMaxGuestBytes);
    static_assert(SelectLimit("256") == CurrentMaxGuestBytes);
    static_assert(SelectLimit("512") == LegacyMaxGuestBytes);
    static_assert(SelectLimit("1024") == CurrentMaxGuestBytes);
    static_assert(SelectLimit("512 ") == CurrentMaxGuestBytes);
    static_assert(SelectLimit("0") == CurrentMaxGuestBytes);
    static_assert(!Reject(CurrentMaxGuestBytes, false, CurrentMaxGuestBytes));
    static_assert(Reject(CurrentMaxGuestBytes + 1, false, CurrentMaxGuestBytes));
    static_assert(!Reject(384ULL << 20, false, LegacyMaxGuestBytes));
    static_assert(Reject(384ULL << 20, false, CurrentMaxGuestBytes));
    static_assert(!Reject(LegacyMaxGuestBytes, false, LegacyMaxGuestBytes));
    static_assert(Reject(LegacyMaxGuestBytes + 1, false, LegacyMaxGuestBytes));
    // The pathological padded guest image must remain rejected in both modes.
    static_assert(Reject(186ULL << 20, true, CurrentMaxGuestBytes));
    static_assert(Reject(186ULL << 20, true, LegacyMaxGuestBytes));

    // Real encoded T# boundaries: identical rejection helpers on both paths.
    using Vulkan::TextureRejectDiag::EarlySharpReason;
    using Vulkan::TextureRejectDiag::MappingSharpReason;
    static_assert(EarlySharpReason(0x0fff, false, false, 1, false, false) ==
                  Reason::LowAddress);
    static_assert(EarlySharpReason(0x10000, false, false, 1, false, false) ==
                  Reason::Count);
    static_assert(EarlySharpReason(0x10000, true, false, 1, false, false) ==
                  Reason::InvalidDataFormat);
    static_assert(EarlySharpReason(0x10000, false, true, 1, false, false) ==
                  Reason::FmaskDescriptor);
    static_assert(EarlySharpReason(0x10000, false, false, 2049, false, false) ==
                  Reason::ExcessiveLayers);
    static_assert(EarlySharpReason(0x10000, false, false, 2048, false, false) ==
                  Reason::Count);
    static_assert(EarlySharpReason(0x10000, false, false, 1, true, false) ==
                  Reason::InvalidArrayRange);
    static_assert(EarlySharpReason(0x10000, false, false, 1, false, true) ==
                  Reason::InvalidMipRange);
    static_assert(MappingSharpReason(false, true, 1, 1, 1) ==
                  Reason::UnmappedAddress);
    static_assert(MappingSharpReason(true, false, 1, 1, 1) ==
                  Reason::UnsupportedFormat);
    static_assert(MappingSharpReason(true, true, 8191, 8191, 1) ==
                  Reason::Count);
    static_assert(MappingSharpReason(true, true, 8192, 1, 1) ==
                  Reason::ExcessiveDimensions);
    static_assert(MappingSharpReason(true, true, 1, 8192, 1) ==
                  Reason::ExcessiveDimensions);
    static_assert(MappingSharpReason(true, true, 1, 1, 2049) ==
                  Reason::ExcessiveLayers);
    static_assert(MappingSharpReason(true, true, 8191, 8191, 5) ==
                  Reason::ExcessiveTexels);
    static_assert(MappingSharpReason(true, true, 2048, 1, 17) ==
                  Reason::ExcessiveLayerDimensions);
    static_assert(MappingSharpReason(true, true, 2047, 2047, 16) ==
                  Reason::Count);

    static_assert(Vulkan::TextureRejectDiag::ReasonCount == 13);
    static_assert(!ShouldLog(0));
    static_assert(ShouldLog(1) && ShouldLog(8));
    static_assert(!ShouldLog(9) && !ShouldLog(15));
    static_assert(ShouldLog(16) && ShouldLog(32));
    static_assert(!ShouldLog(33));

    using Vulkan::TextureRejectDiag::EarlyReason;
    using Vulkan::TextureRejectDiag::MappingReason;
    static_assert(EarlyReason(true, true, true, true, true, true) == Reason::LowAddress);
    static_assert(EarlyReason(false, true, false, false, false, false) ==
                  Reason::InvalidDataFormat);
    static_assert(EarlyReason(false, false, true, false, false, false) ==
                  Reason::FmaskDescriptor);
    static_assert(EarlyReason(false, false, false, true, false, false) ==
                  Reason::ExcessiveLayers);
    static_assert(EarlyReason(false, false, false, false, true, false) ==
                  Reason::InvalidArrayRange);
    static_assert(EarlyReason(false, false, false, false, false, true) ==
                  Reason::InvalidMipRange);
    static_assert(EarlyReason(false, false, false, false, false, false) ==
                  Reason::Count);
    static_assert(MappingReason(true, true, true, true, true, true) ==
                  Reason::UnmappedAddress);
    static_assert(MappingReason(false, true, false, false, false, false) ==
                  Reason::UnsupportedFormat);
    static_assert(MappingReason(false, false, true, false, false, false) ==
                  Reason::ExcessiveDimensions);
    static_assert(MappingReason(false, false, false, true, false, false) ==
                  Reason::ExcessiveLayers);
    static_assert(MappingReason(false, false, false, false, true, false) ==
                  Reason::ExcessiveTexels);
    static_assert(MappingReason(false, false, false, false, false, true) ==
                  Reason::ExcessiveLayerDimensions);
    static_assert(MappingReason(false, false, false, false, false, false) ==
                  Reason::Count);

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
