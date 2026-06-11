# TrayClockTooltip Test Cases

Manual test cases for the current behavior. Do not use debug execution.

## Preconditions

- Use the latest `TrayClockTooltip.exe` built from this folder.
- Close any running `TrayClockTooltip.exe` before starting a new test run.
- Confirm only one instance is running during each test.
- Windows Time should have a configured NTP server unless the test explicitly covers unavailable NTP.

## Basic Startup

| ID | Case | Steps | Expected |
| --- | --- | --- | --- |
| ST-01 | Start app | Launch `TrayClockTooltip.exe`. | App starts without visible main window. Tray icon appears. |
| ST-02 | Single instance | Launch `TrayClockTooltip.exe` while it is already running. | No second tray icon appears. Existing app keeps running. |
| ST-03 | Exit from tray | Right-click tray icon, choose `Exit`. | App exits and tray icon disappears. |

## Tray Hover And Floating Clock

| ID | Case | Steps | Expected |
| --- | --- | --- | --- |
| HV-01 | Hover preview appears | Move mouse over tray icon. | Floating clock appears like a tooltip preview. Standard Windows tooltip text is not shown. |
| HV-02 | Hover preview position | Hover tray icon. | Floating clock appears centered horizontally above the tray icon when the tray icon rectangle can be obtained. If not, it falls back to cursor-based positioning. |
| HV-03 | Hover preview stays while over clock | Hover tray icon, then move mouse upward into the floating clock. | Floating clock remains visible. |
| HV-04 | Hover preview remains when moving back down toward icon | Hover tray icon, move into floating clock, then move downward toward the tray icon within the tray icon X range. | Floating clock remains visible. |
| HV-05 | Hover preview hides when leaving clock sideways | Hover tray icon, move into floating clock, then move left or right outside the floating clock. | Floating clock hides. |
| HV-06 | Hover preview hides when leaving clock upward | Hover tray icon, move into floating clock, then move upward outside the floating clock. | Floating clock hides. |
| HV-07 | Hover preview hides when leaving icon X range | Hover tray icon, then move mouse horizontally away from the tray icon without pinning. | Floating clock hides. |
| HV-08 | Hover preview does not flicker excessively | Move mouse slightly within the tray icon. | Floating clock remains stable and does not visibly re-create or flicker repeatedly. |

## Floating Clock Pinning And Dragging

| ID | Case | Steps | Expected |
| --- | --- | --- | --- |
| FL-01 | Pin from hover | Hover tray icon, then left-click tray icon. | Floating clock changes from hover preview to pinned display. |
| FL-01A | Pinned indicator | Pin the floating clock. | Small indicators appear on both sides of the floating clock. |
| FL-02 | Return unmoved pinned clock to hover behavior | Hover tray icon, left-click the tray icon to pin the floating clock, then left-click the tray icon again without moving the mouse. | Floating clock remains visible but internally returns to hover preview behavior. It hides after the mouse leaves the hover-retention area. |
| FL-03 | Show pinned clock directly | Left-click tray icon while floating clock is hidden. | Floating clock appears pinned. |
| FL-04 | Drag pinned clock | Pin floating clock, drag it with left mouse button. | Floating clock moves with the cursor and stays where dropped. |
| FL-05 | Drag hover clock pins it | Hover tray icon, then drag the floating clock. | Floating clock becomes pinned and can be dragged. |
| FL-06 | Right-click hover clock pins it | Hover tray icon, right-click floating clock. | Floating clock remains visible and its context menu opens. |
| FL-07 | Close floating clock | Right-click floating clock, choose `Close`. | Floating clock hides. App continues running in tray. |
| FL-08 | Exit from floating clock | Right-click floating clock, choose `Exit`. | App exits and tray icon disappears. |
| FL-09 | Hide moved pinned clock from tray | Drag the floating clock to move it, then left-click the tray icon. | Floating clock hides, matching the previous behavior for user-positioned pinned clocks. |

## Tray Menu

