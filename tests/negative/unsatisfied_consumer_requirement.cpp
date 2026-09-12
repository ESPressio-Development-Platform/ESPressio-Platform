#include "ESPressio_Platform.hpp"

using namespace ESPressio::Platform;

struct BackendA final : Backend {};
struct ExecutionProvider final
    : ProviderDeclaration<BackendA, CapabilitySet<Capability::Execution>> {};

using Platform = Composition<ExecutionProvider>;
using ConsumerRequirements = RequirementSet<Capability::Execution, Capability::GPIO>;
using MustFail = RequireT<Platform, ConsumerRequirements>;

int main() { return sizeof(MustFail); }
