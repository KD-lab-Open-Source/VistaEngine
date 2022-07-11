#pragma once

#include "Grid2D.h"
#include "XTL\SwapVector.h"
#include "Handle.h"
#include "UnitLink.h"

class ParameterCustom;
class SoundAttribute;
class SoundController;

enum SourceType;
class SourceAttribute;
class SourceBase;
class BaseUniverseObject;
class UnitBase;
class Anchor;

class ShowChangeController;
typedef ShareHandle<ShowChangeController> SharedShowChangeController;

typedef Grid2D<SourceBase, 7, GridVector<SourceBase, 8> > SourceBaseGridType;

class SourceManager
{
public:
	SourceManager();
	~SourceManager();

	void logicQuant();
	void drawUI(float dt);

	void serialize(Archive& ar);

	/// создаЄт, устанавливает и запускает источник
	/// при некоторых услови€х может вернуть ноль
	SourceBase* createSource(const SourceAttribute* attribute, const Se3f& pose, bool allow_limited_lifetime = true, bool* startFlag = 0);
	SourceBase* addSource (const SourceBase* original);
	void flushNewSources();

	void setSourceOnMouse(const SourceBase* source);

	/// добавл€ет на мир €корь дл€ прив€зки
	Anchor* addAnchor();
	Anchor* addAnchor(const Anchor* original);

	void removeAnchor(const Anchor* src);

	SourceBaseGridType sourceGrid;

	typedef SwapVector< ShareHandle<SourceBase> > Sources;
	void getTypeSources(SourceType type, Sources& out);
	Sources& sources() { return sources_; } 

	typedef SwapVector<SharedShowChangeController> ShowChangeControllers;
	ShowChangeController* addShowChangeController(const ShowChangeController& ctrl);

	SourceBase* findSource(const char* sourceName) const;
	string sourceLabelsComboList() const;

	typedef SwapVector<ShareHandle<Anchor> > Anchors;
	Anchors& anchors() { return anchors_; }
	BaseUniverseObject* findAnchor(const char* anchorName) const;
	string anchorLabelsComboList() const;

	bool soundAttach(const SoundAttribute* sound, const BaseUniverseObject *obj);
	void soundRelease(const SoundAttribute* sound, const BaseUniverseObject *obj);
	bool soundIsPlaying(const SoundAttribute* sound, const BaseUniverseObject *obj);

	void deselectAll();
	void deleteSelected();

	void showEditor();
	void showDebug() const;

	int changeControllersSize() const { return showChangeControllers_.size(); }

	// провер€ет позицию на удовлетвор€емость SurfaceClass
	bool checkEnvironment(const Vect3f& pos, int types) const;

	void clearSources();

private:
	Sources sources_;
	Sources newSources_;

	ShowChangeControllers showChangeControllers_;

	// дл€ показа на  –»2006
	UnitLink<SourceBase> sourceOnMouse_;

	Anchors anchors_;

	typedef SwapVector<SoundController> SoundControllers;
	SoundControllers sounds_;

	friend void fCommandAddShowChangeController(XBuffer& stream);
};

extern SourceManager* sourceManager;