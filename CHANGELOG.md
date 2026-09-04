# Changelog

## [1.05] - TBD

## [1.04] - 2026-09-03

What's new in Devpad 1.04:

- New Windows ARM64 support. SHA: d2307ce68af4774e643649d2ea68c090a7ebc469
- New TUI mode(WIP.) SHA: d2307ce68af4774e643649d2ea68c090a7ebc469
- New NextGen mode(WIP.) SHA: d2307ce68af4774e643649d2ea68c090a7ebc469
- Added Json support. SHA: d2307ce68af4774e643649d2ea68c090a7ebc469
- Added Lua support. SHA: d2307ce68af4774e643649d2ea68c090a7ebc469
- Added QML (Qt Meta-object Language) support. SHA: d2307ce68af4774e643649d2ea68c090a7ebc469

Bug fixes for version 1.04:

- (CI) Version/archatecture prefix added to the flatpak artifact. SHA: d2307ce68af4774e643649d2ea68c090a7ebc469
- JS syntax highlighting improved on dark themes. SHA: d2307ce68af4774e643649d2ea68c090a7ebc469

## [1.03] - 2026-08-16

What's new in Devpad 1.03:

- New project panel open file behavior setting (Single-click or Double-click) SHA: 583e49a27376c1fc4109bf8f1ab5d7627ccec521
- Update CI from using Node.js 20 to Node.js 24 SHA: 583e49a27376c1fc4109bf8f1ab5d7627ccec521

Bug fixes for version 1.03:

- Fix "Check for Updates" bug. SHA: 583e49a27376c1fc4109bf8f1ab5d7627ccec521
- Fix macOS CI job "taps are not trusted" SHA: 583e49a27376c1fc4109bf8f1ab5d7627ccec521
- Terminal panel when in tab mode goes blank after clicking the OK button in the Options dialog. SHA: 6109df7c3e930c8d82ba2ab536a994a519207fc8
- Terminal panel when in tab mode is being made visible on startup when it should not be according to the setting. SHA: 6109df7c3e930c8d82ba2ab536a994a519207fc8

## [1.02] -2026-08-07

What's new in Devpad 1.02:

- (Platform=Windows) The menu bar can now be displayed inside the titlebar, matching the look of modern editors. SHA: 75b965add2c46ed43f3e4c8bebb6c79c01b53846
- (Platform=macOS) Support for opening files from Finder/dock. SHA: da64180422047105e2338341bb256c5c30bb0f34

Bug fixes for version 1.02:

- (Platform=Windows) Failed to start because non-Qt runtime DLLs (libicudt78.dll, libgraphite2.dll, libbrotlicommon.dll, libintl-8.dll) were missing from the deployed dist. SHA: 03895853bb29f8f5f705fa873f2b421d49a1a112
- (Platform=Windows) Terminal bug when running external tools. SHA: b43958113580e834226a8365b36099e8bf840e38
- (Platform=macOS) "Check for Updates" Is in a normal, extra "Devpad" menu when it should be in the App menu. SHA: 3a82aa8f5c1b1dec26c3120e5c82d5ffd6d90132
- (Platform=macOS) "Fullscreen" action is redundant. SHA: 489654b282024e35558f762ebda2eb7ff890147b
- (Platform=macOS) "Options" action shows up in the "Tools" menu after adding an external tool. SHA: eb5733e69e1a6f24022e75867bca3712a6a3470b

## [1.01] - 2026-08-03

What's new in Devpad 1.01:

- Added support for .gitignore based filtering in the project panel. SHA: ce1c7ea7f082dfb1cf1e6834389eebce56183b56
- Improved project panel SHA: 20ef46e6a9aff943fb54ca18d0fd94a8ac719329
- New Check for Updates support. SHA: a228f1ada7f454f736aed447f8930cd212be9f76

Bug fixes for version 1.01:

- User Interface font and font size are not set current on startup.  SHA: 87485e43618f39c6edcd82e01f0d5eca8f773a76
- (Platform=Linux) Ctrl+Shift+O keyboard shortcut is not working (Open Folder action) SHA: caeb20af10aec5b5113c4b49ec6ce64ecfd53824
- Fixed Tab switch from project panel. SHA: 4716ca5622780fea01d6f49db75f9c52f9c93ab8
- On startup, the terminal panel should load and be ordered before the code editors. SHA: 92546a4b3bb3fcce78f930a9e00c0117136ecdfe
- (Platform=Windows) (MSI Installer) Desktop shortcut opens the install folder when it should open Devpad.exe SHA: 1ffb31dbe473978f60dd17bb728a074739267c20

## [1.0.0] - 2026-07-06

- Initial release of Devpad