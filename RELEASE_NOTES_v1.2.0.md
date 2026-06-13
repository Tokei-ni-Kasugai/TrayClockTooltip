# TrayClockTooltip v1.2.0

TrayClockTooltip is a lightweight Windows tray clock with NTP-based app time.

TrayClockTooltip は、NTP時刻を利用できる軽量なWindowsタスクトレイ時計です。

## Files / ファイル

- `TrayClockTooltip-1.2.0.zip`
  Release ZIP containing the executable, README files, changelogs, and license.
  実行ファイル、README、変更履歴、ライセンスを含むRelease用ZIPです。
- `TrayClockTooltip-source.zip`
  Source ZIP.
  ソース一式です。
- `SHA256SUMS.txt`
  SHA256 hashes for the ZIP files.
  ZIPファイルのSHA256を記載しています。

## Changes / 変更点

- Added tray-menu notification threshold options: `1 sec`, `3 sec`, `5 sec`, `7 sec`, `10 sec`, and `Off`.
  トレイメニューに通知しきい値の選択を追加しました。
- Threshold changes apply immediately to the latest known NTP offset and update startup registration arguments while preserving the registered EXE path.
  しきい値変更は最新の既知NTPオフセットへ即時反映し、Startup登録がある場合は登録済みEXEパスを維持したまま起動引数だけを更新します。
- `Off` suppresses automatic drift notifications while keeping NTP-based app time active.
  `Off` ではNTP基準のアプリ時計を維持し、自動のずれ通知だけを抑止します。
- Notification text is Japanese on Japanese Windows UI and English otherwise.
  通知本文は、日本語のWindows UI環境では日本語、日本語以外の環境では英語で表示します。
- Uninstalled and unregistered portable startup can show a small confirmation notification when no drift notification is needed.
  未インストールかつ自動起動未登録のポータブル起動で、ずれ通知が不要な場合に起動確認通知を表示することがあります。
- Install handoff now shows the install success notification from the installed EXE after launch.
  インストール引き継ぎ後、インストール先EXEからインストール成功通知を表示します。
- Release README files are copied to the install folder when they are available next to the source EXE.
  起動元EXEの隣に配布用READMEが存在する場合、インストール先フォルダへコピーします。

## Notes / 注意事項

- Target: Windows 11.
  対応環境: Windows 11
- The executable is unsigned. Windows SmartScreen or security software may show a warning.
  実行ファイルは未署名です。Windows SmartScreenやセキュリティソフトにより警告が表示される可能性があります。
- Time adjustment asks for UAC confirmation only when needed.
  時刻調整時のみUACによる確認を求めます。
- Startup registration uses the current user's Run registry key and does not require administrator rights.
  自動起動登録は現在のユーザーのRunレジストリキーを利用し、管理者権限は不要です。

## SHA256

```text
SHA256 is listed in SHA256SUMS.txt after release packaging.
```

See `SHA256SUMS.txt` for all ZIP hashes.
