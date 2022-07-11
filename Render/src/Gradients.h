#ifndef __GRADIENTS_H_INCLUDED__
#define __GRADIENTS_H_INCLUDED__

#include "NParticleKey.h"

struct TimeNoAlphaGradient: KeysColor{
};

struct BytePositionGradient: KeysColor{
};

struct BytePositionNoColorGradient: KeysColor{
};

struct NoAlphaGradient: KeysColor{
};

struct WaterGradient: KeysColor{
};

struct SkyGradient: KeysColor{//no alpha
};

struct SkyAlphaGradient: KeysColor{
};

#endif
