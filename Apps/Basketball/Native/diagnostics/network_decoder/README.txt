VANTA NETWORKING SOURCES

The optional helper maintains the application's current supported game flow.
Networking prepares independently of the QB aim toggle. New throw edits still
require an enabled, current native plan and physical click. Setup/draining
prime the parser without consuming new throw ownership. Already accepted
reliable edits remain pinned for retransmission until acknowledged/drained.

The decoder supports validated event/acknowledgement profiles, including the
observed match UnionOperation.CFrame acknowledgement. Cached schemas are bound
to process creation and connection identity. Fresh schemas replace reader
state; parser recovery uses a bounded in-memory sample without replaying input.
Transport keys are not written to disk. Diagnostic captures and session data
must not be committed.

Build with scripts/build-network-decoder-lab.cmd and scripts/build-network-host.py
from Native. End-user runtimes include Python and load WinDivert.dll and
WinDivert64.sys beside the helper executable. Supply approved driver files
separately; see the repository README. No machine-specific path is required.

Use scripts/test-network-decoder-lab.cmd for the offline test suite. Frozen
helper --self-test checks imports, AES and filter compilation without opening
capture. Source compatibility remains pinned to build-offsets.hpp.
