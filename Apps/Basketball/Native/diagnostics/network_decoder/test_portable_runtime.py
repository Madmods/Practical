import pathlib,tempfile,unittest
from unittest.mock import patch
import qb_packet_host as host


class PortableRuntimeTests(unittest.TestCase):
    def test_frozen_runtime_resolves_beside_executable_after_move(self):
        with tempfile.TemporaryDirectory() as folder:
            base=pathlib.Path(folder)/'Friend folder with spaces'/ 'network_decoder'/'runtime'
            base.mkdir(parents=True)
            for name in ('WinDivert.dll','WinDivert64.sys'):(base/name).touch()
            with patch.object(host.sys,'frozen',True,create=True),patch.object(host.sys,'executable',str(base/'vanta-network-host.exe')):
                self.assertEqual(host.dependency_path(),base/'WinDivert.dll')

    def test_source_runtime_resolves_beside_module(self):
        with tempfile.TemporaryDirectory() as folder:
            base=pathlib.Path(folder)
            for name in ('WinDivert.dll','WinDivert64.sys'):(base/name).touch()
            with patch.object(host.sys,'frozen',False,create=True),patch.object(host,'__file__',str(base/'qb_packet_host.py')):
                self.assertEqual(host.dependency_path(),base/'WinDivert.dll')

    def test_partial_extraction_has_clear_error_and_no_machine_path_fallback(self):
        for missing in ('WinDivert.dll','WinDivert64.sys'):
            with self.subTest(missing=missing),tempfile.TemporaryDirectory() as folder:
                base=pathlib.Path(folder)
                for name in ('WinDivert.dll','WinDivert64.sys'):
                    if name!=missing:(base/name).touch()
                with patch.object(host.sys,'frozen',True,create=True),patch.object(host.sys,'executable',str(base/'vanta-network-host.exe')):
                    with self.assertRaisesRegex(FileNotFoundError,'Extract the entire Vanta ZIP'):
                        host.dependency_path()


if __name__=='__main__':unittest.main()
