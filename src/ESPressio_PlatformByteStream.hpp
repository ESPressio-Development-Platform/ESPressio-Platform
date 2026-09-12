#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

#include "ESPressio_PlatformCapabilities.hpp"
#include "ESPressio_PlatformIO.hpp"

namespace ESPressio::Platform {

namespace ByteStream {

/** Tag used when an application requires one canonical/default byte stream. */
struct DefaultStream {};

} // namespace ByteStream

namespace Capability {

/** Additional independently selectable byte stream endpoint. */
template <typename TStreamTag>
struct ByteStreamEndpoint final : ExclusiveCapability {};

/** Endpoint supports byte reads. */
template <typename TStreamTag>
struct ByteStreamReadable final : SharedCapability {};

/** Endpoint supports byte writes. */
template <typename TStreamTag>
struct ByteStreamWritable final : SharedCapability {};

/** Endpoint can report immediately readable byte count. */
template <typename TStreamTag>
struct ByteStreamAvailability final : SharedCapability {};

} // namespace Capability

namespace ByteStream {

template <typename TStreamTag>
using CapabilityFor = std::conditional_t<
    std::is_same_v<TStreamTag, DefaultStream>,
    ESPressio::Platform::Capability::ByteStream,
    ESPressio::Platform::Capability::ByteStreamEndpoint<TStreamTag>>;

namespace Detail {

template <typename TExtraCapabilities>
struct DeclarationBuilder;

template <typename... TExtraCapabilities>
struct DeclarationBuilder<CapabilitySet<TExtraCapabilities...>> {
    template <typename TOrigin, typename TStreamTag, typename TRequirements>
    using Type = ESPressio::Platform::ProviderDeclaration<
        TOrigin,
        CapabilitySet<
            CapabilityFor<TStreamTag>,
            Capability::ByteStreamReadable<TStreamTag>,
            Capability::ByteStreamWritable<TStreamTag>,
            Capability::ByteStreamAvailability<TStreamTag>,
            TExtraCapabilities...>,
        TRequirements>;
};

template <typename T, typename TStreamTag, typename = void>
struct IsStream : std::false_type {};

template <typename T, typename TStreamTag>
struct IsStream<
    T,
    TStreamTag,
    std::void_t<
        typename T::PlatformCapabilities,
        decltype(std::declval<T&>().Available(std::declval<std::size_t&>())),
        decltype(std::declval<T&>().Read(std::declval<IO::MutableBuffer>())),
        decltype(std::declval<T&>().Write(std::declval<IO::ConstBuffer>()))>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<CapabilityFor<TStreamTag>> &&
          T::PlatformCapabilities::template Contains<Capability::ByteStreamReadable<TStreamTag>> &&
          T::PlatformCapabilities::template Contains<Capability::ByteStreamWritable<TStreamTag>> &&
          T::PlatformCapabilities::template Contains<Capability::ByteStreamAvailability<TStreamTag>> &&
          std::is_same_v<decltype(std::declval<T&>().Available(std::declval<std::size_t&>())),
                         IO::Result> &&
          std::is_same_v<decltype(std::declval<T&>().Read(std::declval<IO::MutableBuffer>())),
                         IO::TransferResult> &&
          std::is_same_v<decltype(std::declval<T&>().Write(std::declval<IO::ConstBuffer>())),
                         IO::TransferResult> &&
          noexcept(std::declval<T&>().Available(std::declval<std::size_t&>())) &&
          noexcept(std::declval<T&>().Read(std::declval<IO::MutableBuffer>())) &&
          noexcept(std::declval<T&>().Write(std::declval<IO::ConstBuffer>()))> {};

} // namespace Detail

template <typename TOrigin,
          typename TStreamTag = DefaultStream,
          typename TExtraCapabilities = CapabilitySet<>,
          typename TRequirements = RequirementSet<>>
struct ProviderDeclaration
    : Detail::DeclarationBuilder<TExtraCapabilities>::template Type<
          TOrigin, TStreamTag, TRequirements> {};

template <typename T, typename TStreamTag = DefaultStream>
inline constexpr bool IsStreamV = Detail::IsStream<T, TStreamTag>::value;

} // namespace ByteStream

} // namespace ESPressio::Platform