| ID | Case | Steps | Expected |
| --- | --- | --- | --- |
| TM-01 | Open tray menu | Right-click tray icon. | Tray menu opens. Hover preview closes if it was visible. |
| TM-01A | Suppress hover while tray menu is open | Right-click tray icon to open the menu, move the mouse on the icon while the menu is open, or move away and hover the icon again while the menu is still open. | Floating hover preview does not appear while the menu is open. After the menu closes, normal hover behavior resumes. |
| TM-02 | Default tray menu entries | Right-click tray icon when no 1 second or greater drift is known. | Menu shows `NTP: refresh`, separator, `Exit`. |
| TM-03 | Tray NTP refresh | Right-click tray icon, choose `NTP: refresh`. | App queries the Windows Time configured NTP server again. If the query succeeds and drift is below 1 second, a success notification is shown. |
| TM-04 | Tray NTP refresh disabled while querying | Open tray menu while NTP query is in progress. | `NTP: refresh` is present but disabled/grayed out. |
| TM-05 | Drift tray menu status | Create or simulate Windows time drift of at least 1 second, then let the app query NTP. | Tray menu shows a status-only `Time: +...s (...)` or `Time: -...s (...)` item at the top. It uses normal menu colors, does not highlight on hover, and does not close the menu when clicked. |
| TM-06 | Tray adjust item appears | With drift of at least 1 second detected, right-click tray icon. | `Adjust Windows time (admin)` appears below the status-only `Time: ...` item. Selecting it starts time adjustment with administrator rights. |
| TM-07 | Adjust menu hidden without drift | With drift below 1 second after NTP query, right-click tray icon. | `Adjust Windows time (admin)` is not shown. |
| TM-08 | Advanced forced adjust menu | With drift below 1 second after NTP query, hold Shift and right-click tray icon. | Tray menu shows `NTP: refresh`, `Adjust Windows time (admin)`, separator, and `Exit`. Selecting Adjust starts time adjustment with administrator rights. |
| TM-09 | No forced adjust while querying | While an NTP query is in progress, hold Shift and right-click tray icon. | `NTP: refresh` is disabled and `Adjust Windows time (admin)` is not shown. |
| TM-10 | Startup submenu appears | Right-click tray icon. | Tray menu shows a `Startup` submenu with `Install for this user`, `Add this EXE to startup`, and `Remove startup registration`. |
| TM-11 | Startup submenu disabled states | Open the tray menu when startup is unregistered, when startup points to the current EXE, when startup points to a missing EXE, when startup points to an existing identical EXE, when startup points to an existing different EXE, when the installed EXE exists and matches the current EXE, and when startup points away from the installed EXE. | `Remove startup registration` is disabled when no startup registration exists. `Add this EXE to startup` is enabled only when startup is unregistered, when the registered EXE is missing, or when the registered EXE differs from the current EXE. `Install for this user` is disabled when the current EXE is already the installed EXE and the installed EXE matches it; in that state, use `Add this EXE to startup` to restore startup registration. |
| TM-12 | Advanced open log folder | Hold Shift and right-click tray icon, then choose `Open log folder`. | `Open log folder` is shown only in the Shift menu and opens the active log folder. It remains available even while NTP is querying. |

## Startup Registration

