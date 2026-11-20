"""Require real symlink and UNC fixtures for the complete Windows OS tests."""
import os
from pathlib import Path
import sys
import tempfile

workspace = Path(sys.argv[1]).resolve()
unc = os.environ.get('CODON_OS_TEST_UNC_DIR')
if not unc:
    native = str(workspace)
    if workspace.drive.endswith(':'):
        unc = '\\\\localhost\\' + workspace.drive[0] + '$' + native[2:]
    else:
        unc = native
if not unc.startswith('\\\\') or not os.path.isdir(unc):
    raise RuntimeError('The full Windows suite requires an accessible UNC directory; '
                       'set CODON_OS_TEST_UNC_DIR to one.')
with tempfile.TemporaryDirectory(prefix='codon-symlink-fixture-') as directory:
    root = Path(directory)
    (root / 'target').mkdir()
    os.symlink('target', root / 'link', target_is_directory=True)
    if not (root / 'link').is_dir():
        raise RuntimeError('Windows directory symlinks are not usable')
    os.rmdir(root / 'link')
print(unc)
