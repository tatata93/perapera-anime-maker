# final_spec_v6 Phase 2 Step 2-n: new layout load fidelity

- Improve `ProjectLayoutLoadEntry` rather than restoring old `ProjectIO`.
- Restore saved brush engine, stroke style values, Fill bitmap data, and layer type from layer JSON.
- Apply `Cut.cellZOrderKeys` when rebuilding `Project.cells` and `Project.cellOrder`.
- Reflect loaded scene/cut metadata into the minimal `Project` result where available.
- Keep the test focused on save -> load round-trip data fidelity.
