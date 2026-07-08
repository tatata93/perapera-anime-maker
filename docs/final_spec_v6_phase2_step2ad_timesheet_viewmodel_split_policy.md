# Phase 2 Step 2-ad: split timesheet panel ViewModel builder

- Moved the small TimesheetPanelViewModel creation helper out of AppDrawingMode.cpp.
- Kept App state ownership and input flow in AppDrawingMode.cpp.
- Did not restore DrawingNewLayoutIO.
- Did not add a new selftest target.