| ID | Case | Steps | Expected |
| --- | --- | --- | --- |
| SU-01 | Install for this user | Choose tray menu `Startup` -> `Install for this user`. | App copies the current EXE to `%LOCALAPPDATA%\Programs\TrayClockTooltip\TrayClockTooltip.exe`, verifies that the copied file matches the current EXE, writes that path to `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\TrayClockTooltip`, starts the installed EXE after the current process exits, and does not show UAC. |
| SU-02 | Register current EXE at startup | Choose tray menu `Startup` -> `Add this EXE to startup`. | App writes the current EXE path to `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\TrayClockTooltip` and shows a success notification. UAC is not shown. |
| SU-03 | Remove startup registration | Choose tray menu `Startup` -> `Remove startup registration`. | App removes `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\TrayClockTooltip` and shows a success notification. UAC is not shown. |
| SU-04 | Install from already installed path | Run the app from `%LOCALAPPDATA%\Programs\TrayClockTooltip\TrayClockTooltip.exe` and open the `Startup` submenu. | `Install for this user` is disabled because the current EXE is already installed. If startup is unregistered, `Add this EXE to startup` is enabled and restores startup registration. |
| SU-05 | Install failure | Make the install destination unavailable or unwritable, then choose `Install for this user`. | App continues running and shows a failure notification. Existing startup registration is not intentionally removed. |
| SU-06 | Installed EXE differs from current EXE | Place a different EXE at `%LOCALAPPDATA%\Programs\TrayClockTooltip\TrayClockTooltip.exe`, then open the tray menu from another copy. | `Install for this user` remains enabled so the current EXE can update the installed copy. |
| SU-07 | Installed EXE restart handoff | Choose `Install for this user` from a non-installed path. | The installed EXE is launched with an internal parent-wait argument, waits until the original process exits, then starts normally without being blocked by the single-instance mutex. |
| SU-08 | Development install prompt | Launch a non-installed EXE with `--install-prompt` while an installed EXE exists. | App shows a startup confirmation with choices to install this EXE, close existing app instances and run this EXE as portable, or cancel. Installing uses the same verified install and restart handoff as `Install for this user`. |
| SU-08A | Development install prompt portable choice | With the installed EXE running, launch a non-installed EXE with `--install-prompt`, then choose `No`. | App asks the running installed instance to exit normally, does not replace the installed EXE, and continues startup from the non-installed EXE path. |
| SU-09 | Development install prompt skipped | Launch with `--install-prompt` when no installed EXE exists, or when the current EXE is the installed EXE. | App does not show the install prompt and continues normal startup. |
| SU-10 | Update installed EXE while installed instance is running | Start the installed EXE, then run a different EXE and choose `Install for this user` or choose install from the `--install-prompt` dialog. | App asks the running installed instance to exit normally, waits briefly, then replaces the installed EXE. If the running instance cannot exit, install fails without force-killing it. |
| SU-10A | Install prompt while same source EXE is running | Start a non-installed EXE, launch the same EXE with `--install-prompt`, then choose `Yes`. | App asks the already running source-path instance to exit normally, installs the EXE, and starts the installed EXE without being blocked by the single-instance mutex. |
| SU-11 | Restore startup to installed EXE | Make the installed EXE match the current EXE, then set startup registration to a different EXE path and open the tray menu. Choose `Install for this user`. | `Install for this user` is enabled. Selecting it does not need to replace the installed EXE, but it rewrites startup registration to `%LOCALAPPDATA%\Programs\TrayClockTooltip\TrayClockTooltip.exe` and shows success. |
| SU-12 | Delete source folder after install handoff | Extract the Release ZIP to any temporary folder, start that EXE, and choose `Install for this user`. After startup is handed off to the installed EXE, delete the original temporary folder. | The original temporary folder can be deleted while the installed EXE is running. The installed EXE does not keep the source folder as its current directory. |

## Build Script

| ID | Case | Steps | Expected |
| --- | --- | --- | --- |
| BS-01 | Build only | Run `powershell -ExecutionPolicy Bypass -File .\src\build.ps1`. | Build succeeds and writes `dist\TrayClockTooltip.exe`. App is not launched. |
| BS-02 | Build and run | Run `powershell -ExecutionPolicy Bypass -File .\src\build.ps1 -Run`. | Build succeeds, then launches the built EXE. |
| BS-03 | Build and run install prompt | Run `powershell -ExecutionPolicy Bypass -File .\src\build.ps1 -RunInstallPrompt`. | Build succeeds, then launches the built EXE with `--install-prompt`. |

## Floating Clock Menu

| ID | Case | Steps | Expected |
| --- | --- | --- | --- |
| FM-01 | Open floating menu | Right-click floating clock. | Floating clock menu opens. |
| FM-02 | Default floating menu entries | Right-click floating clock when no 1 second or greater drift is known. | Menu shows `Close`, separator, `Exit`. |
| FM-03 | Floating menu has no NTP refresh | Right-click floating clock when no 1 second or greater drift is known. | `NTP: refresh` is not shown. |
| FM-04 | Floating menu during NTP query | Open floating menu while NTP query is in progress. | `NTP: refresh` is not shown. |
| FM-05 | Floating adjust menu appears on drift | With drift of at least 1 second detected, right-click floating clock. | `NTP: refresh` is hidden and `Adjust Windows time (admin)` is shown. |
| FM-06 | Floating menu does not show drift text | With drift of at least 1 second detected, right-click floating clock. | Floating menu does not show `Time: ...`. |
| FM-07 | Advanced forced floating adjust menu | With drift below 1 second after NTP query, hold Shift and right-click floating clock. | Floating menu shows `Adjust Windows time (admin)`, separator, `Close`, separator, and `Exit`. Selecting Adjust starts time adjustment with administrator rights. |
| FM-08 | No forced floating adjust while querying | While an NTP query is in progress, hold Shift and right-click floating clock. | Floating menu does not show `NTP: refresh` or `Adjust Windows time (admin)`. |
| FM-09 | Advanced floating open log folder | Hold Shift and right-click floating clock, then choose `Open log folder`. | `Open log folder` is shown only in the Shift menu and opens the active log folder. It remains available even while NTP is querying. |

