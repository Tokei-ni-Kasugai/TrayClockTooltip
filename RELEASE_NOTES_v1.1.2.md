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
- Adjusted install handoff exit handling and added install-stage logging for troubleshooting.
  インストール引き継ぎ時の終了要求処理を調整し、調査用にインストール処理の段階ログを追加しました。
- Install-stage logs are written to the user log folder so they remain available after deleting the portable source folder.
  インストール処理の段階ログは、ポータブル起動元フォルダ削除後も確認できるようにユーザーログフォルダへ記録します。

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
c13f95c2292633b2e28683ff67814f138a8f13f1439ec6d76258a113cf7499e3  TrayClockTooltip-1.1.2.zip
cf9417ef57bc4fc6b27f7407788157f29d8a8006d05fc99f9be58573c4efd753  TrayClockTooltip-source.zip
```
