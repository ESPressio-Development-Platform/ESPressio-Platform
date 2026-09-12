#pragma once

#include <cstddef>
#include <cstdint>
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
struct Property {
    using ValueType = TValue;
};

template <typename T, typename = void>
struct IsProperty : std::false_type {};

template <typename T>
struct IsProperty<T, std::void_t<typename T::ValueType>>
    : std::bool_constant<std::is_base_of_v<Property<typename T::ValueType>, T> &&
                         !std::is_same_v<Property<typename T::ValueType>, T>> {};

template <typename T>
inline constexpr bool IsPropertyV = IsProperty<T>::value;

namespace PropertyKey {

struct ProcessorCount final : Property<std::size_t> {};
struct ClockFrequencyHz final : Property<std::uint64_t> {};
struct ExternalMemoryBytes final : Property<std::size_t> {};
struct MaximumQueueDepth final : Property<std::size_t> {};

} // namespace PropertyKey

namespace Detail {

template <typename...>
struct TypesAreUnique : std::true_type {};

template <typename T, typename... TRest>
struct TypesAreUnique<T, TRest...>
    : std::bool_constant<(!std::is_same_v<T, TRest> && ...) && TypesAreUnique<TRest...>::value> {};

template <typename TNeedle, typename... THaystack>
inline constexpr bool ContainsTypeV = (std::is_same_v<TNeedle, THaystack> || ...);

template <typename T, typename = void>
struct IsPropertyValue : std::false_type {};

} // namespace Detail

template <typename TProperty, auto TValue>
struct PropertyValue {
    static_assert(IsPropertyV<TProperty>,
                  "PropertyValue key must derive from ESPressio::Platform::Property<T>");
    static_assert(std::is_convertible_v<decltype(TValue), typename TProperty::ValueType>,
                  "PropertyValue value must be convertible to the property's ValueType");

    using PropertyType = TProperty;
    using ValueType = typename TProperty::ValueType;
    static constexpr ValueType Value = static_cast<ValueType>(TValue);
};

namespace Detail {

template <typename T>
struct IsPropertyValue<T, std::void_t<typename T::PropertyType, typename T::ValueType>>
    : std::bool_constant<IsPropertyV<typename T::PropertyType>> {};

template <typename T>
inline constexpr bool IsPropertyValueV = IsPropertyValue<T>::value;

template <typename... TPropertyValues>
struct PropertyKeysAreUnique;

template <>
struct PropertyKeysAreUnique<> : std::true_type {};

template <typename TFirst, typename... TRest>
struct PropertyKeysAreUnique<TFirst, TRest...>
    : std::bool_constant<
          ((!std::is_same_v<typename TFirst::PropertyType, typename TRest::PropertyType>) && ...) &&
          PropertyKeysAreUnique<TRest...>::value> {};

template <typename TProperty, typename... TPropertyValues>
struct FindPropertyValue;

template <typename TProperty>
struct FindPropertyValue<TProperty> {
    using Type = void;
};

template <typename TProperty, typename TFirst, typename... TRest>
struct FindPropertyValue<TProperty, TFirst, TRest...> {
    using Type = std::conditional_t<
        std::is_same_v<TProperty, typename TFirst::PropertyType>,
        TFirst,
        typename FindPropertyValue<TProperty, TRest...>::Type>;
};

} // namespace Detail

template <typename... TPropertyValues>
struct PropertySet {
    static_assert((Detail::IsPropertyValueV<TPropertyValues> && ...),
                  "PropertySet entries must be PropertyValue types");
    static_assert(Detail::PropertyKeysAreUnique<TPropertyValues...>::value,
                  "PropertySet must not contain more than one value for the same property key");

    static constexpr std::size_t Count = sizeof...(TPropertyValues);

    template <typename TProperty>
    static constexpr bool Contains =
        (!std::is_void_v<typename Detail::FindPropertyValue<TProperty, TPropertyValues...>::Type>);

    template <typename TProperty>
    struct Get {
        using Entry = typename Detail::FindPropertyValue<TProperty, TPropertyValues...>::Type;
        static_assert(!std::is_void_v<Entry>,
                      "Requested capability property is not declared by this provider");
        static constexpr typename TProperty::ValueType Value = Entry::Value;
    };

    template <typename TProperty>
    static constexpr typename TProperty::ValueType Value = Get<TProperty>::Value;
};

template <typename TCapability, typename... TPropertyValues>
struct CapabilityProfile {
    static_assert(IsCapabilityV<TCapability>,
                  "CapabilityProfile capability must derive from a Platform capability tag");

    using CapabilityType = TCapability;
    using Properties = PropertySet<TPropertyValues...>;
};

