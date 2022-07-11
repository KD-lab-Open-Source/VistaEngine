#ifndef __NOISE_PARAMS_DIALOG_H_INCLUDED__
#define __NOISE_PARAMS_DIALOG_H_INCLUDED__

#include "kdw/Dialog.h"
#include "Render/src/NParticle.h"
#include "NoisePreview.h"

namespace kdw{
	class PropertyTree;
};

class NoiseParamsDialog: public kdw::Dialog{
public:
	NoiseParamsDialog(kdw::Window* parent);

	void set(const NoiseParams& noiseParams);
	const NoiseParams& get() const{ return noiseParams_; }

protected:
	void onChange();

	kdw::PropertyTree* propertyTree_;
	NoisePreview* noisePreview_;
	NoiseParams noiseParams_;
};

#endif
