# Changelog

## [1.02] -TBD
(WIP)
What's new in Devpad 1.02:

- (Platform=Windows) The menu bar can now be displayed inside the titlebar, matching the look of modern editors. SHA: 75b965add2c46ed43f3e4c8bebb6c79c01b53846

Bug fixes for version 1.02:

- (Platform=Windows) Failed to start because non-Qt runtime DLLs (libicudt78.dll, libgraphite2.dll, libbrotlicommon.dll, libintl-8.dll) were missing from the deployed dist. SHA: 03895853bb29f8f5f705fa873f2b421d49a1a112
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