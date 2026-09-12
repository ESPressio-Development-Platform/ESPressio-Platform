#include "ESPressio_Platform.hpp"
using namespace ESPressio::Platform;

using Invalid = PropertySet<
    PropertyValue<PropertyKey::ProcessorCount, std::size_t{1}>,
    PropertyValue<PropertyKey::ProcessorCount, std::size_t{2}>>;

int main() { return static_cast<int>(Invalid::Count); }