## NTP And Clock Behavior

| ID | Case | Steps | Expected |
| --- | --- | --- | --- |
| NT-01 | Startup NTP query | Launch app. | App reads Windows Time `NtpServer` and queries that server over UDP/123 at startup. |
| NT-02 | App clock uses acquired NTP offset | Launch app with successful NTP acquisition. | Floating clock displays the date and time portions using the Windows user formats, arranged as `date time`, based on Windows time plus acquired offset. Seconds are shown even when the Windows time format omits them. |
| NT-03 | Manual NTP refresh updates app offset | Run tray menu `NTP: refresh` when no 1 second or greater drift is known. | App clock updates using the newly acquired NTP offset. If drift is below 1 second, a success notification is shown. |
| NT-04 | NTP unavailable notification | Make NTP unavailable, then launch app or choose tray menu `NTP: refresh`. | Custom notification appears saying NTP time could not be obtained and app clock uses Windows system clock. Any previous NTP offset is discarded. |
| NT-05 | NTP unavailable menu state | Make NTP unavailable, then query NTP. | Tray menu returns to `NTP: refresh`; `Adjust Windows time (admin)` is not shown. |
| NT-06 | NTP query source follows Windows Time config | Change Windows Time `NtpServer`, then choose tray menu `NTP: refresh`. | App queries the configured source, not a hard-coded server. |
| NT-07 | NTP query on unlock/logon | Keep app running, then unlock or log on to the same Windows session. | App starts an NTP query. If a query is already in progress, no duplicate query starts. |

## Drift Notification And Adjustment

| ID | Case | Steps | Expected |
| --- | --- | --- | --- |
| AD-01 | Drift notification appears | Create or simulate Windows time drift of at least 1 second, then query NTP. | Custom notification appears. It states Windows time differs by the drift amount and app clock uses NTP time. |
| AD-02 | Notification auto closes | Trigger drift notification and wait. | Notification closes after about 5 seconds. |
| AD-03 | Notification click starts adjustment | Trigger drift notification, click it. | `Adjust Windows time (admin)` flow starts and UAC prompt appears. |
| AD-04 | Tray adjustment starts UAC | With drift detected, choose the tray menu `Adjust Windows time (admin)` item. | UAC prompt appears for an elevated helper process. |
| AD-05 | Floating adjust starts UAC | With drift detected, choose floating menu `Adjust Windows time (admin)`. | UAC prompt appears for an elevated helper process. |
| AD-06 | UAC cancel | Start adjustment, cancel UAC. | App continues running. No extra notification is shown solely because of cancellation. Drift state remains unless refreshed/adjusted later. |
| AD-07 | UAC approve and adjustment succeeds | Start adjustment, approve UAC, and let the elevated helper succeed. | Elevated helper re-queries NTP and applies freshly acquired time using `SetLocalTime`. The main app hides adjust/status menus and shows a success notification. |
| AD-08 | No captured stale time | Delay at UAC prompt before approval. | Applied time is based on elevated helper's fresh NTP query, not on the time captured before UAC. |
| AD-09 | Elevated helper fails | Start adjustment, approve UAC, and make the elevated helper fail to acquire/apply time. | App continues running, keeps the drift state, and shows a failure notification. |

## NTP And Adjustment Log

| ID | Case | Steps | Expected |
| --- | --- | --- | --- |
| LG-01 | Startup/session NTP log | Launch the app, then log on or unlock the session while the app is running. | `startup`, `logon`, or `unlock` events are appended to `TrayClockTooltip.log` with `OK` and `offset=... source=...` on success, or `FAILED error=ntp source=...` on failure. |
| LG-02 | Manual refresh log | Choose tray menu `NTP: refresh`. | A `refresh` event is appended to the log. If NTP succeeds, the offset uses a sign, at least two integer digits below 100 seconds, and three decimal digits, such as `+00.842s`. |
| LG-03 | Adjust log | Run `Adjust Windows time (admin)` and approve UAC. | The elevated helper appends an `adjust` event. Success logs `OK offset=... source=...`; failure logs `FAILED` with an error detail. |
| LG-04 | UAC cancel log | Start `Adjust Windows time (admin)` and cancel UAC. | No notification is shown for the cancellation. The non-elevated launcher appends `adjust CANCEL error=uac` to the log. |
| LG-05 | Installed log path | Run from `%LOCALAPPDATA%\Programs\TrayClockTooltip\TrayClockTooltip.exe`, then trigger NTP or adjustment. | Log output goes to `%LOCALAPPDATA%\TrayClockTooltip\TrayClockTooltip.log`. Removing startup registration does not delete this log. |
| LG-06 | Portable log path and rotation | Run from any other folder and trigger NTP or adjustment. Grow the log beyond 256KB if rotation needs confirmation. | Log output goes next to the running EXE. When the log exceeds 256KB, it rotates to `TrayClockTooltip.log.1`. |