namespace Detail {

template <typename T, typename = void>
struct CapabilityEntryTraits {
    static constexpr bool IsValid = IsCapabilityV<T>;
    using CapabilityType = T;
    using Properties = PropertySet<>;
};

template <typename T>
struct CapabilityEntryTraits<T, std::void_t<typename T::CapabilityType, typename T::Properties>> {
    static constexpr bool IsValid = IsCapabilityV<typename T::CapabilityType>;
    using CapabilityType = typename T::CapabilityType;
    using Properties = typename T::Properties;
};

template <typename... TEntries>
struct CapabilityKeysAreUnique;

template <>
struct CapabilityKeysAreUnique<> : std::true_type {};

template <typename TFirst, typename... TRest>
struct CapabilityKeysAreUnique<TFirst, TRest...>
    : std::bool_constant<
          ((!std::is_same_v<typename CapabilityEntryTraits<TFirst>::CapabilityType,
                            typename CapabilityEntryTraits<TRest>::CapabilityType>) && ...) &&
          CapabilityKeysAreUnique<TRest...>::value> {};

template <typename TCapability, typename... TEntries>
struct FindCapabilityEntry;

template <typename TCapability>
struct FindCapabilityEntry<TCapability> {
    using Type = void;
};

template <typename TCapability, typename TFirst, typename... TRest>
struct FindCapabilityEntry<TCapability, TFirst, TRest...> {
    using Type = std::conditional_t<
        std::is_same_v<TCapability,
                       typename CapabilityEntryTraits<TFirst>::CapabilityType>,
        TFirst,
        typename FindCapabilityEntry<TCapability, TRest...>::Type>;
};

} // namespace Detail

template <typename... TEntries>
struct CapabilitySet {
    static_assert((Detail::CapabilityEntryTraits<TEntries>::IsValid && ...),
                  "CapabilitySet entries must be capability tags or CapabilityProfile types");
    static_assert(Detail::CapabilityKeysAreUnique<TEntries...>::value,
                  "CapabilitySet must not declare the same capability more than once");

    static constexpr std::size_t Count = sizeof...(TEntries);

    template <typename TCapability>
    static constexpr bool Contains =
        (!std::is_void_v<typename Detail::FindCapabilityEntry<TCapability, TEntries...>::Type>);

    template <typename TCapability>
    struct Profile {
        using Entry = typename Detail::FindCapabilityEntry<TCapability, TEntries...>::Type;
        static_assert(!std::is_void_v<Entry>,
                      "Requested capability is not declared by this provider");
        using Properties = typename Detail::CapabilityEntryTraits<Entry>::Properties;
    };

    template <typename TCapability>
    using PropertiesFor = typename Profile<TCapability>::Properties;
};

struct Constraint {};

template <typename T>
inline constexpr bool IsConstraintV =
    std::is_base_of_v<Constraint, T> && !std::is_same_v<Constraint, T>;

namespace Detail {

template <typename TPropertySet, typename TProperty, auto TExpected>
constexpr bool PropertyEqualsImpl() {
    if constexpr (!TPropertySet::template Contains<TProperty>) {
        return false;
    } else {
        using ValueType = typename TProperty::ValueType;
        return TPropertySet::template Value<TProperty> == static_cast<ValueType>(TExpected);
    }
}

template <typename TPropertySet, typename TProperty, auto TExpected>
constexpr bool PropertyAtLeastImpl() {
    if constexpr (!TPropertySet::template Contains<TProperty>) {
        return false;
    } else {
        using ValueType = typename TProperty::ValueType;
        return TPropertySet::template Value<TProperty> >= static_cast<ValueType>(TExpected);
    }
}

template <typename TPropertySet, typename TProperty, auto TExpected>
constexpr bool PropertyAtMostImpl() {
    if constexpr (!TPropertySet::template Contains<TProperty>) {
        return false;
    } else {
        using ValueType = typename TProperty::ValueType;
        return TPropertySet::template Value<TProperty> <= static_cast<ValueType>(TExpected);
    }
}

template <typename TPropertySet, typename TProperty, auto TExpected>
constexpr bool PropertyGreaterThanImpl() {
    if constexpr (!TPropertySet::template Contains<TProperty>) {
        return false;
    } else {
        using ValueType = typename TProperty::ValueType;
        return TPropertySet::template Value<TProperty> > static_cast<ValueType>(TExpected);
    }
}

template <typename TPropertySet, typename TProperty, auto TExpected>
constexpr bool PropertyLessThanImpl() {
    if constexpr (!TPropertySet::template Contains<TProperty>) {
        return false;
    } else {
        using ValueType = typename TProperty::ValueType;
        return TPropertySet::template Value<TProperty> < static_cast<ValueType>(TExpected);
    }
}

} // namespace Detail

