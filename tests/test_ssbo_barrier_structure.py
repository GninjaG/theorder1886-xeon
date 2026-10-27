"""Check the IR pass stage and opcode coverage without Vulkan/MSYS2.

The behavioral/native IR test is test_formatted_ssbo_barriers.cpp and is
compiled by tools/run_tests.sh. This only checks the source-level preconditions
required by the stage at which the real pass executes.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SsboBarrierStageTests(unittest.TestCase):
    def test_formatted_ops_are_classified_before_format_lowering(self):
        pass_source = (ROOT / 'gpu/shadps4/shader_recompiler/ir/passes'
                       / 'insert_ssbo_store_load_barrier.cpp').read_text(encoding='utf-8')
        recompiler = (ROOT / 'gpu/shadps4/shader_recompiler'
                      / 'recompiler.cpp').read_text(encoding='utf-8')
        def switch_body(name):
            match = re.search(r'bool ' + name +
                              r'\(const IR::Inst& inst\)\s*\{(.*?)\n\}', pass_source, re.S)
            self.assertIsNotNone(match, name)
            return match.group(1)

        stores = switch_body('IsStoreBuffer')
        loads = switch_body('IsLoadBuffer')
        self.assertIn('case IR::Opcode::StoreBufferFormatF32:', stores)
        self.assertIn('case IR::Opcode::LoadBufferFormatF32:', loads)
        self.assertIn('case IR::Opcode::StoreBufferU32:', stores)
        self.assertIn('case IR::Opcode::LoadBufferU32:', loads)
        self.assertLess(recompiler.index('InsertSsboStoreLoadBarrier(program, profile)'),
                        recompiler.index('LowerBufferFormatToRaw(program)'))
        self.assertIn('if (!profile.needs_ssbo_store_load_barrier)', pass_source)

    def test_targeted_shader_cache_version(self):
        cache = (ROOT / 'gpu/shadps4/video_core/renderer_vulkan'
                 / 'vk_pipeline_serialization.cpp').read_text(encoding='utf-8')
        # Cache salts now live in a pure, profile-gated helper rather than
        # directly in vk_pipeline_serialization.cpp.
        salt = (ROOT / 'gpu/shadps4/shader_recompiler'
                / 'ssbo_barrier_cache_version.h').read_text(encoding='utf-8')
        self.assertIn('NeedsFormattedSsboBarrierCacheVersion(buffers)', salt)
        self.assertIn('NeedsWrittenSsboBarrierCacheVersion(buffers)', salt)
        self.assertIn('if (!needs_ssbo_barriers) return 0;', salt)
        self.assertIn('SsboBarrierCacheSalt(needs_ssbo_barriers, info.buffers)', cache)
        self.assertIn('ShaderBinaryVersionFor(info, spec, needs_ssbo_barriers)', cache)
        self.assertIn('profile.needs_ssbo_store_load_barrier', cache)
        self.assertEqual(cache.count('profile.needs_ssbo_store_load_barrier'), 3)
        pipeline = (ROOT / 'gpu/shadps4/video_core/renderer_vulkan'
                    / 'vk_pipeline_cache.cpp').read_text(encoding='utf-8')
        self.assertIn('RegisterShaderMeta(info, spec.fetch_shader_data', pipeline)
        self.assertIn('profile.needs_ssbo_store_load_barrier);', pipeline)
        self.assertIn('ShaderBinaryVersion = 15u', cache)

    def test_every_shader_meta_writer_uses_the_active_profile_gate(self):
        source = (ROOT / 'gpu/shadps4/video_core/renderer_vulkan'
                  / 'vk_pipeline_cache.cpp').read_text(encoding='utf-8')
        # Three paths publish SPIR-V metadata: async completion, first
        # synchronous compilation and a later new specialization.
        calls = re.findall(r'\\bRegisterShaderMeta\\s*\\((.*?)\\);', source, re.S)
        self.assertEqual(len(calls), 3)
        for call in calls:
            self.assertRegex(call, r',\\s*profile\\.needs_ssbo_store_load_barrier\\s*
        barrier = (ROOT / 'gpu/shadps4/shader_recompiler/ir/passes'
                   / 'insert_ssbo_store_load_barrier.cpp').read_text(encoding='utf-8')
        self.assertIn('#include "shader_recompiler/ir/passes/resource_pass.h"', barrier)
        self.assertIn('const bool is_atomic = IsBufferAtomic(inst);', barrier)
        self.assertIn('if (IsStoreBuffer(inst) || is_atomic)', barrier)
        self.assertIn('if (IsLoadBuffer(inst) || is_atomic)', barrier)
        self.assertIn('!IsStoreBuffer(inst) && !IsBufferAtomic(inst)', barrier)
        self.assertIn('plain_loaded_buffers.count(binding) != 0', barrier)

    def test_real_ir_regression_test_is_in_full_native_suite(self):
        cmake = (ROOT / 'gpu/CMakeLists.txt').read_text(encoding='utf-8')
        runner = (ROOT / 'tools/run_tests.sh').read_text(encoding='utf-8')
        self.assertIn('add_executable(formatted-ssbo-barriers-test ', cmake)
        self.assertIn('../tests/test_formatted_ssbo_barriers.cpp', cmake)
        self.assertIn('multiwave-barriers formatted-ssbo-barriers)', runner)


if __name__ == '__main__':
    unittest.main()
)

    def test_atomic_rmw_is_classified_as_read_and_write(self):
        barrier = (ROOT / 'gpu/shadps4/shader_recompiler/ir/passes'
                   / 'insert_ssbo_store_load_barrier.cpp').read_text(encoding='utf-8')
        self.assertIn('#include "shader_recompiler/ir/passes/resource_pass.h"', barrier)
        self.assertIn('const bool is_atomic = IsBufferAtomic(inst);', barrier)
        self.assertIn('if (IsStoreBuffer(inst) || is_atomic)', barrier)
        self.assertIn('if (IsLoadBuffer(inst) || is_atomic)', barrier)
        self.assertIn('!IsStoreBuffer(inst) && !IsBufferAtomic(inst)', barrier)
        self.assertIn('plain_loaded_buffers.count(binding) != 0', barrier)

    def test_real_ir_regression_test_is_in_full_native_suite(self):
        cmake = (ROOT / 'gpu/CMakeLists.txt').read_text(encoding='utf-8')
        runner = (ROOT / 'tools/run_tests.sh').read_text(encoding='utf-8')
        self.assertIn('add_executable(formatted-ssbo-barriers-test ', cmake)
        self.assertIn('../tests/test_formatted_ssbo_barriers.cpp', cmake)
        self.assertIn('multiwave-barriers formatted-ssbo-barriers)', runner)


if __name__ == '__main__':
    unittest.main()
