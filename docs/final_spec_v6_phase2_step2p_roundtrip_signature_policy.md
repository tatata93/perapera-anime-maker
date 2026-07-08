# final_spec_v6 Phase 2 Step 2-p: round-trip signature coverage

- Strengthen `appio::projectSignature()` so manual save/load checks cover the new layout fields.
- Include project metadata, cell order, cell motion keys, layer metadata, stroke style, brush engine, and Fill bitmap samples.
- Keep the signature lightweight by sampling Fill bitmap data instead of scanning full bitmap payloads.
- Keep `ProjectStats` as a quick count check and use the signature for data-fidelity changes.
