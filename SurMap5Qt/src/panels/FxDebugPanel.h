#pragma once

// TEMP FX debug panel — временные кнопки управления частицами.
// Убрать после диагностики (вместе с fx*-методами).

#include <QWidget>

class QLabel;
class QPushButton;
class RenderViewWidget;

class FxDebugPanel : public QWidget
{
	Q_OBJECT
public:
	explicit FxDebugPanel(RenderViewWidget* view, QWidget* parent = nullptr);

private slots:
	void refresh();
	void toggleVisible(bool on);
	void toggleEmitting(bool on);
	void togglePaused(bool on);
	void toggleIsolated(bool on);
	void toggleForceNoDepth(bool on);
	void toggleForceNoFog(bool on);
	void toggleForceNoSoft(bool on);
	void toggleForceNoPremul(bool on);
	void toggleWireParticles(bool on);
	void toggleForceFlat(bool on);
	void restartAll();

private:
	RenderViewWidget* view_ = nullptr;
	QLabel* countLabel_ = nullptr;
	QPushButton* visibleBtn_ = nullptr;
	QPushButton* emittingBtn_ = nullptr;
	QPushButton* pauseBtn_ = nullptr;
	QPushButton* isolateBtn_ = nullptr;
	QPushButton* noDepthBtn_ = nullptr;
	QPushButton* noFogBtn_ = nullptr;
	QPushButton* noSoftBtn_ = nullptr;
	QPushButton* noPremulBtn_ = nullptr;
	QPushButton* wireBtn_ = nullptr;
	QPushButton* flatBtn_ = nullptr;
	QPushButton* restartBtn_ = nullptr;
	QPushButton* refreshBtn_ = nullptr;
};
