# Learning roadmap

## Stage 0 — Reproducible environment and declarative boundaries

1. Run `scripts/bootstrap-wsl.sh` and read `setup/01_wsl_vscode.md`.
2. Read `declarative/01_declarative_boundaries.md`.
3. Explain which project artifacts declare policy and which components execute it.
4. Verify that VS Code launches the same Debug binary produced by the preset.

## Stage 1 — Qt object model and layouts

1. Read `qt/01_qobject_event_loop.md`.
2. Trace `main.cpp → MainWindow → DocumentManager`.
3. Rebuild one summary section with `QFormLayout`, then compare resize behavior.
4. Use QObject Inspector or debugger to inspect parent/child ownership.

Expected result: you can explain why child widgets use raw observer pointers and
why the application service is parented to the main window while worker objects
are not.

## Stage 2 — MDI, docks, and Model/View

1. Read `qt/02_handwritten_layout_mdi.md` and `qt/03_model_view_threads.md`.
2. Switch between tabbed and subwindow MDI modes.
3. Open two history documents and verify that monitor/protocol documents remain
   unique while history documents do not.
4. Add a filter to the telemetry table without changing its source model.

## Stage 3 — TCP and protocol framing

1. Run `test_protocol` before starting the GUI.
2. Read all guides in `network/`.
3. Put a breakpoint in `FrameDecoder::append` and feed a frame one byte at a time.
4. Corrupt a simulator payload and observe the CRC error in Diagnostics.

## Stage 4 — SQLite

1. Read both database guides.
2. Inspect the generated database under `QStandardPaths::AppLocalDataLocation`.
3. Compare one-row-per-transaction with the existing 250 ms batch transaction.
4. Add a migration instead of editing migration 001 after data exists.

## Stage 5 — Integration and failure recovery

1. Connect the GUI, start the spindle, then kill the simulator.
2. Observe `Online → Reconnecting → Connecting → Online`.
3. Restart the simulator and verify data continues to persist.
4. Query the disconnected time range in a History MDI document.

## Definition of completion

You have completed the initial project when you can explain each thread, each
ownership boundary, every state transition, the TCP frame boundary, and the SQL
transaction boundary without consulting the source.
