// WaveDialog.h — Qt port of CWaveDlg (SurMap5/WaveDlg).
//
// The fixed-waves editor: a list of the world's wave lines with their
// properties (distance, speed, scale, generation time, invert). The original
// drove environment->fixedWaves() (Environment/cFixedWavesContainer), which
// the Qt editor does not create yet — the dialog keeps the UI structure and
// shows an informational message on actions until environment lands.
#pragma once

#include <QDialog>

#include <QStringList>

class QLabel;
class QListWidget;
class QPushButton;
class QCheckBox;
class QLineEdit;

class WaveDialog : public QDialog
{
	Q_OBJECT
public:
	explicit WaveDialog(QWidget* parent = nullptr);

	// The wave-line names to list (cFixedWavesContainer waves).
	void setWaves(const QStringList& names);

private slots:
	void onCreateWave();
	void onRemoveWave();
	void onApply();
	void onListSelectionChanged();

private:
	QListWidget* list_ = nullptr;
	QPushButton* btnCreate_ = nullptr;
	QPushButton* btnRemove_ = nullptr;
	QPushButton* btnApply_ = nullptr;
	QLineEdit* distanceEdit_ = nullptr;
	QLineEdit* speedEdit_ = nullptr;
	QLineEdit* sizeMinEdit_ = nullptr;
	QLineEdit* sizeMaxEdit_ = nullptr;
	QLineEdit* generationTimeEdit_ = nullptr;
	QCheckBox* invertCheck_ = nullptr;
	QLabel* statusLabel_ = nullptr;
};
