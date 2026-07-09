# Phase 2 Step 3-c: completion roadmap and spec entry policy

Purpose:

- Make the top-level spec clearly identifiable as both a software specification and an AI work instruction document.
- Preserve the historical `docs/final_spec_v6.md` filename so existing logs and policy docs remain valid.
- Add a Phase 2 completion roadmap that later agents can follow without guessing the next step number.

Changes in this step:

- Added a clear title/header to `docs/final_spec_v6.md`.
- Added `docs/perapera_anime_maker_spec_and_ai_work_instruction_v6.md` as the recommended entry point.
- Added the remaining Phase 2 Step 3-c to Step 3-h roadmap to the spec.
- Updated README and handoff/progress logs so ChatGPT and other agents can find the current source of truth.

Implementation policy:

- This step is documentation/control only.
- No runtime behavior, save/load format, startup path, preview generation, or UI behavior should change here.
- Phase 2 Step 3-d should next audit and protect Cut-owned Timesheet propagation through app save/load, preview selection, and export setup.