## Notification Window

| ID | Case | Steps | Expected |
| --- | --- | --- | --- |
| NF-01 | Notification theme | Trigger notification in Windows dark mode and light mode. | Notification follows Windows mode colors. |
| NF-02 | Notification click behavior on NTP failure | Trigger NTP failure notification, click it. | Notification closes. Time adjustment is not started. |
| NF-03 | Notification replaced by new notification | Trigger a notification while one is already visible. | Existing notification closes and the new notification is shown. |

## Theme And Display

| ID | Case | Steps | Expected |
| --- | --- | --- | --- |
| TH-01 | Floating dark mode | Set Windows mode to dark, hover or pin floating clock. | Floating clock uses dark background, light text, and dark-mode border. |
| TH-02 | Floating light mode | Set Windows mode to light, hover or pin floating clock. | Floating clock uses light system info colors. |
| TH-03 | Theme change while running | Change Windows mode while app is running. | Floating clock and notification repaint using the current Windows mode. |
| TH-04 | Text centering | Show floating clock. | Timestamp text is centered horizontally and vertically inside the floating window. |
| TH-05 | Screen edge handling | Move tray icon or cursor near screen edge if possible, then show floating clock. | Floating clock stays within working area. |

## Open Questions / Specification Review

| ID | Case | Current Expected | Reason To Review |
| --- | --- | --- | --- |
| OQ-01 | Standard tooltip suppression | Standard tooltip content should not appear. After opening and closing Windows notification center, hovering the tray icon should still show the floating clock instead of the standard tooltip. | Current implementation registers the tray icon without `NIF_TIP` and clears tooltip text only on `NIN_POPUPOPEN` / `NIN_POPUPCLOSE`. Exact shell rendering is environment-dependent. |
| OQ-02 | Tray icon rectangle unavailable | If `Shell_NotifyIconGetRect` fails, floating clock uses cursor-based fallback positioning. | Need decide whether fallback is acceptable or whether another placement method is required. |
| OQ-03 | Hidden tray icons flyout | Hover behavior inside the hidden icons flyout should show the floating clock. | Positioning and auto-hide may differ depending on Windows tray/flyout behavior. |
| OQ-04 | Taskbar orientation | Floating clock should appear near the tray icon and stay onscreen. | Current placement is optimized for bottom taskbar/tray. Top/left/right taskbar behavior should be reviewed if required. |
| OQ-05 | Multiple monitors / mixed DPI | Floating clock should stay near the tray icon and inside working area. | Needs confirmation on multi-monitor and mixed-DPI setups. |
| OQ-06 | Manual refresh notification on no drift | Manual refresh with drift below 1 second shows a success notification. | This gives feedback for an explicit user action while keeping startup/session success quiet. |
| OQ-07 | NTP failure after previous drift | NTP failure resets the tray menu to `NTP: refresh`, discards the previous offset, and hides adjust menu. | This avoids stale adjust actions and stale app-clock offsets, but it also hides previous drift information. Confirm desired behavior. |
| OQ-08 | Hover preview and right-click tray menu | Right-clicking tray icon hides hover preview before showing tray menu. | This avoids overlap but differs from standard tooltip timing. Confirm preferred behavior. |
| OQ-09 | Hover preview close timing | Hover preview is checked every 250 ms. | Exact disappearance timing may feel slightly delayed or abrupt depending on user preference. |
| OQ-10 | Time format | Date and time portions follow the Windows user formats. Overall display is arranged as `date time`. Seconds are shown even when the Windows time format omits them. | Needs confirmation when Windows date/time format or locale differs. |
| OQ-11 | Width from date/time format | Floating clock width is based on worst-case sample strings for the current Windows date/time format. | Needs confirmation for overseas environments or long AM/PM designators. |
## Codex Test Results

Run date: 2026-06-11

Scope:

