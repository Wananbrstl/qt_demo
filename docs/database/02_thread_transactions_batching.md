# Database thread, transactions, and batching

## Thread affinity

A `QSqlDatabase` connection must be created, used, closed, and removed in the
same thread. `DatabaseWorker::initialize` runs after the worker moves to the
database thread. The GUI sends value types through queued slots.

## WAL and busy timeout

WAL allows readers and the single writer to coexist more effectively. A busy
timeout handles short lock contention without immediate failure. WAL is not a
substitute for disciplined transaction size.

## Batch transaction

Writing each telemetry sample in its own durable transaction is expensive. The
worker accumulates samples for 250 ms, then writes them in one transaction. If
the transaction fails, the batch returns to the front of the bounded queue.

## Failure rules

- A queue limit prevents memory exhaustion during storage failure.
- Errors are emitted to Diagnostics rather than shown directly by the worker.
- Shutdown stops the timer, flushes pending data, closes the connection, and only
  then removes the named Qt SQL connection.

## Exercises

- Simulate a read-only database and observe retry behavior.
- Add a retention job that deletes old telemetry in bounded chunks.
- Measure transaction throughput for batch sizes 1, 10, 100, and 1000.
- Move `queryHistory` into a read-only connection if long queries delay writes.

