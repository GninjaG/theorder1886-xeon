"""Native output selection while temporal upscaling stays parked in the launcher."""
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from paths import ROOT
sys.path.insert(0, str(ROOT / 'launcher'))
import port_launcher_win as launcher


class UpscalerSettingsTest(unittest.TestCase):
    def test_remaining_choices_have_no_spatial_upscaler(self):
        self.assertEqual([value for value, _text in launcher.UPSCALERS], ['dlss', 'fsr3', 'fsr4', 'fsr411', 'taa', 'off'])

    def test_removed_controls_do_not_supply_defaults_to_save(self):
        for key in ('fsr1', 'sharpen', 'sharpness', 'rcas', 'rcas_strength'):
            self.assertNotIn(key, launcher.INI_DEFAULTS)

    def test_graphics_page_offers_only_native_backend_output_sizes(self):
        class Page:
            def __init__(self):
                self.choices = []
                self.rows = []

            def scrolled_page(self, *args):
                return self

            def build_display(self, _parent):
                pass

            def section(self, *args):
                pass

            def choice(self, _parent, key, store, options, **kwargs):
                self.choices.append((key, store, options))
                self.preserve_unknown = kwargs.get('preserve_unknown')
                return key

            def row(self, _parent, title, widget, hint=None, **_kwargs):
                self.rows.append((title, widget, hint))

            def add_tab_reset_bar(self, *_args):
                pass

        page = Page()
        with mock.patch.object(launcher, 'LANG', 'en'):
            launcher.Launcher.build_graphics(page)
        self.assertEqual([(key, store) for key, store, _options in page.choices],
                         [('upscaler', 'ini'), ('preset', 'ini'), ('output_res', 'ini')])
        self.assertTrue(page.preserve_unknown)
        self.assertEqual([value for value, _label in page.choices[2][2]],
                         ['1280x720', '1920x1080', '2560x1440', '3840x2160'])
        self.assertEqual([value for value, _label in launcher.OUTPUTS],
                         ['1280x720', '1920x1080', '2560x1440', '3440x1440',
                          '3840x2160', '5120x1440'])
        self.assertIn('post-processing', page.rows[2][2])

    def test_output_choice_round_trips_without_changing_saved_upscaler(self):
        with tempfile.TemporaryDirectory() as folder:
            config = Path(folder) / 'theorder.ini'
            config.write_text('upscaler=fsr4\npreset=3\noutput_res=1920x1080\n'
                              'external_runtime_key=keep\n', encoding='utf-8')
            with mock.patch.dict(os.environ, {'THEORDER_CONFIG': str(config)}):
                initial, lines = launcher.load_ini()
                self.assertEqual(initial['output_res'], '1920x1080')
                launcher.save_ini({'output_res': '3840x2160'}, lines)
                saved, _lines = launcher.load_ini()
                self.assertEqual(saved['output_res'], '3840x2160')
                self.assertEqual(saved['upscaler'], 'fsr4')
                self.assertEqual(saved['preset'], '3')
                self.assertEqual(saved['external_runtime_key'], 'keep')

    def test_unavailable_saved_output_is_not_reset_when_the_selector_opens(self):
        class Variable:
            def __init__(self, value):
                self.value, self.callback = value, None

            def get(self):
                return self.value

            def set(self, value):
                self.value = value
                if self.callback:
                    self.callback()

            def trace_add(self, _kind, callback):
                self.callback = callback

        class Combobox:
            def __init__(self, _parent, values, **_kwargs):
                self.values, self.index, self.label = values, -1, ''

            def current(self, index=None):
                if index is None:
                    return self.index
                self.index, self.label = index, self.values[index]

            def set(self, label):
                self.index, self.label = -1, label

            def bind(self, _event, callback):
                self.on_select = callback

        app = launcher.Launcher.__new__(launcher.Launcher)
        app.ttk = type('Widgets', (), {'Combobox': Combobox})
        saved = Variable('3440x1440')
        with mock.patch.object(launcher, 'LANG', 'en'):
            box = app.choice(None, 'output_res', 'ini', launcher.NATIVE_OUTPUTS,
                             var=saved, preserve_unknown=True)
            self.assertEqual(saved.get(), '3440x1440')
            self.assertEqual(box.current(), -1)
            self.assertIn('3440x1440', box.label)
            self.assertEqual(len(box.values), 4)
            box.current(3)
            box.on_select(None)
            self.assertEqual(saved.get(), '3840x2160')


if __name__ == '__main__':
    unittest.main()
