# LICENSE_POLICY

This project may be released for free in the future.  
Therefore, license safety is important.

## Basic Policy

```text
Preferred Licenses
├── MIT
├── BSD-2-Clause
├── BSD-3-Clause
├── Apache-2.0
├── zlib
└── Public Domain
```

## Avoid

```text
Avoid
├── GPL-only libraries
├── GPL-only modules
├── Qt GPL-only modules
├── FFmpeg builds with --enable-gpl
├── FFmpeg builds with --enable-nonfree
├── Unknown source code
├── Unknown license code snippets
└── Copying code from the internet without license confirmation
```

## LGPL

LGPL libraries should be avoided by default.  
If an LGPL library becomes necessary, the project owner must approve it first.

## FFmpeg

FFmpeg is useful, but it has important license conditions depending on build options.  
This project does not link FFmpeg in the initial phase.

The preferred approach is:

```text
FFmpeg Policy
├── Export PNG image sequences
├── Export WAV audio
├── Let users choose an external FFmpeg executable
├── Do not statically link FFmpeg
├── Do not bundle GPL/nonfree FFmpeg builds without review
└── Document all FFmpeg usage before release
```

## Adding a New Library

Before adding a new library, record the following in `DEPENDENCIES.md`.

```text
Required Information
├── Library name
├── Purpose
├── Why it is needed
├── Alternatives
├── License
├── Official URL
├── Free release considerations
└── How it is used in this project
```

## stb_image_write.h

Phase 3CでPNG保存用に `stb_image_write.h` を追加した。

```text
Policy
├── ライセンス系統
│   └── Public domain / MIT style option
├── 使用範囲
│   └── PNG画像の書き出しのみ
├── 配布時の扱い
│   └── vendor/stb/stb_image_write.h を同梱する
└── 注意
    └── 将来、商用/配布形態を変える場合も、依存関係一覧で明示する