# TrayClockTooltip

`TrayClockTooltip` は、NTP時刻を利用できる軽量なWindowsタスクトレイ時計です。

バージョン: `1.1.1.0`

## 含まれるファイル

- `TrayClockTooltip.exe`
- `README.txt`
- `README.ja.txt`
- `LICENSE`

## はじめに

`TrayClockTooltip.exe` を実行してください。アプリは通知領域に常駐し、メインウィンドウは表示しません。

実行ファイルは未署名です。Windows SmartScreen やセキュリティソフトにより警告が表示される可能性があります。

## 機能

- 標準ツールチップの代わりに、トレイアイコンからフローティング時計を表示。
- Windows Time に設定されているNTPサーバの時刻をアプリ内時計に利用可能。
- 起動時、ログオン時、ロック解除時にNTP取得。
- タスクトレイメニューから手動NTP再取得。
- Windows時計とのずれが1秒以上の場合に通知。
- UAC確認後、NTP時刻を再取得してWindows時刻を調整。
- トレイメニューから、このユーザー向けの配置と自動起動登録を実行可能。
- NTP取得と時刻調整の履歴を小さなテキストログへ記録。
- フローティング時計と通知の色はWindowsモードに追従。
- 日付と時刻はWindowsのユーザー設定形式に従い、秒がない設定でも秒を補完表示。

## 操作

### タスクトレイアイコン

- hover: フローティング時計を表示。
- フローティング時計表示中に左クリック: 状態に応じて固定表示または非表示。
- 右クリック: タスクトレイメニューを表示。

### フローティング時計

- ドラッグ: 時計を移動。
- 右クリック: フローティング時計メニューを表示。
- `Close`: フローティング時計だけを非表示。
- `Exit`: アプリを終了。

### タスクトレイメニュー

- `NTP: refresh`: Windows Time に設定されているNTPサーバへ再問い合わせ。
- `Adjust Windows time (admin)`: 時刻調整が可能な場合に表示。
- `Startup` -> `Install for this user`: EXEを `%LOCALAPPDATA%\Programs\TrayClockTooltip\` にコピーし、そのコピーを自動起動に登録して起動。
- `Startup` -> `Add this EXE to startup`: 現在実行しているEXEを自動起動に登録。
- `Startup` -> `Remove startup registration`: このアプリの自動起動登録を削除。
- `Exit`: アプリを終了。

### 高度な操作

- トレイアイコンまたはフローティング時計を `Shift + 右クリック`: 既知のずれが1秒未満の場合でも `Adjust Windows time (admin)` を表示。
- トレイアイコンまたはフローティング時計を `Shift + 右クリック`: `Open log folder` を表示。

## NTPと時刻調整

アプリは、Windows Time に設定されているNTPサーバへ問い合わせます。NTPサーバはアプリ内に固定していません。

時刻調整を実行すると、UACの確認後にNTP時刻を再取得し、その結果をWindowsの時刻へ適用します。UACキャンセル時は通知しません。

## ログ

NTP取得と時刻調整の結果は `TrayClockTooltip.log` に追記します。

- インストール先 `%LOCALAPPDATA%\Programs\TrayClockTooltip\TrayClockTooltip.exe` から起動した場合: `%LOCALAPPDATA%\TrayClockTooltip\TrayClockTooltip.log`
- それ以外のパスから起動した場合: 起動EXEと同じフォルダの `TrayClockTooltip.log`

256KBを超えた場合は `TrayClockTooltip.log.1` にローテートします。

## アンインストール

1. ログを削除する場合は、先にトレイアイコンまたはフローティング時計を `Shift + 右クリック` し、`Open log folder` でログフォルダを開いておきます。
2. トレイメニューの `Startup` -> `Remove startup registration` で自動起動登録を削除します。
3. `Exit` でアプリを終了します。
4. インストール運用の場合は `%LOCALAPPDATA%\Programs\TrayClockTooltip\` を削除します。
5. ログが不要な場合は、手順1で開いたログフォルダ、または `%LOCALAPPDATA%\TrayClockTooltip\` を削除します。ポータブル運用の場合、ログはEXEと同じフォルダに作成されます。

## 注意事項

- 対応環境: Windows 11
- 自動起動登録は現在のユーザーのRunレジストリキーを利用し、管理者権限は不要です。
- 主な確認環境は、Windowsの下タスクバー環境です。
- トレイ、タスクバー、複数モニター、DPIまわりの挙動は環境により差が出る可能性があります。

## ライセンス

MIT License です。詳細は `LICENSE` を参照してください。
