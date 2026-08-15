# QObject ownership, signals, and the event loop

## Learning objectives

- Distinguish C++ object lifetime from Qt parent/child ownership.
- Understand direct and queued signal delivery.
- Recognize why the GUI event loop must not block.
- Know when raw pointers are appropriate in Qt code.

## Project mapping

- `MainWindow` owns UI objects through the QObject tree.
- `DeviceApplicationService` owns `QThread` values but workers have no QObject
  parent because parented objects cannot move to another thread.
- `DeviceSession::initialize` creates its socket and timers only after the worker
  has moved to the network thread.

## Ownership rule

When a widget is created with a parent, the parent owns it:

```cpp
stateValue_ = new QLabel(this);
```

`stateValue_` is a non-owning pointer used to reach the label later. Wrapping it
in `std::unique_ptr` would introduce two competing ownership systems. Continue to
use RAII for non-QObject resources and for QObjects without a parent.

## Queued delivery

Signals from `DeviceSession` cross from the network thread to the application
service and UI. Qt posts a metacall event to the receiver's event loop. Custom
argument types are registered in `domain::registerMetaTypes()` before any worker
thread starts.

## Common mistakes

- Creating `QTcpSocket` in the GUI thread, then moving only its wrapper object.
- Calling `waitForConnected`, `waitForReadyRead`, or long SQL queries in a slot on
  the GUI thread.
- Deleting a QObject directly while it is processing an event in another thread.
- Capturing a short-lived reference in a queued lambda.

## Exercise

Log `QThread::currentThread()->objectName()` in the telemetry producer, database
writer, application service, and monitor document. Predict every value first.

