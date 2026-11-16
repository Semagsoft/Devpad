# Changelog

## [1.10] - WIP

What's new in Devpad 1.10:



Bug fixes for version 1.10:

- Fix next-gen mode applying every edit twice (text was mutated before
  QUndoStack::push() re-applied it via redo()); large documents (>20k lines)
  no longer render a blank editor. SHA: 3520d90308ac1a4bb52f63d1691664b30be7f363
- Rework the DAP debugger client to speak the Debug Adapter Protocol
  envelope instead of JSON-RPC (request/response/event framing, setBreakpoints
  request shape and 1-based line conversion, execution control resolves the
  last stopped thread id, adapter reverse requests are answered with errors,
  initialize parses the spec body, crash cleanup and auto-restart fixed).
  SHA: 3520d90308ac1a4bb52f63d1691664b30be7f363
- Plugin unload removes plugin-registered actions/docks/panels/objects
  synchronously and deletes the IPlugin instance before unmapping the .so,
  so plugin-code destructors never run from unmapped memory.
  SHA: 3520d90308ac1a4bb52f63d1691664b30be7f363
- Fix local-socket file handoff dropping file paths when delivery of the
  payload was fragmented. SHA: 3520d90308ac1a4bb52f63d1691664b30be7f363
- Refuse lossy decodes when opening files instead of silently corrupting
  non-UTF-8 text in the editor. SHA: 3520d90308ac1a4bb52f63d1691664b30be7f363
- Fix Logger silently swallowing all log messages after a failed log-file
  open. SHA: 3520d90308ac1a4bb52f63d1691664b30be7f363
- Fix theme rename deleting the old file before the new one was written,
  losing the theme on write failure. SHA: 3520d90308ac1a4bb52f63d1691664b30be7f363
- Warn when the session cannot be locked/restored or saved instead of
  silently dropping it. SHA: 3520d90308ac1a4bb52f63d1691664b30be7f363
- Fix Find Symbols duplicating workspace-symbol results fanned out from
  multiple language servers. SHA: 3520d90308ac1a4bb52f63d1691664b30be7f363
- Make BUILD_NEXTGEN=ON link: AUTOMOC could not discover the next-gen
  Q_OBJECT sources behind generator expressions; they are listed explicitly.
  SHA: 116020696023d14328d9e805b59ce6a4c9f3af79
- Fix DAP session ordering against live adapters (verified against
  debugpy): configuration requests now wait for the adapter's initialized
  event or launch/attach acceptance, initialize sends a required adapterID,
  breakpoints are pushed and configurationDone sent at session start, and
  failed setBreakpoints no longer reports an empty verified set.
  SHA: b85f222b1a6c423f9085ce74349ee85528973ca4
- Fix next-gen defects: undo applied every edit twice (via redo() on push),
  oversized documents rendered blank, snippet tab stops landed at the
  insert position, remote-download error path dereferenced a freed reply,
  project-tree delete used a QML/C++ signature mismatch, wrap-mode rows
  were drawn at scrolled offsets, scene-graph side effects are deferred to
  the event loop, and search-match/next-gen QML diagnostics are populated.
  SHA: 11a69e4792a98066e16936282f3b8d0ed46faa6d
- Fix manager/LSP lifecycle defects: session-lock failures are now
  distinguished from 'no session' (with per-key legacy fallback), LSP
  didOpen is deduplicated across rapid language re-detects, references are
  not delivered multiple times after plugin reloads, plugin keyword
  registrations are dropped independently of their languages, keyword
  providers no longer dangle on QHash rehash, remote listing watchdogs
  reclaim ssh processes, and register-only actions restore default shortcuts.
  SHA: ad5bf239f8a931cc1fdeb796df564bf2691c775e
- Fix Find in Files leaking its worker thread on stopped searches, double
  searchFinished emission, and the misreported file count; Options Cancel
  now reverts theme/accent edits; custom theme save refuses same-file-name
  collisions; replace dialog warns on regex back references; .gitignore no
  longer matches substrings; theme QSS failures clear stale style sheets.
  SHA: 44668a09db937f651adef15772287bfa1954883e
- Make BUILD_GHOSTTY default to OFF: the libghostty-vt backend requires
  Zig ≥ 0.17 (not packaged on Ubuntu 24.04), which left every CI job that
  configures with the default failing at 'Could not find ZIG_EXECUTABLE';
  CI, Flatpak and fresh builds now use the qtermwidget/KodoTerm fallback,
  matching the Flatpak and README. Pass -DBUILD_GHOSTTY=ON to opt in.
  SHA: PENDING



