# Timesheet Rebuild Step 3.5 接続スクリプト修正ノート

前回の `apply_timesheet_step3_5_connection.ps1` は、PowerShell 5.1 環境で UTF-8 文字列とC++コード片の埋め込みが壊れ、スクリプト自体が解析エラーになる可能性があった。

今回の `apply_timesheet_step3_5_connection_fixed.ps1` は、PowerShell 側をASCIIのみで構成し、C++に挿入するUI文字列も Unicode escape で表現する。

この修正は `AppDrawingMode.cpp` に TimesheetPanel 表示呼び出しを入れるための差し込みだけを行う。
タイムシート編集、保存接続、キャンバス表示反映、再生/出力反映は行わない。
