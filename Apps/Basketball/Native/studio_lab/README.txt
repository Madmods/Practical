LOCAL STUDIO ADAPTER

The adapter uses src/studio_connection.hpp and the Studio-only scripts in this
folder. It does not connect to a public Roblox server through Studio APIs.

The native application is started with --studio and reads a private
vanta-lab-connection.json beside Vanta.exe. It expects a loopback port from
1024 to 65535 and a randomly generated 48-character lowercase hexadecimal token.

Install BuildField and Profile as modules under ReplicatedStorage/VantaQBLab,
with a RemoteEvent named Event. Client.client.luau belongs in the local player
scripts; Server.server.luau belongs in the server scripts. Supply the matching
private connection table {port=..., token=...} as Connection under VantaQBLab
for the client and VantaLabConnection beside the server script. Local Studio
WebSocket capability and the matching running native adapter are required.

Use a fresh local token; do not publish the generated JSON or connection modules
or include them in shared place files. Those files are excluded from Git.
BuildField creates the synthetic field. This adapter is separate from the
external live-game packet helper and does not require its driver.
