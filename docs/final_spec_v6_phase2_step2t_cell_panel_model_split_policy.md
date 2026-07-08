# final_spec_v6 Phase 2 Step 2-t: CellPanel model helper split

- Split model-oriented CellPanel helpers into `CellPanelModel`.
- Keep ImGui drawing and display-mode state inside `CellPanel.cpp`.
- Reduce `CellPanel.cpp` below the 800-line guideline without changing UI behavior.
- Prefer small, behavior-preserving splits before larger UI refactors.
