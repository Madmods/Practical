# Native external

See the repository root README for build prerequisites, optional runtime dependencies, QB controls and collaboration instructions.

- `src`: native host, engine, game module boundaries, QB planning and overlays.
- `ui`: embedded WebView2 interface and bundled font licenses.
- `resources`: embedded application resources.
- `tests`: isolated native behavior and render checks.
- `diagnostics/network_decoder`: transport parsing, schema cache, packet editor and bounded helper hosts.
- `scripts`: build, validation and packaging entry points.

Generated files stay under `build` and are ignored by Git. Live sessions and local settings must not be committed.
