# TrayClockTooltip 1.1.0

TrayClockTooltip is a lightweight Windows tray clock that can use NTP-based app time.

TrayClockTooltip は、NTP時刻を利用できる軽量なWindowsタスクトレイ時計です。

## Downloads / ダウンロード

- `TrayClockTooltip-1.1.0.zip`  
  Release package. Contains the EXE, README files, and LICENSE.  
  実行ファイル、README、LICENSEを含むRelease用ZIPです。

- `TrayClockTooltip-source.zip`  
  Source package for this release.  
  このReleaseに対応するソース一式です。

- `SHA256SUMS.txt`  
  SHA256 hashes for the ZIP files.  
  ZIPファイルのSHA256を記載しています。

## Changes / 変更点

- Added current-user install and startup registration from the tray menu.  
  トレイメニューから、このユーザー向けの配置と自動起動登録を実行できるようになりました。
- Added NTP and time adjustment history logging.  
  NTP取得と時刻調整の履歴ログを追加しました。
- Added `Open log folder` to the Shift + right-click advanced menus.  
  Shift + 右クリックの高度なメニューに `Open log folder` を追加しました。
- Added advanced Shift + right-click adjustment for manual synchronization even below 1 second of known drift.  
  既知のずれが1秒未満でも、Shift + 右クリックから手動同期を開始できるようになりました。

## Notes / 注意事項

- Target: Windows 11 64-bit.  
  対応環境: Windows 11 64bit
- The executable is unsigned. Windows SmartScreen or security software may show a warning.  
  実行ファイルは未署名です。Windows SmartScreenやセキュリティソフトにより警告が表示される可能性があります。
- Time adjustment asks for UAC confirmation only when needed.  
  時刻調整時のみUACによる確認を求めます。
- Startup registration uses the current user's Run registry key and does not require administrator rights.  
  自動起動登録は現在のユーザーのRunレジストリキーを利用し、管理者権限は不要です。

## SHA256

```text
c0755165287b89b0843ecb5c9f5f2d1dafcb635a47c6e4fbe81df5eafe6af4ff  TrayClockTooltip-1.1.0.zip
7b0aad561b07f4837ccd6ce73f209159a5f05f622e2a2117ed8eb7772f10b213  TrayClockTooltip-source.zip
```
