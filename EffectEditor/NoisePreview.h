#ifndef __NOISE_PREVIEW_H_INCLUDED__
#define __NOISE_PREVIEW_H_INCLUDED__

#include "kdw/_WidgetWithWindow.h"

class NoisePreviewImpl;
struct NoiseParams;

class NoisePreview : public kdw::_WidgetWithWindow{
public:
	NoisePreview(int border = 0);
	void set(const NoiseParams& params);
protected:
	NoisePreviewImpl& impl();
};

#endif
