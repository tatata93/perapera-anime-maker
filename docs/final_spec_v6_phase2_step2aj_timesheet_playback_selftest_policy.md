# Phase 2 Step 2-aj: Timesheet playback helper selftest policy

- Add small pure helper coverage for timesheet preview playback and range playback behavior.
- Keep the test independent from ImGui windows, SDL startup, project loading, and preview cache warmup.
- Cover frame clamping, linear wrap, ping-pong bounce, range normalization, range wrap, and single-frame stability.
- This closes the new helper split with lightweight regression coverage before moving on to later Phase work.