template <typename TProperty, auto TExpected>
struct PropertyEquals final : Constraint {
    static_assert(IsPropertyV<TProperty>, "PropertyEquals key must be a Platform property");
    static_assert(std::is_convertible_v<decltype(TExpected), typename TProperty::ValueType>,
                  "PropertyEquals value must be convertible to the property's ValueType");
    using PropertyType = TProperty;
    template <typename TPropertySet>
    static constexpr bool Satisfied =
        Detail::PropertyEqualsImpl<TPropertySet, TProperty, TExpected>();
};

template <typename TProperty, auto TMinimum>
struct PropertyAtLeast final : Constraint {
    static_assert(IsPropertyV<TProperty>, "PropertyAtLeast key must be a Platform property");
    static_assert(std::is_convertible_v<decltype(TMinimum), typename TProperty::ValueType>,
                  "PropertyAtLeast value must be convertible to the property's ValueType");
    using PropertyType = TProperty;
    template <typename TPropertySet>
    static constexpr bool Satisfied =
        Detail::PropertyAtLeastImpl<TPropertySet, TProperty, TMinimum>();
};

template <typename TProperty, auto TMaximum>
struct PropertyAtMost final : Constraint {
    static_assert(IsPropertyV<TProperty>, "PropertyAtMost key must be a Platform property");
    static_assert(std::is_convertible_v<decltype(TMaximum), typename TProperty::ValueType>,
                  "PropertyAtMost value must be convertible to the property's ValueType");
    using PropertyType = TProperty;
    template <typename TPropertySet>
    static constexpr bool Satisfied =
        Detail::PropertyAtMostImpl<TPropertySet, TProperty, TMaximum>();
};

template <typename TProperty, auto TMinimumExclusive>
struct PropertyGreaterThan final : Constraint {
    static_assert(IsPropertyV<TProperty>, "PropertyGreaterThan key must be a Platform property");
    static_assert(std::is_convertible_v<decltype(TMinimumExclusive), typename TProperty::ValueType>,
                  "PropertyGreaterThan value must be convertible to the property's ValueType");
    using PropertyType = TProperty;
    template <typename TPropertySet>
    static constexpr bool Satisfied =
        Detail::PropertyGreaterThanImpl<TPropertySet, TProperty, TMinimumExclusive>();
};

template <typename TProperty, auto TMaximumExclusive>
struct PropertyLessThan final : Constraint {
    static_assert(IsPropertyV<TProperty>, "PropertyLessThan key must be a Platform property");
    static_assert(std::is_convertible_v<decltype(TMaximumExclusive), typename TProperty::ValueType>,
                  "PropertyLessThan value must be convertible to the property's ValueType");
    using PropertyType = TProperty;
    template <typename TPropertySet>
    static constexpr bool Satisfied =
        Detail::PropertyLessThanImpl<TPropertySet, TProperty, TMaximumExclusive>();
};

template <typename TCapability, typename... TConstraints>
struct CapabilityRequirement {
    using RequirementTag = void;
    static_assert(IsCapabilityV<TCapability>,
                  "CapabilityRequirement capability must be a Platform capability");
    static_assert((IsConstraintV<TConstraints> && ...),
                  "CapabilityRequirement entries must be Platform property constraints");

    using CapabilityType = TCapability;

    template <typename TPropertySet>
    static constexpr bool PropertiesSatisfied =
        (TConstraints::template Satisfied<TPropertySet> && ...);
};

namespace Detail {

template <typename T, typename = void>
struct RequirementEntryTraits {
    static constexpr bool IsValid = IsCapabilityV<T>;
    using CapabilityType = T;

    template <typename TPropertySet>
    static constexpr bool PropertiesSatisfied = true;
};

template <typename T>
struct RequirementEntryTraits<T,
                              std::void_t<typename T::RequirementTag,
                                          typename T::CapabilityType>> {
    static constexpr bool IsValid = IsCapabilityV<typename T::CapabilityType>;
    using CapabilityType = typename T::CapabilityType;

    template <typename TPropertySet>
    static constexpr bool PropertiesSatisfied =
        T::template PropertiesSatisfied<TPropertySet>;
};

template <typename... TEntries>
struct RequirementKeysAreUnique;

template <>
struct RequirementKeysAreUnique<> : std::true_type {};

template <typename TFirst, typename... TRest>
struct RequirementKeysAreUnique<TFirst, TRest...>
    : std::bool_constant<
          ((!std::is_same_v<typename RequirementEntryTraits<TFirst>::CapabilityType,
                            typename RequirementEntryTraits<TRest>::CapabilityType>) && ...) &&
          RequirementKeysAreUnique<TRest...>::value> {};

} // namespace Detail

template <typename... TEntries>
struct RequirementSet {
    static_assert((Detail::RequirementEntryTraits<TEntries>::IsValid && ...),
                  "RequirementSet entries must be capability tags or CapabilityRequirement types");
    static_assert(Detail::RequirementKeysAreUnique<TEntries...>::value,
                  "RequirementSet must not require the same capability more than once");

