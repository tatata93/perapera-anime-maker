# final_spec_v6 Phase 2 Step 2-f: app save connection

Goal:

- Connect the new layout save entry to the normal app save path.
- Keep the current legacy `ProjectIO` save/load path available until the new layout has a verified load path.
- Avoid adding new UI panels or Scene Plate concepts.

Implemented behavior:

- `App::saveProject()` still writes the existing project save format.
- The same save operation also writes the new layout through `saveProjectNewLayoutMinimal()`.
- `App::saveLoadRoundTripCheck()` also writes the new layout before doing the existing legacy load verification.
- The new layout uses:
  - `scene_001`
  - `cut_001`
  - current project cells
  - current app working timesheet
  - project output FPS
  - project timeline total frames

Verification:

- `perapera_anime_maker.exe` builds.
- All current `*selftest.exe` checks pass.

Next step:

- Add a narrow new-layout load/inspection step before replacing the app load path.
