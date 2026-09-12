#pragma once

#include <cstddef>
#include <type_traits>

namespace ESPressio::Platform {

/**
 * Base tag for platform backend identities.
 *
 * Concrete backend libraries define a distinct Backend type deriving from this
 * tag. The type identity, rather than an enum value, is used so third-party and
 * future ESPressio backends remain extensible without changing this library.
 */
struct Backend {};

/** Base tag for capabilities supplied by at most one provider in a composition. */
struct ExclusiveCapability {};

/** Base tag for feature capabilities that may be reported by multiple providers. */
struct SharedCapability {};

template <typename T>
inline constexpr bool IsBackendV = std::is_base_of_v<Backend, T> && !std::is_same_v<Backend, T>;

template <typename T>
inline constexpr bool IsExclusiveCapabilityV =
    std::is_base_of_v<ExclusiveCapability, T> && !std::is_same_v<ExclusiveCapability, T>;

template <typename T>
inline constexpr bool IsSharedCapabilityV =
    std::is_base_of_v<SharedCapability, T> && !std::is_same_v<SharedCapability, T>;

template <typename T>
inline constexpr bool IsCapabilityV = IsExclusiveCapabilityV<T> || IsSharedCapabilityV<T>;

namespace Capability {

// Core machine/runtime services. Exactly one provider may own each service in
// a validated Platform composition.
struct Execution final : ExclusiveCapability {};
struct Synchronization final : ExclusiveCapability {};
struct Queue final : ExclusiveCapability {};
struct Memory final : ExclusiveCapability {};
struct Clock final : ExclusiveCapability {};
struct Entropy final : ExclusiveCapability {};
struct GPIO final : ExclusiveCapability {};
struct ByteStream final : ExclusiveCapability {};

// Cross-service compile-time facts. More than one provider may legitimately
// contribute the same feature capability.
struct ProcessorAffinity final : SharedCapability {};
struct InterruptSafeSignalling final : SharedCapability {};
struct ExternalMemory final : SharedCapability {};
struct MonotonicClock final : SharedCapability {};
struct HighResolutionClock final : SharedCapability {};

} // namespace Capability

namespace Detail {

template <typename...>
struct TypesAreUnique : std::true_type {};

template <typename T, typename... TRest>
struct TypesAreUnique<T, TRest...>
    : std::bool_constant<(!std::is_same_v<T, TRest> && ...) && TypesAreUnique<TRest...>::value> {};

template <typename TNeedle, typename... THaystack>
inline constexpr bool ContainsTypeV = (std::is_same_v<TNeedle, THaystack> || ...);

} // namespace Detail

/** Compile-time set of capabilities supplied by one provider. */
template <typename... TCapabilities>
struct CapabilitySet {
    static_assert((IsCapabilityV<TCapabilities> && ...),
                  "CapabilitySet entries must derive from ExclusiveCapability or SharedCapability");
    static_assert(Detail::TypesAreUnique<TCapabilities...>::value,
                  "CapabilitySet must not contain duplicate capability types");

    static constexpr std::size_t Count = sizeof...(TCapabilities);

    template <typename TCapability>
    static constexpr bool Contains = Detail::ContainsTypeV<TCapability, TCapabilities...>;
};

/** Compile-time set of capabilities required by a provider or consumer. */
template <typename... TCapabilities>
struct RequirementSet {
    static_assert((IsCapabilityV<TCapabilities> && ...),
                  "RequirementSet entries must derive from ExclusiveCapability or SharedCapability");
    static_assert(Detail::TypesAreUnique<TCapabilities...>::value,
                  "RequirementSet must not contain duplicate capability types");

    static constexpr std::size_t Count = sizeof...(TCapabilities);

    template <typename TCapability>
    static constexpr bool Contains = Detail::ContainsTypeV<TCapability, TCapabilities...>;
};

/**
 * Static declaration inherited by concrete provider implementations.
 *
 * It carries no runtime state and imposes no implementation shape. Its sole
 * purpose is to state which backend owns a provider, which capabilities that
 * provider supplies, and which other capabilities must exist in the complete
 * composition for the provider to be legal.
 */
template <typename TBackend,
          typename TCapabilities,
          typename TRequirements = RequirementSet<>>
struct ProviderDeclaration {
    static_assert(IsBackendV<TBackend>,
                  "ProviderDeclaration backend must derive from ESPressio::Platform::Backend");

