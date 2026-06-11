// src/drawing/AnimationFrame.cpp
//
// AnimationFrameの処理は、Phase 3Dでは AnimationFrame.h 内に inline 実装しています。
//
// 理由:
// 一部環境で AnimationFrame.cpp がリンク対象に入らず、
// AnimationFrame::strokeCount() と AnimationFrame::clearAllLayers() の
// LNK2019 が発生したためです。
//
// このファイルは将来、AnimationFrameの処理が増えたときに再び使います。

#include "drawing/AnimationFrame.h"