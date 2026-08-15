# Troubleshooting cases

## Case 1 — Breakpoint does not appear to bind

Confirm the selected binary contains the source, is a Debug build, and GDB is not
waiting on debuginfod. The supplied VS Code configuration disables debuginfod for
this project.

## Case 2 — QObject timer warning

If Qt reports “Timers cannot be started from another thread,” inspect where the
timer was created. Worker timers must be created after `moveToThread`, not in a
constructor executed by the GUI thread.

## Case 3 — UI freezes during history query

Verify no widget calls `QSqlQuery` directly. Add timing around the queued request
and worker result. For very large reads, introduce a separate read connection and
stream pages rather than returning millions of rows.

## Case 4 — Some TCP messages disappear

Do not count `readyRead` emissions as frames. Log decoder buffered bytes, frame
length, sequence number, and errors. Run the every-fragment-boundary unit test.

## Case 5 — Dock layout is unusable

Use **View → Reset Dock Layout**. When changing dock object names or layout
structure, increment the `saveState` version so incompatible state is rejected.

