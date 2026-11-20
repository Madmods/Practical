# Native application

See the repository README for current build, test, networking and packaging instructions.

- `src`: C++ application and headers.
- `ui`, `resources`: embedded WebView interface, fonts and Windows resources.
- `tests`: offline regression tests.
- `diagnostics`: version-pinned probes, decoder code and synthetic experiments.
- `scripts`: build, test and packaging entry points.
- `packaging`: the friend package setup guide.
- `studio_lab`: local Studio adapter sources.
- `build`: generated output, excluded from Git.

`build-webui.cmd` builds a candidate; `build.cmd` also runs native checks and installs it beside the Native directory. Keep the executable and release catalog from the same build. Optional networking dependencies are supplied separately through the documented build arguments.
