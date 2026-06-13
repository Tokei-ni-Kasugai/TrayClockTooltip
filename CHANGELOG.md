# Changelog

## 1.2.0 - 2026-06-14

- Added tray-menu notification threshold options: `1 sec`, `3 sec`, `5 sec`, `7 sec`, `10 sec`, and `Off`.
- Notification threshold now controls drift notifications and normal `Adjust Windows time (admin)` availability while the app clock continues to use acquired NTP time.
- Threshold changes apply immediately to the latest known NTP offset and update startup registration arguments while preserving the registered EXE path.
- Notification text now follows the Windows UI language for Japanese notifications, while non-Japanese UI environments remain English.
- Added a small startup confirmation notification for uninstalled, unregistered portable startup when no drift notification is needed.
- Install handoff now shows the install success notification from the installed EXE after launch.
- Added install-time copying of release README files to the installed app folder when they are available next to the source EXE.
- Updated tests and documentation for configurable notification thresholds.

## 1.1.2 - 2026-06-12

- Fixed installed app launch so it no longer keeps the portable source folder as its current directory after install.
- Adjusted install handoff exit handling and added install-stage logging for troubleshooting.
- Install-stage logs are written to the user log folder so they remain available after deleting the portable source folder.

## 1.1.1 - 2026-06-11

- Changed `Open log folder` to run ShellExecute in a short-lived helper process.
- This keeps Explorer's normal folder reuse behavior while reducing memory retained by the resident app.

## 1.1.0 - 2026-06-11

- Added current-user install and startup registration from the tray menu.
- Added installed EXE replacement flow when installing from another copy.
- Added NTP and time adjustment history logging with 256KB rotation.
- Added advanced `Open log folder` access from Shift + right-click menus.
- Added advanced Shift + right-click adjustment when known drift is below 1 second.
- Updated GitHub Pages and documentation for the new setup and logging behavior.

## 1.0.0 - 2026-06-05

Initial release.

- Lightweight native Win32 tray clock.
- Custom floating clock shown instead of the standard tray tooltip text.
- NTP-based app time using the NTP server configured in Windows Time.
- NTP checks at startup, logon, and unlock.
- Manual NTP refresh from the tray menu.
- Drift notification when Windows time differs by 1 second or more.
- Time adjustment with UAC confirmation and fresh NTP re-query.
- Custom notifications that follow Windows mode and close after 5 seconds.
- Floating clock colors follow Windows mode.
- Pinned floating clock side indicators.
- Windows user date/time format support with seconds added when omitted.
