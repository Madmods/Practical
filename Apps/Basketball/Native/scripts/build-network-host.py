"""Package the inspector and QB throw hosts after build-network-decoder-lab.cmd."""
import argparse,pathlib,sys,shutil,importlib.metadata
parser=argparse.ArgumentParser();parser.add_argument('--packaging-deps',type=pathlib.Path);args=parser.parse_args()
if args.packaging_deps:sys.path.insert(0,str(args.packaging_deps.resolve()))
import PyInstaller.__main__
native=pathlib.Path(__file__).resolve().parents[1];source=native/'diagnostics'/'network_decoder';build=native/'build'/'network_decoder'
PyInstaller.__main__.run([
    '--noconfirm','--clean','--onedir','--console','--name','vanta-network-host',
    '--distpath',str(build/'package'),'--workpath',str(build/'packager-work'),'--specpath',str(build/'packager-work'),
    '--paths',str(source),'--paths',str(build/'network_lab_deps'),
    '--add-data',str(source/'replication-type-maps.json')+';.',
    '--add-binary',str(build/'scan_aes_schedules.dll')+';.',
    str(source/'network_decoder_host.py'),
])
runtime=build/'runtime';shutil.copytree(build/'package'/'vanta-network-host',runtime,dirs_exist_ok=True)
shutil.copy2(build/'capture-network-lab.exe',runtime/'capture-network-lab.exe')
licenses=runtime/'licenses';licenses.mkdir(exist_ok=True)
python_license=pathlib.Path(sys.base_prefix)/'LICENSE.txt'
if python_license.exists():shutil.copy2(python_license,licenses/'Python-LICENSE.txt')
for name in ('cryptography','pyinstaller'):
    distribution=importlib.metadata.distribution(name)
    for entry in distribution.files or []:
        if 'licenses' in entry.parts:
            path=distribution.locate_file(entry)
            if path.is_file():
                target=licenses/name/path.name;target.parent.mkdir(exist_ok=True);shutil.copy2(path,target)
zstd=build/'network_lab_deps'/'zstandard-0.25.0.dist-info'/'licenses'
if zstd.exists():shutil.copytree(zstd,licenses/'zstandard',dirs_exist_ok=True)
print('Packaged host:',runtime/'vanta-network-host.exe')
