# Timesheet Rebuild Step 7.15.4: native Windows image path lookup fix

PowerShell の `Test-Path -LiteralPath` で存在する画像が、Scene Plate UI では `画像未検出 / image file not found` になる問題を修正する。

## 変更

- 画像パス正規化で半角/全角の円記号を `\\` へ寄せる。
- Windows では UTF-8 変換に失敗した場合、ANSI codepage 変換へフォールバックする。
- UI側の存在確認で `GetFileAttributesW` に加え、ASCII/ANSIパスを `GetFileAttributesA` で確認する。
- ScenePlateImageCache 側も同様に `GetFileAttributesW` / `GetFileAttributesA` のフォールバックを行う。
- Scene Plate管理UIへ `Win32確認: 入力=OK/NG / 解決=OK/NG` を表示し、入力文字列そのものと解決後パスのどちらで失敗しているか確認できるようにした。

## 今回やらないこと

- ファイル選択ダイアログ
- 素材コピー管理
- PNG/MP4出力反映
- Project保存/読込統合
