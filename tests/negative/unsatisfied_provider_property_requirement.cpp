#include "ESPressio_Platform.hpp"
using namespace ESPressio::Platform;

struct TestBackend final : Backend {};

struct SingleCoreExecution final
    : ProviderDeclaration<
          TestBackend,
          CapabilitySet<
              CapabilityProfile<
                  Capability::Execution,
                  PropertyValue<PropertyKey::ProcessorCount, std::size_t{1}>>>> {};

using NeedTwoCores =
    CapabilityRequirement<
        Capability::Execution,
        PropertyAtLeast<PropertyKey::ProcessorCount, std::size_t{2}>>;

struct NeedsDualCore final
    : ProviderDeclaration<
          TestBackend,
          CapabilitySet<Capability::Synchronization>,
          RequirementSet<NeedTwoCores>> {};

using MustFail = Composition<SingleCoreExecution, NeedsDualCore>;
static_assert(MustFail::IsValid);
int main() { return 0; }
