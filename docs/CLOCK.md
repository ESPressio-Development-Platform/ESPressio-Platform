# ESPressio Platform Clock Contract

The Platform Clock contract is a clean-slate, provider-neutral elapsed-time abstraction. It does not mirror the legacy Clock APIs currently present elsewhere in ESPressio.

## Core model

A Clock provider returns a raw `Platform::Clock::Tick` from `Now()`. The tick domain is described at compile time by:

- `PropertyKey::ClockFrequencyHz` — number of tick units per SI second;
- `PropertyKey::ClockCounterWidthBits` — number of meaningful low-order counter bits;
- optional `PropertyKey::ClockResolutionNanoseconds` — strongest effective resolution the provider can guarantee at compile time.

The contract deliberately does not force providers to convert native counters into nanoseconds. This preserves exact native timebases such as 32,768 Hz RTC oscillators and FreeRTOS scheduler ticks.

## Monotonic semantics

`Capability::MonotonicClock` means the source is driven by elapsed time and is not adjusted by civil-time, timezone, DST, or wall-clock corrections during one runtime epoch.

Finite counters may wrap at their declared `ClockCounterWidthBits`. `Clock::Elapsed()` performs modular elapsed-tick arithmetic for that width. A caller cannot infer unseen complete counter revolutions; higher-level code that needs an indefinitely extended epoch must sample often enough or explicitly maintain one.

## Interrupt-readable clocks

A provider that claims `Capability::InterruptReadableClock` exposes:

```cpp
Clock::Tick NowFromInterrupt() const noexcept;
```

in the same tick domain as `Now()`.

## Compile-time requirements

Consumers can constrain the selected Clock without naming its implementation:

```cpp
using Requirements = Platform::RequirementSet<
    Platform::Capability::MonotonicClock,
    Platform::Clock::MinimumFrequency<1'000'000ULL>,
    Platform::Clock::MinimumCounterWidth<32U>,
    Platform::Clock::MaximumResolution<1'000ULL>>;
```

A composition that cannot satisfy those requirements is rejected at compile time.

## Third-party and hardware-specific providers

Clock providers are not restricted to ESPressio's Arduino, ESP-IDF, or FreeRTOS backend repositories. A hardware-specific library may satisfy the Platform Clock contract directly:

```cpp
struct ExternalRtcOrigin final : Platform::Backend {};

class ExternalRtcClock final
    : public Platform::Clock::MonotonicProviderDeclaration<
          ExternalRtcOrigin,
          32'768ULL,
          16U,
          Platform::PropertySet<
              Platform::PropertyValue<
                  Platform::PropertyKey::ClockResolutionNanoseconds,
                  30'518ULL>>> {
public:
    Platform::Clock::Tick Now() const noexcept;
};
```

The application may then select that Clock alongside unrelated providers from FreeRTOS, ESP-IDF, Arduino, or other provider origins.

## First-party concretes

The initial Platform family provides:

- `ESPressio-Platform-Arduino`: `Arduino::MicrosClock`, using only `micros()`. It deliberately omits an effective-resolution guarantee because that is Arduino-core-specific.
- `ESPressio-Platform-FreeRTOS`: `FreeRTOS::TickClock`, using `xTaskGetTickCount()` and `xTaskGetTickCountFromISR()`, with frequency and width derived from the kernel configuration.
- `ESPressio-Platform-ESP-IDF`: `IDF::EspTimerClock`, using `esp_timer_get_time()` with a 1 MHz tick scale, 1 microsecond guaranteed resolution, and task/ISR readability.

No existing ESPressio consumer is migrated by this tranche. Downstream migration will occur after the current primitives redesign has completed and the resulting architecture is re-audited.
