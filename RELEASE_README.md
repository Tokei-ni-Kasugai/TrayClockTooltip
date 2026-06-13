# TrayClockTooltip

`TrayClockTooltip` is a lightweight Windows tray clock that can use NTP-based app time.

Version: `1.2.0.0`

Website:
https://tokei-ni-kasugai.github.io/TrayClockTooltip/

## Included Files

- `TrayClockTooltip.exe`
- `README.txt`
- `README.ja.txt`
- `CHANGELOG.txt`
- `CHANGELOG.ja.txt`
- `LICENSE`

## Quick Start

Run `TrayClockTooltip.exe`. The app stays in the notification area and does not show a main window.

The executable is unsigned, so Windows SmartScreen or security software may show a warning.

If you extract a downloaded ZIP with Windows Explorer and the extracted EXE is blocked when starting, open the ZIP file properties before extracting and check `Unblock`. This is usually unnecessary when extracting with 7-Zip.

## Features

- Floating clock shown from the tray icon instead of the standard tooltip.
- App clock can use the NTP server configured in Windows Time.
- NTP checks run at startup, logon, and unlock.
- Manual NTP refresh from the tray menu.
- Notification when Windows time differs by the current notification threshold or more.
- Tray menu notification threshold: `1 sec`, `3 sec`, `5 sec`, `7 sec`, `10 sec`, or `Off`.
- Notification text is Japanese on Japanese Windows UI and English otherwise.
- First portable startup can show a small confirmation notification when the app is not installed or registered for startup yet.
- Windows time adjustment with UAC confirmation and fresh NTP re-query.
- Current-user install and startup registration from the tray menu.
- NTP query, time adjustment, and install handoff history are written to a small text log.
- Floating clock and notification colors follow Windows mode.
- Date and time follow the Windows user format, with seconds added when omitted.

## Controls

### Tray Icon

- Hover: show the floating clock.
- Left-click while the floating clock is shown: pin or hide the floating clock depending on its state.
- Right-click: open the tray menu.

### Floating Clock

- Drag: move the clock.
- Right-click: open the floating clock menu.
- `Close`: hide only the floating clock.
- `Exit`: exit the app.

### Tray Menu

- `NTP: refresh`: query the Windows Time configured NTP server again.
- `Adjust Windows time (admin)`: shown when adjustment is available.
- `Notify threshold`: choose the drift threshold for automatic notifications and normal adjustment availability. `Off` suppresses automatic drift notifications while keeping NTP-based app time active.
- `Startup` -> `Install for this user`: copy the EXE to `%LOCALAPPDATA%\Programs\TrayClockTooltip\`, register that copy for startup, then launch the installed EXE and show a success notification.
- `Startup` -> `Add this EXE to startup`: register the currently running EXE for startup.
- `Startup` -> `Remove startup registration`: remove the app's current-user startup registration.
- `Exit`: exit the app.

### Advanced Usage

- `Shift + right-click` the tray icon or floating clock: show `Adjust Windows time (admin)` even when known drift is below the current notification threshold.
- `Shift + right-click` the tray icon or floating clock: show `Open log folder`.

## NTP And Time Adjustment

The app queries the NTP server configured in Windows Time. It does not hard-code an NTP server.

When adjustment is requested, the app asks for UAC confirmation, re-queries NTP, and applies the result to Windows time. UAC cancellation stays silent.

## Log

NTP query, time adjustment, and install handoff results are appended to `TrayClockTooltip.log`.

- Installed app path `%LOCALAPPDATA%\Programs\TrayClockTooltip\TrayClockTooltip.exe`: `%LOCALAPPDATA%\TrayClockTooltip\TrayClockTooltip.log`
- Other paths: `TrayClockTooltip.log` next to the running EXE
- Install handoff events are always written to `%LOCALAPPDATA%\TrayClockTooltip\TrayClockTooltip.log` so they remain available after deleting the source folder.

When the log grows beyond 256KB, it is rotated to `TrayClockTooltip.log.1`.

## Uninstall

1. If you want to delete logs, first `Shift + right-click` the tray icon or floating clock and use `Open log folder` to open the log folder.
2. Use `Startup` -> `Remove startup registration` from the tray menu to remove startup registration.
3. Use `Exit` to close the app.
4. If you used install mode, delete `%LOCALAPPDATA%\Programs\TrayClockTooltip\`.
5. If logs are no longer needed, delete the folder opened in step 1, or `%LOCALAPPDATA%\TrayClockTooltip\`. In portable mode, logs are created next to the EXE.

## Notes

- Target: Windows 11.
- Startup registration uses the current user's Run registry key and does not require administrator rights.
- Notification threshold persistence uses the startup command line. No separate settings file is created.
- The app is mainly verified with the Windows taskbar at the bottom.
- Some tray, taskbar, multi-monitor, or DPI behavior may vary by environment.

## License

MIT License. See `LICENSE`.
