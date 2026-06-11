# TrayClockTooltip

[English README](README.md)

`TrayClockTooltip` は、NTP時刻を利用できる軽量なタスクトレイ時計です。

.NET Framework ランタイムへの依存を避け、通知領域に常駐します。アプリ内の時計表示には、Windows Time に設定されているNTPサーバの時刻を利用できます。Windowsの時刻調整が必要な場合だけ、UACによる確認を求めます。

バージョン: `1.1.1.0`

## 機能

- 軽量なネイティブアプリ。
- 標準ツールチップ文字列の代わりに、独自のフローティング時計を表示。
- 起動時に Windows Time に設定されているNTPサーバから時刻を取得。
- セッションのログオン時とロック解除時にもNTP取得。スリープ復帰後の再確認に利用。
- タスクトレイメニューから手動NTP再取得。
- 手動NTP再取得でNTP取得に成功し、ずれが1秒未満の場合は成功通知を表示。
- NTP取得と時刻調整の履歴を小さなテキストログへ記録。
- Startupメニューから、このユーザー向けの配置と自動起動登録を実行可能。
- NTP取得に成功した場合、アプリ内時計は取得したNTP時刻を基準に表示。
- NTP取得に失敗した場合、Windowsシステム時計を利用。
- Windows時計とのずれが1秒以上の場合、独自通知を表示し、メニューから時刻調整を実行可能。
- 時刻調整はUACで確認後、NTP時刻を再取得してから適用。
- 時刻調整の成功、または時刻調整に失敗した場合は独自通知を表示。UACキャンセル時は通知しません。
- 独自通知は5秒で自動クローズ。
- フローティング時計と通知の色はWindowsモードに追従。
- 固定表示中のフローティング時計は、左右に小さな固定インジケータを表示。
- 日付と時刻はWindowsのユーザー設定形式に従い、`日付 時刻` の並びで表示。
- Windowsの時刻形式が秒を含まない場合でも、秒を補完表示。

## 操作

### タスクトレイアイコン

- hover: フローティング時計をツールチップ風プレビューとして表示。
- hoverプレビュー表示中に左クリック: フローティング時計を固定表示。
- 未移動の固定表示中に左クリック: 表示は維持したまま、マウスが離れると非表示になる状態へ戻す。
- 移動済みの固定表示中に左クリック: フローティング時計を非表示。
- 右クリック: タスクトレイメニューを表示。hoverプレビューが出ている場合は、閉じてからメニューを表示。

### フローティング時計

- ドラッグ: 時計を移動。
- 固定表示中: 左右に小さなインジケータを表示。
- 右クリック: フローティング時計メニューを表示。
- `Adjust Windows time (admin)`: 1秒以上のずれが検出された場合に表示。選択すると管理者権限での時刻調整を開始。
- `Close`: フローティング時計だけを非表示。
- `Exit`: アプリを終了。

### タスクトレイメニュー

