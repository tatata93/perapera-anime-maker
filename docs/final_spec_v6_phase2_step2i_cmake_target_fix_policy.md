# final_spec_v6 Phase 2 Step 2-i CMake target fix

## Scope

This package fixes the Step 2-i CMake registration for app-side new-layout drawing IO.

## Reason

`perapera_drawing_new_layout_io_selftest.exe` was not generated because the CMake target was missing from the configured project.

## Policy

- Keep the fix small.
- Do not touch large UI files.
- Do not restore legacy compatibility.
- Do not add duplicate helper layers.
- Keep all added files under 800 lines.
