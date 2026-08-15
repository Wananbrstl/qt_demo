PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS schema_version (
    version INTEGER PRIMARY KEY,
    applied_at_ms INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS devices (
    id TEXT PRIMARY KEY,
    display_name TEXT NOT NULL,
    host TEXT NOT NULL,
    port INTEGER NOT NULL CHECK(port BETWEEN 1 AND 65535),
    created_at_ms INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS telemetry_samples (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id TEXT NOT NULL,
    captured_at_ms INTEGER NOT NULL,
    temperature REAL NOT NULL,
    spindle_rpm REAL NOT NULL,
    vibration REAL NOT NULL,
    load_percent REAL NOT NULL,
    FOREIGN KEY(device_id) REFERENCES devices(id)
);

CREATE INDEX IF NOT EXISTS idx_telemetry_device_time
    ON telemetry_samples(device_id, captured_at_ms DESC);

CREATE TABLE IF NOT EXISTS commands (
    request_id TEXT PRIMARY KEY,
    device_id TEXT NOT NULL,
    command_name TEXT NOT NULL,
    state TEXT NOT NULL,
    requested_at_ms INTEGER NOT NULL,
    completed_at_ms INTEGER,
    error_message TEXT,
    FOREIGN KEY(device_id) REFERENCES devices(id)
);

CREATE TABLE IF NOT EXISTS alarms (
    alarm_id TEXT PRIMARY KEY,
    device_id TEXT NOT NULL,
    severity TEXT NOT NULL,
    message TEXT NOT NULL,
    raised_at_ms INTEGER NOT NULL,
    cleared_at_ms INTEGER,
    acknowledged_at_ms INTEGER,
    FOREIGN KEY(device_id) REFERENCES devices(id)
);

INSERT OR IGNORE INTO schema_version(version, applied_at_ms)
VALUES (1, unixepoch('subsec') * 1000);