- No debug execution.
- Build completed with `powershell -ExecutionPolicy Bypass -File .\src\build.ps1`.
- GUI operation, tray interaction, UAC operation, Windows setting changes, and environment-dependent visual checks were not performed by Codex.
- Results below include only the latest re-run by build verification, source trace review, artifact checks, and README/test-case consistency review.
- Previous Codex result entries were discarded before writing this section.

| Result | IDs | Check |
| --- | --- | --- |
| PASS | ST-02, ST-03, HV-01, HV-02, HV-04, HV-07, FL-01, FL-01A, FL-02, FL-03, FL-04, FL-05, FL-06, FL-07, FL-08, FL-09, TM-01, TM-01A, TM-02, TM-03, TM-04, TM-05, TM-06, TM-07, TM-08, TM-09, TM-10, TM-11, TM-12, FM-01, FM-02, FM-03, FM-04, FM-05, FM-06, FM-07, FM-08, FM-09, NT-01, NT-02, NT-03, NT-04, NT-05, NT-06, NT-07, AD-01, AD-02, AD-03, AD-04, AD-05, AD-06, AD-07, AD-08, AD-09, LG-01, LG-02, LG-03, LG-04, LG-05, LG-06, NF-02, NF-03, TH-03, TH-04, TH-05, BS-01, OQ-01, OQ-02, OQ-06, OQ-07, OQ-08, OQ-09, OQ-10, OQ-11 | Re-executed by source trace/build/artifact review. No NG was found in the cases Codex could execute. |
| Not run by Codex | ST-01, HV-03, HV-05, HV-06, HV-08, SU-01, SU-02, SU-03, SU-04, SU-05, SU-06, SU-07, SU-08, SU-08A, SU-09, SU-10, SU-10A, SU-11, BS-02, BS-03, NF-01, TH-01, TH-02, OQ-03, OQ-04, OQ-05 | Requires GUI/visual/environment operation, registry mutation, file placement, app launch, or another environment-dependent check that Codex did not perform from this session. |

Notes:

- The build succeeded and produced `dist\TrayClockTooltip.exe`.
- Floating menu refresh removal was rechecked: `ID_POPUP_REFRESH_NTP` is absent and only tray `NTP: refresh` remains.
- Standard tooltip suppression was rechecked: tray registration omits `NIF_TIP`, and tooltip text is cleared only on `NIN_POPUPOPEN` / `NIN_POPUPCLOSE`.
- Session NTP refresh was rechecked: `WTS_SESSION_LOGON` and `WTS_SESSION_UNLOCK` call `StartNtpLoad`, with duplicate query prevention handled by `g_ntpQueryInProgress`.
- Advanced Shift adjustment was rechecked by source trace: both tray and floating menus use `CanForceAdjustmentFromMenu`, and the condition excludes active NTP queries.
- Advanced log folder access was rechecked by source trace: both tray and floating Shift menus add `Open log folder`, and it remains independent from the forced adjustment availability check.
- NTP/adjustment logging was rechecked by source trace: startup, manual refresh, logon, unlock, UAC cancellation, elevated adjustment success, and elevated adjustment failure paths append log events without changing notification or menu behavior.
- Log routing was rechecked by source trace: installed-path runs write under `%LOCALAPPDATA%\TrayClockTooltip`, other runs write next to the EXE, and startup registration removal does not delete logs.
- Startup menu wiring was rechecked by source trace: the tray menu owns the `Startup` submenu; disabled states are derived from current HKCU Run, registered EXE existence/binary match, and install state; command handlers call current-user install, current-EXE startup registration, and startup removal helpers.
- Install handoff was rechecked by source trace: copied EXE verification uses byte comparison, then the installed EXE is launched with an internal parent-wait argument before the original process quits.
- Development install prompt was rechecked by source trace: `--install-prompt` prompts when an installed EXE exists and the current EXE is running from another path, and its install choice uses the same verified install/restart core.
- The portable prompt choice was rechecked by source trace: choosing `No` asks existing installed/source-path instances to exit normally before continuing, so the global single-instance mutex does not silently discard the portable run.
- Instance close handling was rechecked by source trace: install updates only send `WM_CLOSE` to running instances whose process path matches the installed EXE or the source EXE, then wait briefly before copying. Force-kill is not used.
- Build script launch helpers were rechecked by source trace. `-Run` and `-RunInstallPrompt` were not executed in this pass because they launch the GUI app.
