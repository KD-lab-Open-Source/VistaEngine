#ifndef __SUR_MAP_OPTIONS_H_INCLUDED__
#define __SUR_MAP_OPTIONS_H_INCLUDED__

#include "XMath\Colors.h"
#include "XTL\Rect.h"

class SurMapOptions
{
public:
    SurMapOptions();
    ~SurMapOptions();

    std::string getLastDirResource(const char* internalResourcePath);
    void setLastDirResource(const char* internalResourcePath, const char* selectedDir);
    
    void serialize(Archive& ar);

	void load();
	void save();

	void apply();

	bool showPathFinding() const{ return showPathFinding_; }
	void setShowPathFinding(bool showPathFinding) { showPathFinding_ = showPathFinding; }
	
	const char* showPathFindingReferenceUnit() const { return showPathFindingReferenceUnit_.c_str(); }
	void setShowPathFindingReferenceUnit(const char* attributeName) { showPathFindingReferenceUnit_ = attributeName; }

	void setShowSources(bool showSources) { showSources_ = showSources; }
	bool showSources() const{ return showSources_; }

	void setShowCameras(bool showCameras) { showCameras_ = showCameras; }
	bool showCameras() const{ return showCameras_; }

	void setHideWorldModels(bool hideWorldModels) { hideWorldModels_ = hideWorldModels; }
	bool hideWorldModels() const { return hideWorldModels_; }

	typedef std::vector<std::pair<std::string, std::string> > Map;
	Map last_dirs_;

	vector<unsigned char> dlgBarState;
	vector<unsigned char> dlgBarStateExt;

	int lastToolzerRadius;
	int lastToolzerForm;

	bool showFog_;
	bool zFarInfinite;

	float lod12,lod23;

	bool enableGrid_;
	int gridSpacing_;
	Color4c gridColor_;

	Color4c cameraBorderColor_;
	Color4c cameraBorderMinimapColor_;

	float evolutionSpeed;

	bool showCameraBorders()const{ return showCameraBorders_; }
	void setShowCameraBorders(bool show){ showCameraBorders_ = show; }
	const Recti& cameraBorderSelection()const{ return cameraBorderSelection_; }
	void setCameraBorderSelection(const Recti& rect){ cameraBorderSelection_ = rect; }
protected:
	bool showSources_;
	bool showCameras_;

	bool hideWorldModels_;
	bool showPathFinding_;
	bool showCameraBorders_;
    Recti cameraBorderSelection_;
	std::string showPathFindingReferenceUnit_;

	static const char* configFile;
};

extern SurMapOptions surMapOptions;

#endif
