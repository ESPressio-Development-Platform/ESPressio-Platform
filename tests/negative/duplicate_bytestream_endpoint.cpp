#include "ESPressio_Platform.hpp"

using namespace ESPressio::Platform;

struct OriginA final : Backend {};
struct OriginB final : Backend {};
struct StreamTag {};

struct StreamA final : ByteStream::ProviderDeclaration<OriginA, StreamTag> {};
struct StreamB final : ByteStream::ProviderDeclaration<OriginB, StreamTag> {};

using InvalidPlatform = Composition<StreamA, StreamB>;
static_assert(InvalidPlatform::IsValid);

int main() { return 0; }
