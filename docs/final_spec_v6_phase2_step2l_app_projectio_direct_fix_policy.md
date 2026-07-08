# final_spec_v6 Phase 2 Step 2-l: AppProjectIO direct layout IO fix

- `src/ui/AppProjectIO.cpp` no longer calls legacy `ProjectIO::save` or `ProjectIO::load` directly.
- App save/load now enters the new layout IO through `ProjectLayoutSaveEntry` and `ProjectLayoutLoadEntry`.
- `DrawingNewLayoutIO` is not used or restored.
- `ProjectIO.*` is not intentionally deleted in this step.
- Whether to remove remaining `ProjectIO.*` remnants is deferred to Step 2-m.
