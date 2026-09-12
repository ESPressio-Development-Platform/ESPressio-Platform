#include "ESPressio_Platform.hpp"

using namespace ESPressio::Platform;

struct BackendA final : Backend {};

struct NeedsClock final
    : ProviderDeclaration<BackendA,
                          CapabilitySet<Capability::Execution>,
                          RequirementSet<Capability::Clock>> {};

using InvalidPlatform = Composition<NeedsClock>;
static_assert(InvalidPlatform::IsValid);
