# TrayClockTooltip v1.1.2

TrayClockTooltip is a lightweight Windows tray clock with NTP-based app time.

TrayClockTooltip は、NTP時刻を利用できる軽量なWindowsタスクトレイ時計です。

## Files / ファイル

- `TrayClockTooltip-1.1.2.zip`
  Release ZIP containing the executable, README files, changelogs, and license.
  実行ファイル、README、変更履歴、ライセンスを含むRelease用ZIPです。
- `TrayClockTooltip-source.zip`
  Source ZIP.
  ソースコード一式です。
- `SHA256SUMS.txt`
  SHA256 hashes for the ZIP files.
  ZIPファイルのSHA256を記載しています。

## Changes / 変更点

- Fixed installed app launch so it no longer keeps the portable source folder as its current directory after install.
  インストール後に起動したインストール先EXEが、ポータブル起動元フォルダをカレントディレクトリとして保持しないように修正しました。

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
1011d022c49ae20ed9292ec0baeee9350566f6fdca59e29ece0b533a6d05dc4f  TrayClockTooltip-1.1.2.zip
011faa5dbc16d1b60507b67c81f5943a9709ceded9d79bbee0180da60e7e48e4  TrayClockTooltip-source.zip
```
