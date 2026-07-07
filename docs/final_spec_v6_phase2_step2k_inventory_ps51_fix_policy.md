# final_spec_v6 Phase 2 Step 2-k inventory PowerShell 5.1 fix

This patch fixes the Step 2-k inventory script so it works on Windows PowerShell 5.1.

The previous script used `System.IO.Path.GetRelativePath`, which is not available in the user's PowerShell/.NET environment. This version uses manual full-path prefix removal instead.

It also avoids PowerShell interpolation forms such as `$rel:` and uses `-f` formatting where colons follow variable values.

No C++ source logic is changed by this fix.
