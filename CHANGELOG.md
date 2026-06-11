# Changelog

## 1.1.0 - 2026-06-11

- Added current-user install and startup registration from the tray menu.
- Added a development install prompt for replacing or running a non-installed build.
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
