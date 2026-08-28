# Repository Structure

Canonical sources:

- `RevivalControl.xcodeproj/` — Xcode project
- `RevivalControl/` — iPhone target sources
- `RevivalWatch Watch App/` — Apple Watch target sources
- `firmware/revival50/revival50.ino` — ESP32-S3 firmware
- `docs/` — wiring and project documentation

Legacy snapshot duplicates currently still present:

- `ios/`
- `watch/`
- `/revival50.ino`

These should not be edited. They were created during the initial repository bootstrap before the real Xcode project was committed. They can be removed once deletion is performed safely.
