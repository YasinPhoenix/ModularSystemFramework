# Hey there!

This is a hobby project I’m building for my own entertainment and possibly future use in other projects.

I’m a student trying to push my understanding of embedded systems and C++ design. Expect mistakes, this is intentionally a learning system, not a finished product.

Feel free to use it or modify it if it helps you.

---

## Brief introduction

This project is built for the ESP32 platform and focuses on creating a modular, event-driven runtime system.

The goal is not just to execute code, but to create a system that can:

- detect what modules are available
- adapt behavior based on them
- stay stable even when parts are missing or failing

---

## Features

This is what exists so far:

- Dual-core runtime (separating updates and event handling)
- Typed event system (safe structured communication)
- Event filtering (modules only receive relevant events)
- Priority-based event queues (critical vs non-critical execution)
- Event coalescing and rate limiting (prevents spam overload)
- Module update scheduling (per-module timing control)

---

## More detail

### Dual-core runtime

ESP32 has two cores. This system uses that by splitting responsibilities:

- Core 0 → event processing and dispatch
- Core 1 → module updates and execution

This improves responsiveness and prevents blocking behavior between subsystems.

---

### Typed event system

Instead of using raw bytes or unsafe casting, events are strongly structured using typed payloads.

Each event has:

- a type identifier
- metadata (source, timestamp)
- a structured data payload

This removes undefined behavior and makes the system safer and easier to extend.

---

### Event filtering

Each module declares which events it is interested in.

The system only forwards matching events to each module instead of broadcasting everything to everyone.

This reduces CPU usage and improves scalability.

---

### Priority queues

Events are separated into multiple priority levels:

- HIGH → critical system events
- NORMAL → standard communication
- LOW → high-frequency sensor updates

Higher priority events are always processed first.

---

### Event coalescing and rate limiting

To prevent system overload:

- Rate limiting reduces how often a module can emit events
- Coalescing ensures repeated events are merged instead of queued

This keeps the system stable under heavy sensor or network load.

---

### Module update scheduling

Each module defines how often it should run.

Instead of executing every module every loop, the system schedules updates based on time intervals, improving efficiency and predictability.

---

## Code Structure

```cpp
/core
    system.h
    imodule.h
    module_registry.h
    event_queue.h
    event_types.h
    event_priorities.h

/modules
    temp_sensor.h
    wifi_module.h

/app
    app.ino
```

## Personalization

If you want to use this project in your own setup, there are a few places you’ll likely need to modify.

### event_types.h

This is where all event types are defined.
If you extend the system, you will need to add new event types here so modules can subscribe to them properly.

### event.h

This defines the structure of events.
If you add new event types with custom data, you should also extend the event payload structure or add helper constructors for cleaner usage.

### event_priorities.h

Defines priority levels for events.
If needed, you can extend this to support more granular scheduling (for example: realtime, background, deferred).

### module_capabilities.h

This is intended to define module capabilities.
Right now, it is mostly conceptual and not heavily enforced in logic, but it exists for future expansion where modules can be dynamically matched based on capabilities.
