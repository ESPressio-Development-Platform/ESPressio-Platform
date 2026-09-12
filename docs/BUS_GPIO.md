# Bus I/O and GPIO contracts

The Platform bus contracts intentionally compose **bound device endpoints**, not one mutable global bus configuration.

- `I2C::DeviceProviderDeclaration` binds a device tag, address width, raw address, and bus frequency. The structural contract is `Write`, `Read`, and repeated-START `WriteRead` over deterministic byte views.
- `SPI::DeviceProviderDeclaration` binds a device tag, frequency, mode, and bit order. The minimum transfer contract is full-duplex `Transfer(tx, rx, size)`; either side may be discarded. Chip-select ownership is deliberately outside the minimum SPI contract so software GPIO CS and backend-managed CS can coexist.
- `GPIO::ProviderDeclaration` is domain-tagged. `GPIO::DefaultDomain` maps to the original generic `Capability::GPIO`; additional GPIO controllers (for example an expander) use `Capability::GPIODomain<Tag>` and can coexist in the same composition.

Feature capabilities describe stronger guarantees rather than forcing all providers to implement them. GPIO feature tags include pull-up, pull-down, open-drain, bidirectional I/O and per-pin interrupts. I2C repeated START and SPI full-duplex are part of the initial endpoint declarations.

The composition model therefore permits, for example, an Arduino I2C endpoint, ESP-IDF native GPIO, and a future external GPIO expander in the same firmware without any global platform identity.
