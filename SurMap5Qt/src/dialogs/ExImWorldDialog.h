// ExImWorldDialog.h — Qt port of CDlgExImWorld (SurMap5/DlgExImWorld.{h,cpp}).
//
// The Export/Import World dialog: lists the worlds in the worlds directory and
// runs packer.exe to pack one into a .pkw (export) or unpack a .pkw into the
// directory (import). The original spawned packer.exe with a command line and
// waited; Qt uses QProcess, which also keeps the UI pumping.
#pragma once

#include <QDialog>

#include <QString>

class QLabel;
class QPushButton;
class QTableView;
class WorldList;

class ExImWorldDialog : public QDialog
{
	Q_OBJECT
public:
	// path2worlds: the directory the worlds live in (vMap.getWorldsDir()).
	// packerPath: the packer.exe executable (same dir as the app by default).
	ExImWorldDialog(const QString& path2worlds,
	                const QString& packerPath = QString(),
	                QWidget* parent = nullptr);
	~ExImWorldDialog() override;

private slots:
	void fillWorldList();
	void onImport();
	void onExport();

private:
	// The world name of the current list row, or an empty string.
	QString currentWorldName() const;

	QTableView* table_ = nullptr;
	WorldList* worldList_ = nullptr;
	QString path2worlds_;
	QString packerPath_;
	QLabel* statusLabel_ = nullptr;
	QPushButton* btnExport_ = nullptr;
	QPushButton* btnImport_ = nullptr;

	static int s_previsionWorldSelect;
};
