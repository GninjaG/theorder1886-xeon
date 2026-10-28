"""MSYS2 bootstrap must honor the selected toolchain root, not stale installs."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from paths import ROOT


@unittest.skipIf(sys.platform == 'win32',
                 'POSIX fixture: Windows native build validates /clang64 runtime')
class MsysRootSelectionTest(unittest.TestCase):
    def test_explicit_root_is_used_without_touching_other_installations(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / 'custom-msys'
            (root / 'clang64' / 'bin').mkdir(parents=True)
            (root / 'usr' / 'bin').mkdir(parents=True)
            env = os.environ.copy()
            env['MSYS2_ROOT'] = str(root)
            env['MSYSTEM'] = 'ANOTHER_TOOLCHAIN'
            env['PATH'] = '/usr/bin'
            result = subprocess.run(
                ['bash', '-c', 'source ./msys2-env.sh; printf "%s\\n%s\\n" "$PATH" "$MSYSTEM"'],
                cwd=ROOT, env=env, text=True, capture_output=True, check=True)
            path, system = result.stdout.splitlines()
            self.assertEqual(system, 'CLANG64')
            self.assertEqual(path.split(':')[:3],
                             [str(root / 'clang64' / 'bin'),
                              str(root / 'usr' / 'bin'), '/usr/bin'])

    def test_active_virtual_root_precedes_fallback_in_script(self):
        content = (ROOT / 'msys2-env.sh').read_text(encoding='utf-8')
        self.assertLess(content.index('elif [[ -d /clang64/bin'),
                        content.index('elif [[ -d /c/msys64/clang64/bin'))


if __name__ == '__main__':
    unittest.main()
