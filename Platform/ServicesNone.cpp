// No store: what the engine's own builds use, and a game sold without one. See
// Platform/Services.h.
#include "Platform/Services.h"

namespace PlatformServices {

bool startup()        { return true; }
bool update()         { return true; }
bool overlayActive()  { return false; }
const char* language() { return nullptr; }
void shutdown()       {}

} // namespace PlatformServices
