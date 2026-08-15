# SQLite schema, migrations, and queries

## Schema intent

`devices` is the parent entity. Telemetry, commands, and alarms reference a stable
device ID. The composite telemetry index supports the principal query:

```sql
WHERE device_id = ? AND captured_at_ms BETWEEN ? AND ?
ORDER BY captured_at_ms DESC
```

Index design follows query shape; indexes are not added blindly to every column.

## Migration discipline

Migration 001 creates the baseline schema and records its version. Once shared
data exists, never edit an applied migration. Add `002_...sql`, execute it in a
transaction, and advance `schema_version` only after success.

## Prepared statements

Values are bound separately from SQL. This prevents quoting bugs and SQL
injection while allowing the driver to reuse statement plans.

## Pagination

The teaching version uses LIMIT/OFFSET. Large histories should move to keyset
pagination with `(captured_at_ms, id)` to avoid scanning increasingly large
offsets.

## Exercises

- Add a migration for a telemetry quality field.
- Compare query plans before and after removing the composite index.
- Implement keyset pagination while preserving deterministic ordering.

