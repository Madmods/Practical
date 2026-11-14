Vanta network inspector update

Installed in the existing Apps\Basketball folder. Open Vanta, enter the workspace,
then choose System > Network. Search Clicked to inspect the saved throw.
The prior executable and release catalog are preserved in
Practical Basketball\Archives\Network-Live-20261113.

Confirmed throw from the controlled 30-second test:
  Capture packet: 1141 (throw-test.pcap)
  Remote instance in this session: peer 1, instance 5459797
  Method: FireServer
  Arguments: "Clicked", Vector3, Vector3, 60
  Wire event member: 4595, OnRemoteServerEvent
  Complete 75-byte application record; event starts at byte 21.
See decoder-join-test\throw-event.json for the complete arguments and metadata.
Packet numbers and instance IDs refer to this recorded session.

Start capture runs a sniff-only capture for 60 seconds and requests Windows
administrator access. Stop capture stops the helper. Capturing never starts
automatically. The last decoded capture is retained and loads when Vanta starts.
Clicked calls are retained separately so routine traffic does not displace them.
Clear calls clears the retained display and saved last-capture file.

The join schema is cached for up to 30 minutes, bound to the process creation
time, network endpoints and authenticated connection IDs. Rejoin if the decoder
asks for a schema. Transport keys stay in process memory and are not saved.

This is a limited decoder. Full instance paths, all replication record formats,
QUIC key updates and unreliable application datagrams are not supported.
It currently supports the observed Roblox version-4ea8e85481e34f46 profile.
It is not a complete replacement for every Yuna feature.

Verification: 46 offline decoder checks; 27 native validation checks including
the captured records; 159 existing native UI checks; 94 WebView checks, including
the real throw displayed in the inspector. The captured test contains 4,674
authenticated packets and the final decoder reads 692 calls, including the throw.
The test capture completed and all capture helpers have stopped.

The update package is for this installation and uses its existing
Tools\LatencyTest\bin\WinDivert.dll. Source and tests are in
Apps\Basketball\Native\diagnostics\network_decoder and Native\src.
Build with scripts\build-network-decoder-lab.cmd, scripts\build-network-host.py
(PyInstaller required), and scripts\build-webui.cmd. The packaged host includes
its Python runtime; running the inspector does not require a Python installation.
