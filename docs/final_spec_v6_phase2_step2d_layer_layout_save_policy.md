# final_spec_v6 Phase 2 Step 2-d: Layer layout save policy

## Position

Phase 2 Step 2-d adds the first layer-level save helper for the new layout:

```text
scenes/scene_001/cuts/cut_001/cells/cell_A/frames/frame_001/layers/layer_001.json
```

## Decision

- Save layer metadata and strokes into one `layer_NNN.json` file per layer.
- Keep the helper small and independent from UI code.
- Do not modify `ProjectIO` yet.
- Do not add legacy compatibility handling.
- Do not revive Scene Plate / scene panel code.

## Lightweight rule

This step does not add code to large UI files. It also avoids adding duplicate path helper files. The new helper writes only the layer directory below a frame directory provided by the caller.
