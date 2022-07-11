#pragma once
#include "Serialization\SerializationTypes.h"
#include "Serialization\StringTableReference.h"

enum TerrainType;
typedef int TerToolsID;

class TerToolLibElement;
typedef StringTableReference<TerToolLibElement, false> TerToolReference;

class TerToolCtrl {
friend class Toolser;
public:
	TerToolCtrl();
	~TerToolCtrl();
	void serialize(Archive& ar);
	bool isFinished() const;

	void start(const Se3f& pos, float _scaleFactor=1.0f);

	void stop();
	void setPosition(const Se3f& pos);
	bool isEmpty() const;

private:
	BitVector<TerrainType> terrainType_;

	//TerToolsID terToolNrmlID;
	//TerToolsID terToolIndsID;
	TerToolsID terToolID;

	string modelName;
	float scaleFactor;
	float currentScaleFactor;
	TerToolReference terToolReference;
	float scaleFactorTerrainType;
};

class Toolser
{
public:
	typedef vector<TerToolCtrl> TerToolCtrls;

	Toolser();

	void serialize(Archive& ar);

	void start(const Se3f& pos);
	void setPosition(const Se3f& pos);
	void stop();
	
	bool isFinished() const;
	bool isEmpty() const;

private:
	TerToolCtrls controllers_;
	float scale_;
	//float vscale_;

public: ///  CONVERSION 2008-1-24
	void setControllers(const TerToolCtrls& ctrls) { controllers_ = ctrls; }
};
