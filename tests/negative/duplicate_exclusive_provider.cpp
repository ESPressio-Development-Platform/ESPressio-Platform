#include "ESPressio_Platform.hpp"

using namespace ESPressio::Platform;

struct BackendA final : Backend {};
struct BackendB final : Backend {};

struct A final : ProviderDeclaration<BackendA, CapabilitySet<Capability::Execution>> {};
struct B final : ProviderDeclaration<BackendB, CapabilitySet<Capability::Execution>> {};

using InvalidPlatform = Composition<A, B>;
static_assert(InvalidPlatform::IsValid);
