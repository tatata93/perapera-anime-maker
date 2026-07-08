# Phase 2 Step 2-k app IO inventory cleanup applyfix

This applyfix corrects the PowerShell parser error caused by `$rel:` in the previous inventory script.

Policy:
- Do not reintroduce DrawingNewLayoutIO.
- Keep core layout IO under `src/io`.
- Generate a local inventory report before reconnecting app save/load.
- Avoid adding logic to large UI files until the real app IO route is confirmed.