- `NTP: refresh`: Windows Time に設定されているNTPサーバへ再問い合わせ。1秒以上のずれが検出されていない場合に表示。手動再取得に成功し、ずれが1秒未満の場合は成功通知を表示。
- `Time: +...s (...)` / `Time: -...s (...)`: 1秒以上のずれが検出された場合、表示専用項目として表示。通常メニュー色で表示し、hoverしても選択色に変化しません。
- `Adjust Windows time (admin)`: 1秒以上のずれが検出された場合、表示専用項目の下に表示。選択すると管理者権限での時刻調整を開始。
- `Startup`:
  - `Install for this user`: EXEを `%LOCALAPPDATA%\Programs\TrayClockTooltip\` にコピーし、コピー結果を検証し、そのコピーを自動起動に登録。現在のEXEを終了してインストール先EXEを起動。
  - `Add this EXE to startup`: 現在実行しているEXEを自動起動に登録。
  - `Remove startup registration`: このアプリの自動起動登録を削除。
- `Exit`: アプリを終了。

### 高度な操作

- トレイアイコンまたはフローティング時計を `Shift + 右クリック`: 既知のずれが1秒未満の場合でも `Adjust Windows time (admin)` を表示。予約開始や販売開始など、時刻に敏感な操作の前に手動でWindows時刻を同期したい場合に利用できます。NTP問い合わせ中は表示しません。
- トレイアイコンまたはフローティング時計を `Shift + 右クリック`: NTP取得と時刻調整の履歴を確認するための `Open log folder` を表示。

## NTPと時刻調整

アプリは、Windows Time に設定されているNTPサーバへ問い合わせます。NTPサーバはアプリ内に固定していません。

時刻調整を実行すると、UACの確認後にNTP時刻を再取得し、その結果をWindowsの時刻へ適用します。

## ログ

NTP取得と時刻調整の結果は `TrayClockTooltip.log` に追記します。

- インストール先 `%LOCALAPPDATA%\Programs\TrayClockTooltip\TrayClockTooltip.exe` から起動した場合: `%LOCALAPPDATA%\TrayClockTooltip\TrayClockTooltip.log`
- それ以外のパス（ポータブル版や開発用ビルドなど）から起動した場合: 起動EXEと同じフォルダの `TrayClockTooltip.log`

ログは1イベント1行です。

```text
2026-06-11 16:20:31.123  startup  OK      offset=+00.842s  source=ntp.nict.jp
2026-06-11 16:30:00.000  unlock   FAILED  error=ntp  source=time.windows.com
```

256KBを超えた場合は `TrayClockTooltip.log.1` にローテートします。ログ書き込みに失敗しても、アプリの動作は継続します。

## 注意事項

- 主な確認環境は、Windows 11 64bit の下タスクバー環境です。
- 標準ツールチップ抑止はWindows側の挙動に依存します。環境によっては表示タイミングに差が出る可能性があります。
- 隠れているインジケーター内での挙動、下以外のタスクバー位置、複数モニターや混在DPI環境は追加確認の余地があります。
- 実行ファイルは未署名です。Windows SmartScreen やセキュリティソフトにより警告が表示される可能性があります。
- 自動起動登録は現在のユーザーのRunレジストリキーを利用します。管理者権限は不要です。
- 現在の状態に対して変更がないStartupメニュー項目はグレーアウトします。
- 別の場所にあるEXEからインストール済みEXEを更新する場合、起動中のインストール済みインスタンスと起動元パスの既存インスタンスへ通常終了を依頼してから置き換えます。

## Releaseパッケージ

Release用ZIPには以下を含めます。

- `TrayClockTooltip.exe`
- `README.txt`
- `README.ja.txt`
- `LICENSE`

サイトとGitHub ReleasesにはRelease用ZIPとSource ZIPのSHA256を掲載します。

GitHub Pages用のサイトファイルは `docs` に配置しています。

開発者向けの構成メモは `ARCHITECTURE.md` にあります。

## ライセンス

MIT License です。詳細は `LICENSE` を参照してください。

## ビルド

MinGW-w64 をインストールし、以下を実行します。

```powershell
powershell -ExecutionPolicy Bypass -File .\src\build.ps1
```

実行ファイルは以下に出力されます。

```text
dist\TrayClockTooltip.exe
```

開発用の補助オプション:

```powershell
powershell -ExecutionPolicy Bypass -File .\src\build.ps1 -Run
powershell -ExecutionPolicy Bypass -File .\src\build.ps1 -RunInstallPrompt
```

`-RunInstallPrompt` は、ビルドしたEXEを `--install-prompt` 付きで起動します。インストール済みEXEが存在し、ビルドしたEXEが別パスから起動している場合は、ビルドしたEXEをインストールするか、既存インスタンスを終了してビルドしたEXEをポータブル動作として起動するか、キャンセルするかを確認します。

## テストケース

- 英語版: `tests\TEST_CASES.md`
- 日本語版: `tests\TEST_CASES.ja.md`

Source ZIPには、対応するRelease EXEのSHA256を記載した `RELEASE_SHA256.txt` を含めます。