    static constexpr std::size_t Count = sizeof...(TEntries);
};

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
inline constexpr bool ProviderProvidesV =
    TProvider::PlatformCapabilities::template Contains<TCapability>;

template <typename TCapability, typename... TProviders>
inline constexpr std::size_t ProviderCountV =
    (std::size_t{0} + ... +
     (ProviderProvidesV<TProviders, TCapability> ? std::size_t{1} : std::size_t{0}));

template <typename TProvider, typename TRequirementEntry, bool =
              ProviderProvidesV<TProvider,
                                typename RequirementEntryTraits<TRequirementEntry>::CapabilityType>>
struct ProviderSatisfiesRequirement : std::false_type {};

template <typename TProvider, typename TRequirementEntry>
struct ProviderSatisfiesRequirement<TProvider, TRequirementEntry, true>
    : std::bool_constant<
          RequirementEntryTraits<TRequirementEntry>::template PropertiesSatisfied<
              typename TProvider::PlatformCapabilities::template PropertiesFor<
                  typename RequirementEntryTraits<TRequirementEntry>::CapabilityType>>> {};

template <typename TRequirementEntry, typename... TProviders>
inline constexpr std::size_t SatisfyingProviderCountV =
    (std::size_t{0} + ... +
     (ProviderSatisfiesRequirement<TProviders, TRequirementEntry>::value
          ? std::size_t{1}
          : std::size_t{0}));

template <typename TRequirementSet, typename... TProviders>
struct RequirementsSatisfied;

template <typename... TEntries, typename... TProviders>
struct RequirementsSatisfied<RequirementSet<TEntries...>, TProviders...>
    : std::bool_constant<((SatisfyingProviderCountV<TEntries, TProviders...> > 0U) && ...)> {};

template <typename TCapabilitySet, typename... TProviders>
struct CapabilitySetConflictFree;

template <typename... TEntries, typename... TProviders>
struct CapabilitySetConflictFree<CapabilitySet<TEntries...>, TProviders...>
    : std::bool_constant<
          ((!IsExclusiveCapabilityV<typename CapabilityEntryTraits<TEntries>::CapabilityType> ||
            ProviderCountV<typename CapabilityEntryTraits<TEntries>::CapabilityType,
                           TProviders...> <= 1U) &&
           ...)> {};

template <typename TCapability, typename... TProviders>
struct FirstProviderFor;

template <typename TCapability>
struct FirstProviderFor<TCapability> {
    using Type = void;
};

template <typename TCapability, typename TFirst, typename... TRest>
struct FirstProviderFor<TCapability, TFirst, TRest...> {
    using Type = std::conditional_t<
        ProviderProvidesV<TFirst, TCapability>,
        TFirst,
        typename FirstProviderFor<TCapability, TRest...>::Type>;
};

template <typename TRequirementSet>
struct IsRequirementSet : std::false_type {};

template <typename... TEntries>
struct IsRequirementSet<RequirementSet<TEntries...>> : std::true_type {};

} // namespace Detail

template <typename T>
inline constexpr bool IsProviderV = Detail::IsProvider<T>::value;

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
                  "Platform Composition contains a provider with unsatisfied capability/property requirements");

    static constexpr std::size_t ProviderCount = sizeof...(TProviders);
    static constexpr bool IsValid =
        NoExclusiveProviderConflicts && AllProviderRequirementsSatisfied;

    template <typename TCapability>
    static constexpr std::size_t ProvidersFor =
        Detail::ProviderCountV<TCapability, TProviders...>;

    template <typename TCapability>
    static constexpr bool Provides = ProvidersFor<TCapability> > 0U;

    template <typename TRequirement>
    static constexpr std::size_t ProvidersSatisfying =
        Detail::SatisfyingProviderCountV<TRequirement, TProviders...>;

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

    template <typename TCapability>
    using PropertiesFor =
        typename ProviderFor<TCapability>::PlatformCapabilities::template PropertiesFor<TCapability>;

    template <typename TCapability, typename TProperty>
    static constexpr bool HasProperty =
        PropertiesFor<TCapability>::template Contains<TProperty>;

    template <typename TCapability, typename TProperty>
    static constexpr typename TProperty::ValueType PropertyValue =
        PropertiesFor<TCapability>::template Value<TProperty>;
};

template <typename TComposition, typename TRequirements>
struct Require {
    static_assert(TComposition::template Satisfies<TRequirements>,
                  "Platform Composition does not satisfy the required capability/property constraints");
    using Type = TComposition;
};

template <typename TComposition, typename TRequirements>
using RequireT = typename Require<TComposition, TRequirements>::Type;

} // namespace ESPressio::Platform
