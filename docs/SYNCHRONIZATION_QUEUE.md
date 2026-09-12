# Synchronization and Queue contracts

`ESPressio-Platform` models synchronization and queueing as compile-time selected provider families rather than runtime registries or factories.

## Synchronization

A synchronization provider supplies concrete nested primitive types. The minimum provider exposes:

- `Mutex` with `Lock()`, `TryLock()`, and `Unlock()`;
- `Signal` with `Give()`, `Wait()`, `TryWait()`, and `Reset()`.

Additional behavior is advertised through capabilities:

- `Capability::StaticSynchronizationStorage` — synchronization objects own their storage statically;
- `Capability::RecursiveMutex` — the provider additionally exposes `RecursiveMutex`;
- `Capability::InterruptSafeSignalling` — `Signal` additionally exposes `GiveFromInterrupt()`.

`GiveFromInterrupt()` returns `Synchronization::InterruptSignalResult`. `HigherPriorityWaiterWoken` reports that the backend unblocked a higher-priority waiter; it does not prescribe a scheduler-specific ISR-yield primitive.

Timed waits are deliberately not part of this first contract. `Lock()`/`Wait()` mean wait until successful, while `TryLock()`/`TryWait()` never wait. This avoids prematurely coupling synchronization to one timeout unit or clock conversion policy.

A selected provider is consumed entirely at compile time:

```cpp
using SyncProvider = Platform::ProviderFor<Capability::Synchronization>;
using Mutex = SyncProvider::Mutex;
using Signal = SyncProvider::Signal;
```

No mutable provider registry or heap-backed factory is required.

## Queue

A queue provider exposes a nested compile-time queue family:

```cpp
using QueueProvider = Platform::ProviderFor<Capability::Queue>;
using EventQueue = QueueProvider::Queue<EventEnvelope, 32>;
```

Every concrete queue type publishes:

- `Depth`;
- `ElementSizeBytes`;
- `PayloadStorageBytes`.

The minimum queue contract supplies:

- `Send(const T&)` — waits until an element can be enqueued;
- `TrySend(const T&)` — immediate enqueue attempt;
- `Receive(T&)` — waits until an element can be dequeued;
- `TryReceive(T&)` — immediate dequeue attempt;
- `Size()`;
- `Reset()`.

Static-storage providers additionally publish `ControlStorageBytes` and `TotalStaticStorageBytes` for each queue type.

Interrupt operations are opt-in capabilities:

- `Capability::QueueSendFromInterrupt`;
- `Capability::QueueReceiveFromInterrupt`.

The corresponding methods return `Queue::InterruptOperationResult`, containing the operation result and whether a higher-priority waiter was unblocked.

## Deterministic resource model

Queue capacity is part of the C++ type. For `Queue<T, Depth>`, payload reservation is therefore known at compile time as `sizeof(T) * Depth`. A static backend can add its fixed control-block storage and expose the complete queue object's static resource requirement without hidden heap fallback.

Synchronization providers can likewise expose backend-specific compile-time storage constants on their concrete primitive types.

## Type safety

Backends that implement byte-copy queues, such as FreeRTOS queues, are expected to constrain queue element types appropriately (for example to trivially copyable types). This is an implementation requirement rather than a universal restriction on every possible Platform queue backend.

## Runtime versus compile-time facts

The selected provider and its supported features are compile-time facts. Runtime contention—whether a mutex is currently held, a signal currently pending, or a queue currently full/empty—remains runtime state.
