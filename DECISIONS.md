# DECISIONS

## Phase 1.5 Step 15

- Implemented export modes in `PngExporter` rather than changing the live canvas renderer.
- `Composite` follows the user instruction for this step: all visible layers on a white background.
- `LineTest` exports only Normal layers on a white background.
- `ColorTrace` exports Normal and ColorTrace layers on a white background, preserving stroke colors.
- `LineOnly` exports only Normal layers on a transparent background.
- MP4 uses the same PNG sequence export mode because MP4 generation already consumes exported PNG frames.