    using PlatformBackend = TBackend;
    using PlatformCapabilities = TCapabilities;
    using PlatformRequirements = TRequirements;
};

namespace Detail {

template <typename T, typename = void>
struct IsProvider : std::false_type {};

template <typename T>
struct IsProvider<T,
                  std::void_t<typename T::PlatformBackend,
                              typename T::PlatformCapabilities,
                              typename T::PlatformRequirements>>
    : std::bool_constant<IsBackendV<typename T::PlatformBackend>> {};

template <typename TProvider, typename TCapability>
inline constexpr bool ProviderProvidesV = TProvider::PlatformCapabilities::template Contains<TCapability>;

template <typename TCapability, typename... TProviders>
inline constexpr std::size_t ProviderCountV =
    (std::size_t{0} + ... + (ProviderProvidesV<TProviders, TCapability> ? std::size_t{1} : std::size_t{0}));

template <typename TRequirementSet, typename... TProviders>
struct RequirementsSatisfied;

template <typename... TCapabilities, typename... TProviders>
struct RequirementsSatisfied<RequirementSet<TCapabilities...>, TProviders...>
    : std::bool_constant<((ProviderCountV<TCapabilities, TProviders...> > 0U) && ...)> {};

template <typename TCapabilitySet, typename... TProviders>
struct CapabilitySetConflictFree;

template <typename... TCapabilities, typename... TProviders>
struct CapabilitySetConflictFree<CapabilitySet<TCapabilities...>, TProviders...>
    : std::bool_constant<((!IsExclusiveCapabilityV<TCapabilities> ||
                           ProviderCountV<TCapabilities, TProviders...> <= 1U) && ...)> {};

template <typename TCapability, typename... TProviders>
struct FirstProviderFor;

template <typename TCapability>
struct FirstProviderFor<TCapability> {
    using Type = void;
};

template <typename TCapability, typename TFirst, typename... TRest>
struct FirstProviderFor<TCapability, TFirst, TRest...> {
    using Type = std::conditional_t<ProviderProvidesV<TFirst, TCapability>,
                                    TFirst,
                                    typename FirstProviderFor<TCapability, TRest...>::Type>;
};

template <typename TRequirementSet>
struct IsRequirementSet : std::false_type {};

template <typename... TCapabilities>
struct IsRequirementSet<RequirementSet<TCapabilities...>> : std::true_type {};

} // namespace Detail

template <typename T>
inline constexpr bool IsProviderV = Detail::IsProvider<T>::value;

/**
 * Fully static composition of platform providers.
 *
 * Instantiation fails at compile time if:
 *  - an entry is not a provider declaration;
 *  - two providers claim the same exclusive capability; or
 *  - a provider's own requirements are not satisfied by the composition.
 */
template <typename... TProviders>
struct Composition {
    static_assert((IsProviderV<TProviders> && ...),
                  "Platform Composition entries must expose a valid ProviderDeclaration");

private:
    static constexpr bool NoExclusiveProviderConflicts =
        (Detail::CapabilitySetConflictFree<typename TProviders::PlatformCapabilities,
                                           TProviders...>::value && ...);

    static constexpr bool AllProviderRequirementsSatisfied =
        (Detail::RequirementsSatisfied<typename TProviders::PlatformRequirements,
                                       TProviders...>::value && ...);

public:
    static_assert(NoExclusiveProviderConflicts,
                  "Platform Composition has multiple providers for an exclusive capability");
    static_assert(AllProviderRequirementsSatisfied,
                  "Platform Composition contains a provider with unsatisfied capability requirements");

    static constexpr std::size_t ProviderCount = sizeof...(TProviders);
    static constexpr bool IsValid = NoExclusiveProviderConflicts && AllProviderRequirementsSatisfied;

    template <typename TCapability>
    static constexpr std::size_t ProvidersFor = Detail::ProviderCountV<TCapability, TProviders...>;

    template <typename TCapability>
    static constexpr bool Provides = ProvidersFor<TCapability> > 0U;

    template <typename TRequirementSet>
    static constexpr bool Satisfies =
        Detail::IsRequirementSet<TRequirementSet>::value &&
        Detail::RequirementsSatisfied<TRequirementSet, TProviders...>::value;

    template <typename TCapability>
    struct ResolveProvider {
        static_assert(ProvidersFor<TCapability> == 1U,
                      "ResolveProvider requires exactly one provider for the requested capability");
        using Type = typename Detail::FirstProviderFor<TCapability, TProviders...>::Type;
    };

    template <typename TCapability>
    using ProviderFor = typename ResolveProvider<TCapability>::Type;
};

/**
 * Compile-time consumer gate. The alias can be placed at a consumer boundary so
 * a missing required capability is diagnosed during compilation, never at boot.
 */
template <typename TComposition, typename TRequirements>
struct Require {
    static_assert(TComposition::template Satisfies<TRequirements>,
                  "Platform Composition does not satisfy the required capabilities");
    using Type = TComposition;
};

template <typename TComposition, typename TRequirements>
using RequireT = typename Require<TComposition, TRequirements>::Type;

} // namespace ESPressio::Platform
