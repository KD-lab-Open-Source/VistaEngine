// WorldPropertiesDialog.h — Qt port of CWorldPropertiesDlg
// (SurMap5/WorldPropertiesDlg.{h,cpp}).
//
// A read-only summary of the loaded world: the map's grid size in vertices
// (H_SIZE x V_SIZE) and the creation parameters (size power, world generation
// method, initial height). The original showed only the size powers via
// getEnumNameAlt; we show the vertex sizes (the more useful number) and the
// method.
#pragma once

#include <QDialog>

class QLabel;

class WorldPropertiesDialog : public QDialog
{
	Q_OBJECT
public:
	// hSize/vSize: the loaded map's vertex grid (vMap.H_SIZE/V_SIZE).
	// hSizePower/vSizePower: the SIZE_POWER values (vMap.H_SIZE_POWER/...).
	// createMethod: vrtMapCreationParam::eCreateWorldMetod (0=FullPlain,
	// 1=Mountains). initialHeight: the world's starting height.
	WorldPropertiesDialog(int hSize, int vSize,
	                      int hSizePower, int vSizePower,
	                      int createMethod, int initialHeight,
	                      QWidget* parent = nullptr);

private:
	QLabel* sizeLabel_ = nullptr;
};
