# Licensing and provenance

The application includes an adaptation of Input Overlay's
`deps/common/input_data.cpp` at commit
`7d97ae9e66d9e4dd968f759fcafd447969510f23`. That file identifies:

> Copyright 2023 Alex <uni@vrsal.xyz>.

Its header grants GPL version 2, without an "or later" clause. The original header
is retained verbatim at the start of `src/input/input_state.cpp`, followed by a
notice describing the standalone modifications made in 2026. The adaptation
preserves dispatch of key/button state, cursor movement and wheel activity;
it replaces the library event type, containers and OBS clock and omits gamepads,
remote copying and plugin integration.

The standalone application and new scaffold source are distributed under
**GPL-2.0-only**. The complete GPL v2 text is in `LICENSE` and
`LICENSES/GPL-2.0-only.txt`, copied from upstream. The original repository's MIT
text and Yibo Ma copyright notice are preserved verbatim in `LICENSES/MIT.txt`.
That original permission does not override upstream GPL terms or make the
combined application MIT-only.

When distributing this derivative:

- Preserve copyright, licence and warranty notices, attribution and modification
  notices. Include the GPL v2 text and retained MIT notice.
- License the combined GPL-derived work under GPL v2, with no additional
  restrictions that conflict with recipients' GPL rights.
- For executable distribution, satisfy GPL v2 section 3. The intended release
  process is to accompany binaries with the complete corresponding machine-readable
  source for that exact build, including modifications and scripts/build files
  needed to build and install it. A generic link to upstream or this project's
  moving main branch is not sufficient on its own.
- Keep an exact source commit and release source archive associated with every
  binary. A `cmake --install` directory includes notices, but is not by itself a
  complete source distribution.
- Do not add upstream dependencies or assets based solely on their presence in
  upstream. Check their individual licences, compatibility and notices first.

For the controlling terms, read GPL v2 sections 1–3 in [LICENSE](../LICENSE).
The GPLv2-only versus GPLv3 libuiohook issue remains unresolved for future reuse;
this scaffold neither includes nor links libuiohook. No claim is made that the
upstream's full dependency combination has been cleared. See
[the dependency audit](UPSTREAM_AUDIT.md) for the scope of inspection.
