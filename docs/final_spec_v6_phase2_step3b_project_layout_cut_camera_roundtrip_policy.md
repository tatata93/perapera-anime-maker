# Phase 2 Step 3-b: Project layout Cut camera round-trip policy

- Extend the project layout load selftest so Cut-owned camera metadata is verified through the scene/cut/cell layout save and load path.
- Keep Project-level camera metadata and Cut-level camera metadata distinct in the test so both paths are protected.
- Do not add UI work, startup scanning, or preview generation.
- This is focused coverage for the Phase 2 Step 3 Cut-level Timesheet / Cell / Camera connection.
