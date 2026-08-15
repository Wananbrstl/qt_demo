# Declarative programming in a handwritten Qt/C++ application

Declarative programming is not synonymous with QML. A system is declarative
when code or data states **what should hold**, while a reusable engine decides
**how to make it hold**. DeviceStudio intentionally keeps widgets in C++ but
uses declarative ideas at stable policy boundaries.

## Four levels used by this project

### 1. Build graph

`CMakeLists.txt` declares targets, source membership, resource files, and link
dependencies. CMake decides command ordering. Prefer target-scoped declarations
such as `target_link_libraries` over global compiler and linker mutation.

### 2. Runtime policy

`config/application.json` declares the default device endpoint, heartbeat
policy, frame bound, database batch interval, and retention policy. The values
are data, not branches spread across widgets. The next milestone will load this
file through a typed configuration boundary and validate all ranges before any
worker thread starts.

The desired dependency direction is:

```text
JSON document -> parser/validator -> immutable typed settings -> services
```

Widgets should receive validated values. They should not perform JSON lookup,
invent fallback values, or know where the configuration file lives.

### 3. Persistent schema evolution

`migrations/001_initial.sql` declares the database shape. `DatabaseWorker`
provides the imperative mechanism: transaction, execution, rollback, and
version tracking. A migration is append-only after release; editing an already
applied migration destroys the meaning of a schema version.

### 4. UI constraints

Qt layouts are constraint declarations even when constructed in C++:

```cpp
auto* form = new QFormLayout;
form->addRow(tr("Temperature"), temperatureValue);
form->addRow(tr("Spindle speed"), spindleValue);
```

This states row relationships. The layout engine owns geometry calculation for
different fonts, translations, DPI settings, and window sizes. Calling
`setGeometry()` for every child would encode one accidental screenshot instead
of the intended relationships.

## Choosing declarative versus imperative code

Use a declarative representation when all of the following are mostly true:

- the information is policy or structure rather than a one-off algorithm;
- it benefits from validation, diffing, tooling, or runtime substitution;
- ordering and resource lifetime can be delegated to a well-defined engine;
- invalid states can be rejected at one boundary.

Prefer explicit imperative C++ for lifecycle transitions, error recovery,
thread-affinity work, and protocol state machines. Hiding a TCP reconnect state
machine in callbacks described by configuration makes sequencing harder to
audit, not more declarative.

## A typed configuration boundary

Do not pass a `QJsonObject` through the application. Parse once into value types:

```cpp
struct NetworkSettings {
    std::chrono::milliseconds heartbeatInterval;
    std::chrono::milliseconds heartbeatTimeout;
    qsizetype maximumFrameBytes;
};

struct ApplicationSettings {
    DeviceEndpoint endpoint;
    NetworkSettings network;
};
```

Validation expresses cross-field invariants:

```cpp
if (settings.network.heartbeatTimeout
        <= settings.network.heartbeatInterval) {
    return error("heartbeatTimeout must exceed heartbeatInterval");
}
```

After validation, prefer immutable settings or constructor injection. This turns
many runtime checks into a single startup decision and makes tests independent
of process-global configuration.

## Exercise: data-driven device catalog

Replace the one demo device with a JSON array, but keep `DeviceTreeModel`
unaware of JSON:

1. Define `DeviceDefinition` as a value type.
2. Parse and validate `QVector<DeviceDefinition>` at startup.
3. Inject the vector into the application service and model.
4. Report duplicate IDs and invalid ports as structured diagnostics.
5. Add a test that supplies an in-memory JSON document.

The exercise is complete only when malformed configuration cannot partially
start network or database workers.
