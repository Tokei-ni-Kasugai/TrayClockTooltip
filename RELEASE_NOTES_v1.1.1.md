# TrayClockTooltip 1.1.1

TrayClockTooltip is a lightweight Windows tray clock that can use NTP-based app time.

TrayClockTooltip は、NTP時刻を利用できる軽量なWindowsタスクトレイ時計です。

## Downloads / ダウンロード

- `TrayClockTooltip-1.1.1.zip`  
  Release package. Contains the EXE, README files, and LICENSE.  
  実行ファイル、README、LICENSEを含むRelease用ZIPです。

- `TrayClockTooltip-source.zip`  
  Source package for this release.  
  このReleaseに対応するソース一式です。

- `SHA256SUMS.txt`  
  SHA256 hashes for the ZIP files.  
  ZIPファイルのSHA256を記載しています。

## Changes / 変更点

- Changed `Open log folder` to run ShellExecute in a short-lived helper process.  
  `Open log folder` で、短命ヘルパープロセス側からShellExecuteを実行するようにしました。
- This keeps Explorer's normal folder reuse behavior while reducing memory retained by the resident app.  
  Explorerの通常のフォルダ再利用挙動を保ちつつ、常駐プロセス側に残るメモリを抑えます。

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
82f3a16bd141faa7cb549b508992fce17e216b0677df5cac7562255ec9c20160  TrayClockTooltip-1.1.1.zip
51d506ac7ef6c36ccf348cf2987fc6d489007dca02c2e5ed5e9c33b9bbb5e556  TrayClockTooltip-source.zip
```
