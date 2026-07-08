# final_spec_v6 Phase 2 Step 2-q: lightweight layout inspection

- Keep `ProjectLayoutReadEntry` suitable for project inspection and listing.
- Avoid parsing full layer JSON payloads during inspection because layer files may contain many strokes and Fill bitmap arrays.
- Validate normal layer files through the header/schema area first.
- Fall back to full JSON parsing only when the schema is not found in the small prefix.
