#ifndef _REELMANAGER_H
#define _REELMANAGER_H

#include "PlayBink.h"
#include "bubles\blobs.h"
#include "..\Sound\SoundSystem.h"
#include "..\Game\SoundApp.h"

struct LogoAttributes
{
	string bkgName;
	string fishName;
	string logoName;
	string groundName;
	float fishAlpha;
	float logoAlpha;
	float fadeTime;
};


class ReelManager {
	public:
		enum SizeType {
			CUSTOM,
			REAL_SIZE,
			FULL_SCREEN
		};
		ReelManager();
		~ReelManager();
		void showModal(const char* binkFileName, const char* soundFileName, bool stopBGMusic = true, int alpha = 255);
		void showPictureModal(const char* pictureFileName, int stableTime);
		void showLogoModal(LogoAttributes& logoAttributes, const cBlobsSetting& blobsSetting, int stableTime,SoundLogoAttributes& soundAttributes);
		bool isVisible() const {
			return visible;
		}
		void hide() {
			visible = false;
		}
		Vect2i pos;
		Vect2i size;
		SizeType sizeType;
	protected:
		bool visible;
		PlayBink* bink;
		cTexture* bgTexture;
		double startTime;
};

#endif //_REELMANAGER_H
