# TrayClockTooltip アーキテクチャ

この文書は、実装を読み直すときの道しるべです。ユーザー向け説明ではなく、開発・保守時に迷いやすい境界をまとめます。

## 基本方針

- ネイティブC/Win32の単一EXEとして動作します。
- アプリ本体は通常権限で常駐し、Windows時刻を書き換える瞬間だけUACで昇格ヘルパーを起動します。
- 設定ファイルは持たず、スタートアップ登録だけHKCUのRunキーを使います。
- UI文字列は英語で統一し、READMEとサイトは日英で用意します。

## プロセス構成

- 通常起動時は通知領域に常駐し、メインウィンドウは表示しません。
- 多重起動は `TrayClockTooltip.SingleInstance` ミューテックスで抑止します。
- `--set-local-time` 付きで起動された場合は、昇格ヘルパーとしてNTP時刻を再取得し、`SetLocalTime` を実行して終了します。

## 時刻取得と表示

- Windows Timeに設定されているNTP参照先を読み取り、そのサーバへ直接問い合わせます。
- NTP取得に成功した場合、アプリ内時計はWindows時刻に取得オフセットを加味して表示します。
- NTP取得に失敗した場合、アプリ内時計はWindowsシステム時計へ戻します。
- 表示フォーマットはWindowsのユーザー設定に従い、秒が含まれない設定でも秒を補完します。

## トレイとフローティング時計

- 標準ツールチップ文字列は使わず、トレイアイコンhoverで独自のフローティング時計を表示します。
- フローティング時計はhoverプレビュー、固定表示、ドラッグ移動の状態を持ちます。
- 固定表示中は左右に小さな固定インジケータを描画します。
- トレイメニュー表示中はhoverプレビューを抑止し、メニューより前面に出ないようにします。

## NTPずれと時刻調整

- 1秒以上のずれを検出した場合、独自通知を表示し、トレイ/フローティングメニューに `Adjust Windows time (admin)` を表示します。
- 1秒未満でも、Shiftを押しながらトレイアイコンまたはフローティング時計を右クリックすると、上級操作として時刻調整メニューを表示します。
- NTP問い合わせ中は、Shift操作でも強制Adjustを表示しません。
- 時刻調整時は、メインプロセスで保持している時刻を渡さず、昇格ヘルパー側でNTP時刻を再取得してから適用します。
- Shiftメニューでは `Open log folder` も表示します。これは診断用の導線であり、通常メニューには出しません。

## NTP/時刻調整ログ

- NTP取得と時刻調整の結果は `TrayClockTooltip.log` へ追記します。ログ失敗はUIや時刻処理へ影響させません。
- インストール先 `%LOCALAPPDATA%\Programs\TrayClockTooltip\TrayClockTooltip.exe` から起動している場合は、ログを `%LOCALAPPDATA%\TrayClockTooltip\TrayClockTooltip.log` に出力します。
- それ以外の場所から起動している場合は、EXEと同じフォルダへ出力します。ポータブル運用や開発ビルドでは、アプリ本体とログが同じ場所に残ります。
- ログは1イベント1行で、日時、イベント種別、結果、`key=value` 形式の詳細を出力します。NTP取得元サーバ名は可変長として扱い、列幅固定にはしません。
- ログが256KBを超えた場合は `TrayClockTooltip.log.1` へローテートします。Startup登録削除時もログは削除しません。

## 自動起動登録

- `Install for this user` は、現在のEXEを `%LOCALAPPDATA%\Programs\TrayClockTooltip\TrayClockTooltip.exe` にコピーし、コピー結果をバイナリ比較で検証し、その配置先をHKCU Runへ登録します。現在のEXEが配置先と異なる場合は、現在のEXEを終了して配置先EXEを起動します。
- `Add this EXE to startup` は、現在実行中のEXEパスをHKCU Runへ登録します。
- `Remove startup registration` は、HKCU Runの `TrayClockTooltip` 値を削除します。
- これらは現在のユーザー範囲だけを変更するため、UACを要求しません。
- 配置済みパスから `Install for this user` を実行した場合は、自分自身の上書きを避け、スタートアップ登録だけを更新します。
- Startupメニューは状態に応じてグレーアウトします。`Install for this user` は、インストール先EXEが存在しない、現在EXEと異なる、Startup未登録、登録先EXEが存在しない、または登録先がインストール先EXEではない場合に有効化します。`Add this EXE to startup` は、自動起動未登録、登録先EXEが存在しない、または登録先EXEが現在EXEと異なる場合だけ有効化します。登録がなければ `Remove startup registration` を無効化します。
- インストール更新時は、インストール先EXEまたは起動元EXEと同じパスで起動している既存インスタンスがあれば、隠しメインウィンドウへ `WM_CLOSE` を送り、通常終了を短時間待ってからコピーします。強制終了はしません。

## 開発用起動補助

- `--install-prompt` は開発用の起動補助です。通常ユーザー向けの常時プロンプトではありません。
- インストール先EXEが存在し、現在EXEがインストール先以外のパスから起動している場合だけ、起動前に確認ダイアログを表示します。
- ダイアログでは、現在EXEをインストールしてインストール先から起動し直すか、既存インスタンスを通常終了して現在EXEをポータブル動作として起動するか、起動をキャンセルするかを選びます。
- `src/build.ps1 -RunInstallPrompt` は、ビルド成功後に `dist\TrayClockTooltip.exe --install-prompt` を起動します。

## ドキュメントとテスト

- ユーザー向けREADMEは `README.md` と `README.ja.md` です。
- 手動テストケースは `tests/TEST_CASES.md` と `tests/TEST_CASES.ja.md` です。
- GitHub Pages用サイトは `docs/index.html` と `docs/index.en.html` です。
- Release ZIPに含める配布用READMEは `RELEASE_README.md` と `RELEASE_README.ja.md` から、`README.txt` と `README.ja.txt` として生成します。古めのWindows系テキストエディタでの閲覧を考慮し、どちらもShift-JISで生成します。
- 配布用READMEには、ユーザーが実行・操作するための情報だけを載せます。ビルド手順、テストケース、GitHub Pages、開発用補助オプション、Source ZIP内の検証ファイルなど、開発者向けの情報は含めません。
