# TrayClockTooltip

`TrayClockTooltip` is a lightweight tray clock with NTP-based app time.

It avoids the .NET Framework runtime dependency and stays resident only in the notification area. The app clock can use time from the NTP server configured in Windows Time. Windows time adjustment asks for UAC confirmation only when adjustment is requested.

Version: `1.1.2.0`

## Features

- Lightweight native app.
- Tray icon with a custom floating clock instead of the standard tooltip text.
- Startup NTP query using the NTP server configured in Windows Time.
- NTP query on session logon and unlock, useful after sleep/resume.
- Manual NTP refresh from the tray menu.
- Manual NTP refresh shows a success notification when NTP succeeds and drift is below 1 second.
- NTP query, time adjustment, and install handoff history are written to a small text log.
- Startup menu for current-user install and startup registration.
- App clock uses the acquired NTP time when NTP succeeds.
- If NTP acquisition fails, the app uses the Windows system clock.
- If the Windows clock differs by 1 second or more, a custom notification is shown and time adjustment becomes available from menus.
- Time adjustment asks for UAC confirmation, then re-queries NTP and applies the freshly acquired time.
- Time adjustment success or adjustment failure is shown with a custom notification. UAC cancellation stays silent.
- Custom notifications close after 5 seconds.
- Floating clock and notification colors follow Windows mode.
- Pinned floating clock shows small side indicators.
- Date and time follow the Windows user format and are displayed as `date time`.
- Seconds are shown even when the Windows time format omits them.

## Controls

### Tray Icon

- Hover: show the floating clock as a tooltip-like preview.
- Left-click while hover preview is visible: pin the floating clock.
- Left-click while an unmoved pinned clock is visible: keep it visible, then hide it when the mouse leaves.
- Left-click while a moved pinned clock is visible: hide the floating clock.
- Right-click: open the tray menu. If a hover preview is visible, it closes before the menu opens.

### Floating Clock

- Drag: move the clock.
- Pinned state: small side indicators are shown while the clock is pinned.
- Right-click: open the floating clock menu.
- `Adjust Windows time (admin)`: shown when drift of 1 second or more is detected. Selecting it starts time adjustment with administrator rights.
- `Close`: hide only the floating clock.
- `Exit`: exit the app.

### Tray Menu

- `NTP: refresh`: query the Windows Time configured NTP server again. This is shown when no drift of 1 second or more is known. If the manual refresh succeeds and drift is below 1 second, a success notification is shown.
- `Time: +...s (...)` / `Time: -...s (...)`: shown as a status-only item when drift of 1 second or more is detected. It uses normal menu colors and does not highlight on hover.
- `Adjust Windows time (admin)`: shown below the status item when drift of 1 second or more is detected. Selecting it starts time adjustment with administrator rights.
- `Startup`:
  - `Install for this user`: copy the EXE to `%LOCALAPPDATA%\Programs\TrayClockTooltip\`, verify the copied file, register that copy for startup, exit the current EXE, and launch the installed EXE.
  - `Add this EXE to startup`: register the currently running EXE for startup.
  - `Remove startup registration`: remove the app's current-user startup registration.
- `Exit`: exit the app.

### Advanced Usage

- `Shift + right-click` the tray icon or floating clock: show `Adjust Windows time (admin)` even when known drift is below 1 second. Use this when you want to manually synchronize Windows time before a time-sensitive reservation, sale, or similar event. It is not shown while an NTP query is in progress.
- `Shift + right-click` the tray icon or floating clock: show `Open log folder` for checking NTP and adjustment history.

## NTP And Time Adjustment

The app queries the NTP server configured in Windows Time. It does not hard-code an NTP server.

When adjustment is requested, the app asks for UAC confirmation, re-queries NTP, and applies the result to Windows time.

## Log

NTP query, time adjustment, and install handoff results are appended to `TrayClockTooltip.log`.

- Installed app path `%LOCALAPPDATA%\Programs\TrayClockTooltip\TrayClockTooltip.exe`: `%LOCALAPPDATA%\TrayClockTooltip\TrayClockTooltip.log`
- Other paths, including portable or development builds: `TrayClockTooltip.log` next to the running EXE
- Install handoff events are always written to `%LOCALAPPDATA%\TrayClockTooltip\TrayClockTooltip.log` so they remain available after deleting the source folder.

The log uses one line per event:

```text
2026-06-11 16:20:31.123  startup  OK      offset=+00.842s  source=ntp.nict.jp
2026-06-11 16:30:00.000  unlock   FAILED  error=ntp  source=time.windows.com
```

When the log grows beyond 256KB, it is rotated to `TrayClockTooltip.log.1`. Log write failures do not interrupt the app.

## Uninstall

1. If you want to delete logs, first `Shift + right-click` the tray icon or floating clock and use `Open log folder` to open the log folder.
2. Use `Startup` -> `Remove startup registration` from the tray menu to remove startup registration.
3. Use `Exit` to close the app.
4. If you used install mode, delete `%LOCALAPPDATA%\Programs\TrayClockTooltip\`.
5. If logs are no longer needed, delete the folder opened in step 1, or `%LOCALAPPDATA%\TrayClockTooltip\`. In portable mode, logs are created next to the EXE.

## Notes

- The app is mainly verified on Windows 11 with the taskbar at the bottom.
- Standard tooltip suppression depends on Windows behavior. Timing may vary slightly by environment.
- Hidden tray icon flyout behavior, non-bottom taskbar positions, and multi-monitor or mixed-DPI environments may need additional confirmation.
- The executable is unsigned, so Windows SmartScreen or security software may show a warning.
- Startup registration uses the current user's Run registry key and does not require administrator rights.
- Startup menu items are disabled when they would not change the current startup state.
- When updating the installed EXE from another copy, the app asks already running installed/source-path instances to exit before replacing it.

## Release Package

The release ZIP contains:

- `TrayClockTooltip.exe`
- `README.txt`
- `README.ja.txt`
- `CHANGELOG.txt`
- `CHANGELOG.ja.txt`
- `LICENSE`

The release page lists SHA256 hashes for the release ZIP and source ZIP.

The website files are in `docs` for GitHub Pages.

Release-oriented Japanese documents are maintained as `RELEASE_README.ja.md` and `RELEASE_CHANGELOG.ja.md`.

Developer architecture notes are in `ARCHITECTURE.md`.

## License

MIT License. See `LICENSE`.

## Build

Install MinGW-w64, then run:

```powershell
powershell -ExecutionPolicy Bypass -File .\src\build.ps1
```

The executable is written to:

```text
dist\TrayClockTooltip.exe
```

Development helpers:

```powershell
powershell -ExecutionPolicy Bypass -File .\src\build.ps1 -Run
powershell -ExecutionPolicy Bypass -File .\src\build.ps1 -RunInstallPrompt
```

`-RunInstallPrompt` launches the built EXE with `--install-prompt`. If an installed EXE exists and the built EXE is running from another path, the app asks whether to install the built EXE, close existing app instances and run the built EXE as portable, or cancel.

## Test Cases

- English: `tests\TEST_CASES.md`
- Japanese: `tests\TEST_CASES.ja.md`

The source ZIP includes `RELEASE_SHA256.txt`, which contains the SHA256 hash of the corresponding release EXE.
