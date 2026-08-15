# Handwritten layouts, docks, and MDI

## Learning objectives

- Build adaptive widget hierarchies without absolute coordinates.
- Use stretch, size policy, margins, and splitters deliberately.
- Manage unique and non-unique documents with `QMdiArea`.
- Persist and recover a `QMainWindow` workspace.

## Main shell

`MainWindow` uses `QMdiArea` as its central widget. The device explorer and
property inspector are left/right `QDockWidget`s; diagnostics is a bottom dock.
This follows QMainWindow's native docking system instead of implementing custom
dragging, floating, and persistence.

## Document identity

`DocumentManager` assigns stable keys:

```text
monitor:<device-id>      unique
protocol:<device-id>     unique
history:<device-id>:N    non-unique
```

Opening a unique document activates its existing `QMdiSubWindow`. History
queries are intentionally independent so operators can compare ranges.

## Layout principles demonstrated

- Page margins come from `UiMetrics`, not repeated magic values.
- Live summary columns use grid stretch rather than fixed widths.
- The chart/table division uses `QSplitter`, allowing operator adjustment.
- Tables use `QHeaderView` resize policies.
- Controls receive minimum hints only where collapse would make them unusable.
- All pages remain font- and DPI-aware.

## Workspace persistence

`saveGeometry` stores the top-level window. `saveState(version)` stores docks and
toolbars. The explicit version allows future layouts to invalidate incompatible
state. “Reset Dock Layout” is a recovery path for corrupt or off-screen layouts.

## Exercises

1. Add an Alarm document that is globally unique rather than device-unique.
2. Persist MDI mode separately from dock state.
3. Add keyboard shortcuts for next/previous document.
4. Replace one fixed-looking arrangement with stretch factors and test at 125%,
   150%, and 200% scale.

