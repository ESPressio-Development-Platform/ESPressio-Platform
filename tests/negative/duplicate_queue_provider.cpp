#include "ESPressio_PlatformQueue.hpp"

using namespace ESPressio::Platform;

struct BackendA final : Backend {};
struct BackendB final : Backend {};

struct QueueProviderA final : Queue::ProviderDeclaration<BackendA> {
    template <typename T, std::size_t TDepth>
    struct Queue {
        static constexpr std::size_t Depth = TDepth;
        static constexpr std::size_t ElementSizeBytes = sizeof(T);
        static constexpr std::size_t PayloadStorageBytes = sizeof(T) * TDepth;
        void Send(const T&) noexcept {}
        bool TrySend(const T&) noexcept { return true; }
        void Receive(T&) noexcept {}
        bool TryReceive(T&) noexcept { return true; }
        std::size_t Size() const noexcept { return 0U; }
        void Reset() noexcept {}
    };
};

struct QueueProviderB final : Queue::ProviderDeclaration<BackendB> {
    template <typename T, std::size_t TDepth>
    using Queue = typename QueueProviderA::template Queue<T, TDepth>;
};

using Invalid = Composition<QueueProviderA, QueueProviderB>;
static_assert(Invalid::IsValid);

int main() { return 0; }
