#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "ESPressio_PlatformQueue.hpp"
#include "ESPressio_PlatformSynchronization.hpp"

using namespace ESPressio::Platform;

namespace {

struct TestBackend final : Backend {};

class TestMutex {
public:
    void Lock() noexcept { locked_ = true; }
    bool TryLock() noexcept {
        if (locked_) return false;
        locked_ = true;
        return true;
    }
    bool Unlock() noexcept {
        if (!locked_) return false;
        locked_ = false;
        return true;
    }

private:
    bool locked_{false};
};

class TestSignal {
public:
    void Give() noexcept { signalled_ = true; }
    void Wait() noexcept { signalled_ = false; }
    bool TryWait() noexcept {
        if (!signalled_) return false;
        signalled_ = false;
        return true;
    }
    void Reset() noexcept { signalled_ = false; }
    Synchronization::InterruptSignalResult GiveFromInterrupt() noexcept {
        const bool changed = !signalled_;
        signalled_ = true;
        return {changed, false};
    }

private:
    bool signalled_{false};
};

struct TestSynchronizationProvider final
    : Synchronization::ProviderDeclaration<
          TestBackend,
          CapabilitySet<
              Capability::StaticSynchronizationStorage,
              Capability::RecursiveMutex,
              Capability::InterruptSafeSignalling>> {
    using Mutex = TestMutex;
    using RecursiveMutex = TestMutex;
    using Signal = TestSignal;
};

template <typename T, std::size_t TDepth>
class TestQueue {
public:
    static_assert(TDepth > 0U);

    static constexpr std::size_t Depth = TDepth;
    static constexpr std::size_t ElementSizeBytes = sizeof(T);
    static constexpr std::size_t PayloadStorageBytes = sizeof(T) * TDepth;
    static constexpr std::size_t ControlStorageBytes = sizeof(std::size_t) * 3U;
    static constexpr std::size_t TotalStaticStorageBytes =
        PayloadStorageBytes + ControlStorageBytes;

    void Send(const T& value) noexcept {
        while (!TrySend(value)) {}
    }

    bool TrySend(const T& value) noexcept {
        if (count_ == TDepth) return false;
        values_[tail_] = value;
        tail_ = (tail_ + 1U) % TDepth;
        ++count_;
        return true;
    }

    void Receive(T& value) noexcept {
        while (!TryReceive(value)) {}
    }

    bool TryReceive(T& value) noexcept {
        if (count_ == 0U) return false;
        value = values_[head_];
        head_ = (head_ + 1U) % TDepth;
        --count_;
        return true;
    }

    std::size_t Size() const noexcept { return count_; }

    void Reset() noexcept {
        head_ = 0U;
        tail_ = 0U;
        count_ = 0U;
    }

    Queue::InterruptOperationResult SendFromInterrupt(const T& value) noexcept {
        return {TrySend(value), false};
    }

    Queue::InterruptOperationResult ReceiveFromInterrupt(T& value) noexcept {
        return {TryReceive(value), false};
    }

private:
    std::array<T, TDepth> values_{};
    std::size_t head_{0U};
    std::size_t tail_{0U};
    std::size_t count_{0U};
};

struct TestQueueProvider final
    : Queue::ProviderDeclaration<
          TestBackend,
          CapabilitySet<
              Capability::StaticQueueStorage,
              Capability::QueueSendFromInterrupt,
              Capability::QueueReceiveFromInterrupt>> {
    template <typename T, std::size_t TDepth>
    using Queue = TestQueue<T, TDepth>;
};

using TestPlatform = Composition<TestSynchronizationProvider, TestQueueProvider>;

static_assert(Synchronization::IsMutexV<TestMutex>);
static_assert(Synchronization::IsSignalV<TestSignal>);
static_assert(Synchronization::IsInterruptSignalV<TestSignal>);
static_assert(Synchronization::IsProviderV<TestSynchronizationProvider>);
static_assert(Queue::IsQueueV<TestQueue<std::uint32_t, 4U>, std::uint32_t, 4U>);
static_assert(Queue::IsInterruptQueueV<TestQueue<std::uint32_t, 4U>, std::uint32_t>);
static_assert(Queue::IsStaticQueueV<TestQueue<std::uint32_t, 4U>>);
static_assert(Queue::IsProviderV<TestQueueProvider>);
static_assert(TestPlatform::Provides<Capability::Synchronization>);
static_assert(TestPlatform::Provides<Capability::Queue>);
static_assert(TestPlatform::Provides<Capability::InterruptSafeSignalling>);
static_assert(TestPlatform::Provides<Capability::QueueSendFromInterrupt>);

using SelectedSynchronization = TestPlatform::ProviderFor<Capability::Synchronization>;
using SelectedQueueProvider = TestPlatform::ProviderFor<Capability::Queue>;
using SelectedQueue = SelectedQueueProvider::Queue<std::uint32_t, 4U>;
static_assert(Synchronization::IsProviderV<SelectedSynchronization>);
static_assert(Queue::IsQueueV<SelectedQueue, std::uint32_t, 4U>);

} // namespace

int main() {
    SelectedSynchronization::Mutex mutex;
    assert(mutex.TryLock());
    assert(!mutex.TryLock());
    assert(mutex.Unlock());

    SelectedSynchronization::Signal signal;
    assert(!signal.TryWait());
    signal.Give();
    assert(signal.TryWait());
    const auto interruptSignal = signal.GiveFromInterrupt();
    assert(interruptSignal.StateChanged);

    SelectedQueue queue;
    assert(queue.Size() == 0U);
    assert(queue.TrySend(11U));
    assert(queue.TrySend(22U));
    assert(queue.Size() == 2U);

    std::uint32_t value = 0U;
    assert(queue.TryReceive(value));
    assert(value == 11U);

    const auto interruptSend = queue.SendFromInterrupt(33U);
    assert(interruptSend.Succeeded);
    const auto interruptReceive = queue.ReceiveFromInterrupt(value);
    assert(interruptReceive.Succeeded);
    assert(value == 22U);

    queue.Reset();
    assert(queue.Size() == 0U);
    return 0;
}
