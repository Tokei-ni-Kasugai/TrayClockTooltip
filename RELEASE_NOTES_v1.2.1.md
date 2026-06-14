# TrayClockTooltip v1.2.1

TrayClockTooltip is a lightweight Windows tray clock with NTP-based app time.

TrayClockTooltip は、NTP時刻を利用できる軽量なWindowsタスクトレイ時計です。

## Files / ファイル

- `TrayClockTooltip-1.2.1.zip`
  Release ZIP containing the executable, README files, changelogs, and license.
  実行ファイル、README、変更履歴、ライセンスを含むRelease用ZIPです。
- `TrayClockTooltip-source.zip`
  Source ZIP.
  このReleaseに対応するソース一式です。
- `SHA256SUMS.txt`
  SHA256 hashes for the ZIP files.
  ZIPファイルのSHA256を記載しています。

## Changes / 変更点

- Changed the floating clock time display to follow the Windows short time format, with seconds added when omitted.
  フローティング時計の時刻表示を、Windowsの短い時刻形式に従うようにしました。秒がない設定では秒を補完表示します。
- Resized and repositioned the floating clock when Windows date/time format settings change while the app is running.
  アプリ起動中にWindowsの日付/時刻形式を変更した場合、フローティング時計の幅と位置を再計算するようにしました。
- Kept the floating clock within the monitor work area when resizing or dragging near screen edges.
  画面端付近でのリサイズやドラッグ時に、フローティング時計がモニターの作業領域内に収まるようにしました。
- Fixed tray right-click behavior so a pinned floating clock no longer hides when opening the tray menu.
  固定表示中のフローティング時計が、トレイメニューを開いたときに非表示にならないようにしました。

## Notes / 注意事項

- Target: Windows 11.
  対応環境: Windows 11
- The executable is unsigned. Windows SmartScreen or security software may show a warning.
  実行ファイルは未署名です。Windows SmartScreen やセキュリティソフトにより警告が表示される可能性があります。
- Time adjustment asks for UAC confirmation only when needed.
  時刻調整時のみUACによる確認を求めます。
- Startup registration uses the current user's Run registry key and does not require administrator rights.
  自動起動登録は現在のユーザーのRunレジストリキーを利用し、管理者権限は不要です。

## SHA256

```text
SHA256 is listed in SHA256SUMS.txt after release packaging.
```

See `SHA256SUMS.txt` for all ZIP hashes.
