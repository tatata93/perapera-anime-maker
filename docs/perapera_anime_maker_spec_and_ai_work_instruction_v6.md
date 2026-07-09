# perapera-anime-maker software specification and AI work instruction v6.0

This is the clear entry point for the project specification and AI handoff instructions.

Canonical working spec:

- `docs/final_spec_v6.md`

The historical filename `final_spec_v6.md` remains in place because many work logs and policy docs already refer to it. Its role is now clarified as the top-level software specification plus implementation instruction document.

Current focus:

- Phase 2: file structure revision and Cell concept organization.
- Phase 2 Step 3: Cut-level Timesheet / Cell / Camera connection.

Before making changes, agents should read the Phase 2 appendix in `docs/final_spec_v6.md`, then check:

- `docs/github_progress_for_chatgpt.md`
- `CHATGPT_HANDOFF.md`
- `WORK_LOG.md`
- `DECISIONS.md`

Phase 2 implementation rules:

- Keep startup and project loading light.
- Do not restore `DrawingNewLayoutIO`.
- Do not reintroduce Scene Plate as a separate image panel path.
- Prefer focused selftests before UI expansion.
- Keep source files under roughly 800 lines, or split them before adding more feature bulk.
