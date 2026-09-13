#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "ESPressio_CompositionFramework.hpp"

namespace ESPressio::Platform {

namespace CompositionFramework = ESPressio::System::CompositionFramework;

struct CompositionDomain final : CompositionFramework::Domain {};

/** Base tag for platform backend identities. */
struct Backend {};

template <typename T>
inline constexpr bool IsBackendV = std::is_base_of_v<Backend, T> && !std::is_same_v<Backend, T>;

using ExclusiveCapability = CompositionFramework::ExclusiveCapability<CompositionDomain>;
using SharedCapability = CompositionFramework::SharedCapability<CompositionDomain>;

template <typename T>
inline constexpr bool IsExclusiveCapabilityV = CompositionFramework::IsExclusiveCapabilityForV<CompositionDomain, T>;

template <typename T>
inline constexpr bool IsSharedCapabilityV = CompositionFramework::IsSharedCapabilityForV<CompositionDomain, T>;

template <typename T>
inline constexpr bool IsCapabilityV = CompositionFramework::IsCapabilityForV<CompositionDomain, T>;

namespace Capability {
struct Execution final : ExclusiveCapability {};
struct Synchronization final : ExclusiveCapability {};
struct Queue final : ExclusiveCapability {};
struct Memory final : ExclusiveCapability {};
struct Clock final : ExclusiveCapability {};
struct Entropy final : ExclusiveCapability {};
struct GPIO final : ExclusiveCapability {};
struct ByteStream final : ExclusiveCapability {};
struct ProcessorAffinity final : SharedCapability {};
struct InterruptSafeSignalling final : SharedCapability {};
struct ExternalMemory final : SharedCapability {};
struct MonotonicClock final : SharedCapability {};
struct HighResolutionClock final : SharedCapability {};
} // namespace Capability

template <typename TValue>
using Property = CompositionFramework::Property<CompositionDomain, TValue>;

template <typename T>
inline constexpr bool IsPropertyV = CompositionFramework::IsPropertyForV<CompositionDomain, T>;

namespace PropertyKey {
struct ProcessorCount final : Property<std::size_t> {};
struct ClockFrequencyHz final : Property<std::uint64_t> {};
struct ExternalMemoryBytes final : Property<std::size_t> {};
struct MaximumQueueDepth final : Property<std::size_t> {};
} // namespace PropertyKey

template <typename TProperty, auto TValue>
using PropertyValue = CompositionFramework::PropertyValue<CompositionDomain, TProperty, TValue>;

template <typename... TPropertyValues>
using PropertySet = CompositionFramework::PropertySet<CompositionDomain, TPropertyValues...>;

template <typename TCapability, typename... TPropertyValues>
using CapabilityProfile = CompositionFramework::CapabilityProfile<CompositionDomain, TCapability, TPropertyValues...>;

template <typename... TEntries>
using CapabilitySet = CompositionFramework::CapabilitySet<CompositionDomain, TEntries...>;

using Constraint = CompositionFramework::Constraint<CompositionDomain>;

template <typename T>
inline constexpr bool IsConstraintV = CompositionFramework::IsConstraintForV<CompositionDomain, T>;

template <typename TProperty, auto TExpected>
using PropertyEquals = CompositionFramework::PropertyEquals<CompositionDomain, TProperty, TExpected>;
template <typename TProperty, auto TMinimum>
using PropertyAtLeast = CompositionFramework::PropertyAtLeast<CompositionDomain, TProperty, TMinimum>;
template <typename TProperty, auto TMaximum>
using PropertyAtMost = CompositionFramework::PropertyAtMost<CompositionDomain, TProperty, TMaximum>;
template <typename TProperty, auto TMinimumExclusive>
using PropertyGreaterThan = CompositionFramework::PropertyGreaterThan<CompositionDomain, TProperty, TMinimumExclusive>;
template <typename TProperty, auto TMaximumExclusive>
using PropertyLessThan = CompositionFramework::PropertyLessThan<CompositionDomain, TProperty, TMaximumExclusive>;

template <typename TCapability, typename... TConstraints>
using CapabilityRequirement = CompositionFramework::CapabilityRequirement<CompositionDomain, TCapability, TConstraints...>;

template <typename... TEntries>
using RequirementSet = CompositionFramework::RequirementSet<CompositionDomain, TEntries...>;

template <typename TBackend, typename TCapabilities, typename TRequirements = RequirementSet<>>
struct ProviderDeclaration
    : CompositionFramework::ProviderDeclaration<CompositionDomain, TCapabilities, TRequirements> {
    static_assert(IsBackendV<TBackend>,
                  "ProviderDeclaration backend must derive from ESPressio::Platform::Backend");
    using PlatformBackend = TBackend;
    using PlatformCapabilities = TCapabilities;
    using PlatformRequirements = TRequirements;
};

namespace Detail {
template <typename T, typename = void>
struct IsPlatformProvider : std::false_type {};
template <typename T>
struct IsPlatformProvider<T, std::void_t<typename T::PlatformBackend,
                                         typename T::PlatformCapabilities,
                                         typename T::PlatformRequirements>>
    : std::bool_constant<IsBackendV<typename T::PlatformBackend> &&
                         CompositionFramework::IsProviderForV<CompositionDomain, T>> {};
} // namespace Detail

template <typename T>
inline constexpr bool IsProviderV = Detail::IsPlatformProvider<T>::value;

template <typename... TProviders>
struct Composition : CompositionFramework::Composition<CompositionDomain, TProviders...> {
    static_assert((IsProviderV<TProviders> && ...),
                  "Platform Composition entries must expose a valid Platform ProviderDeclaration");
    using Base = CompositionFramework::Composition<CompositionDomain, TProviders...>;
    template <typename TCapability>
    static constexpr std::size_t ProvidersFor = Base::template ProviderCountFor<TCapability>;
};

template <typename TComposition, typename TRequirements>
struct Require : CompositionFramework::Require<TComposition, TRequirements> {};

template <typename TComposition, typename TRequirements>
using RequireT = typename Require<TComposition, TRequirements>::Type;

} // namespace ESPressio::Platform