## [1.09] - 2026-11-09

What's new in Devpad 1.09:

- New references panel. SHA: 407491d4148e7a9d072dbf07b630f37c6d277ab5
- Custom theme editor. SHA: 407491d4148e7a9d072dbf07b630f37c6d277ab5
- DAP Debugger Integration. SHA: 407491d4148e7a9d072dbf07b630f37c6d277ab5

Bug fixes for version 1.09:

- Fix shell-open files ignoring "Current window" mode. SHA: 407491d4148e7a9d072dbf07b630f37c6d277ab5
- Changed exclude entries from slash-wrapped to bare names. SHA: 407491d4148e7a9d072dbf07b630f37c6d277ab5
- QHash modified during iteration in pollModifications. SHA: 407491d4148e7a9d072dbf07b630f37c6d277ab5

## [1.08] - 2026-11-06

What's new in Devpad 1.08:

- New Mini-map support. SHA: 1cd7ba92fb282233101cbbef690291414cbd9339
- "Reset All Settings" button in Options dialog. SHA: 55a827170c67921e742b432a4852ebfb0426ac97
- Open files from shell and Open folders from shell — each with two modes: New window (default) and Open in current window. SHA: 55a827170c67921e742b432a4852ebfb0426ac97

Bug fixes for version 1.08:

-  SVG fixes. SHA: 1cd7ba92fb282233101cbbef690291414cbd9339
-  Fix for saving remote files. SHA: 75718f32995f5809eaf963713e589a3b41ea7063
-  Fix file save issues. SHA: 55a827170c67921e742b432a4852ebfb0426ac97
-  Fix file watcher issues. SHA: 55a827170c67921e742b432a4852ebfb0426ac97
-  Fix Update Download button doing nothing. SHA: 55a827170c67921e742b432a4852ebfb0426ac97

## [1.07] - 2026-10-27

What's new in Devpad 1.07:

- Add Plugin/extension system SHA: 0adf34758ca8343990947c97470c042d6f3a8c9f
- Add support for loading a remote folder via Open Remote Folder action. SHA: d6cff10ac35b06c8b7e7a8b565878091ac5042d7
- New PHP syntax support. SHA: d6cff10ac35b06c8b7e7a8b565878091ac5042d7

Bug fixes for version 1.07:

- Revert / external-change reload silently kept stale content when a file was open in multiple tabs. SHA: 7bbe615bf6e9bd89080a6e2c37e4fdc37db91864
- Two pure conversion helpers (src/lsp/lsptypes.h/cpp):
byteOffsetToUtf16(lineText, byteOffset) — QScintilla byte offset → LSP UTF-16 character count
utf16ToByteOffset(lineText, utf16Character) — LSP UTF-16 character count → QScintilla byte offset. SHA: 7bbe615bf6e9bd89080a6e2c37e4fdc37db91864

## [1.06] - 2026-10-24

Bug fixes for version 1.06:

- File background watcher file changed outside of editor dialog shows every time there is a change, when it should only show one dialog at any given time and wait for user input before ever showing another. SHA: 3682800a7c93a6a9cb7912b86538a608f20590ae
- TLS error when clicking "Check for updates" in some cases. SHA: b8fc85921fee333bacf857d24aad878f1d82c025
- Unused UI separator removed. SHA: 74dec77dbbaaacb5de794aaed41968ba17c41d4d
- Fix LSP protocol defects in JSON-RPC layer and client lifecycle. SHA: 79111e33e68bd061df5a2ceb752c4d9854d333e4
- Bound memory usage from unbounded network/pipe reads. SHA: 79111e33e68bd061df5a2ceb752c4d9854d333e4
- Stop session restore from resurrecting stale sessions. SHA: 79111e33e68bd061df5a2ceb752c4d9854d333e4

## [1.05] - 2026-09-18

What's new in Devpad 1.05:

- New JSONC support. SHA: 298446238baec715916a1537532f9cf3abb7de8e

Bug fixes for version 1.05:

- Fix for theme issue involving darkmode failure. SHA: 298446238baec715916a1537532f9cf3abb7de8e
- Saved files no longer add UTF-8 Byte Order Mark (BOM: \xef\xbb\xbf). SHA: 298446238baec715916a1537532f9cf3abb7de8e

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