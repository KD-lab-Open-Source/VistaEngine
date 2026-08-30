// BorderRollingDialog.h — Qt port of DlgBorderRolling (SurMap5/DlgBorderRolling).
//
// Parameters for the "rolling border" (autoLace) — the world's edge relief:
// the lace height in voxels and the lace angle in degrees. Original: a modal
// CDialog with DDX_Text bound to borderHeight (0..511) and boderAngle (1..90).
#pragma once

#include <QDialog>

class QSpinBox;

class BorderRollingDialog : public QDialog
{
	Q_OBJECT
public:
	explicit BorderRollingDialog(QWidget* parent = nullptr);

	// The lace height in voxels (DlgBorderRolling::borderHeight, 0..511).
	int borderHeight() const;
	void setBorderHeight(int value);

	// The lace angle in degrees (DlgBorderRolling::boderAngle, 1..90).
	int borderAngle() const;
	void setBorderAngle(int value);

private:
	QSpinBox* heightSpin_ = nullptr;
	QSpinBox* angleSpin_ = nullptr;
};
