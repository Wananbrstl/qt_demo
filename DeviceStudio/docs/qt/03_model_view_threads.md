# Model/View and UI update boundaries

## Why Model/View

`QTableWidget` stores presentation items inside the widget and becomes awkward
for large or shared data sets. `TelemetryTableModel` owns domain samples while
`QTableView` owns selection, scrolling, headers, and delegates.

## Required model protocol

Rows must be announced before the underlying container changes:

```cpp
beginInsertRows({}, 0, 0);
samples_.prepend(sample);
endInsertRows();
```

Views may hold persistent indexes and selection state. Editing the vector without
model notifications creates subtle UI corruption.

## Bounded live data

The live model retains 500 rows. SQLite owns long-term history. This separates
operational display needs from storage retention and prevents a week-long window
from consuming unbounded memory.

## Thread boundary

Models and widgets remain in the GUI thread. Worker signals deliver value-type
copies through queued connections. Workers never call a model method directly.

## Exercises

- Add a `QSortFilterProxyModel` that filters temperature above a threshold.
- Implement a delegate that colors high vibration values without storing colors
  in the domain object.
- Write a Qt Test that verifies insert/remove row signals with `QSignalSpy`.

