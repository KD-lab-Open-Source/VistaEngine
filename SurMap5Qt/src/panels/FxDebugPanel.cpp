// TEMP FX debug panel — временные кнопки управления частицами.
// Убрать после диагностики.

#include "FxDebugPanel.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "RenderViewWidget.h"

FxDebugPanel::FxDebugPanel(RenderViewWidget* view, QWidget* parent)
	: QWidget(parent)
	, view_(view)
{
	countLabel_ = new QLabel(tr("Effects: —"), this);

	visibleBtn_ = new QPushButton(tr("Hide FX"), this);
	visibleBtn_->setCheckable(true);
	visibleBtn_->setChecked(true);
	emittingBtn_ = new QPushButton(tr("Stop emit"), this);
	emittingBtn_->setCheckable(true);
	emittingBtn_->setChecked(true);
	pauseBtn_ = new QPushButton(tr("Pause time"), this);
	pauseBtn_->setCheckable(true);
	pauseBtn_->setChecked(false);
	isolateBtn_ = new QPushButton(tr("Isolate FX"), this);
	isolateBtn_->setCheckable(true);
	isolateBtn_->setChecked(false);
	noDepthBtn_ = new QPushButton(tr("No depth"), this);
	noDepthBtn_->setCheckable(true);
	noDepthBtn_->setChecked(false);
	noFogBtn_ = new QPushButton(tr("No fog"), this);
	noFogBtn_->setCheckable(true);
	noFogBtn_->setChecked(false);
	noSoftBtn_ = new QPushButton(tr("No soft"), this);
	noSoftBtn_->setCheckable(true);
	noSoftBtn_->setChecked(false);
	noPremulBtn_ = new QPushButton(tr("No premul"), this);
	noPremulBtn_->setCheckable(true);
	noPremulBtn_->setChecked(false);
	wireBtn_ = new QPushButton(tr("Wire quads"), this);
	wireBtn_->setCheckable(true);
	wireBtn_->setChecked(false);
	flatBtn_ = new QPushButton(tr("Flat quad"), this);
	flatBtn_->setCheckable(true);
	flatBtn_->setChecked(false);
	restartBtn_ = new QPushButton(tr("Restart all"), this);
	refreshBtn_ = new QPushButton(tr("Refresh"), this);

	auto* row1 = new QHBoxLayout;
	row1->addWidget(visibleBtn_);
	row1->addWidget(emittingBtn_);
	auto* row2 = new QHBoxLayout;
	row2->addWidget(pauseBtn_);
	row2->addWidget(isolateBtn_);
	row2->addWidget(restartBtn_);
	row2->addWidget(refreshBtn_);
	// TEMP FX debug: по одному выключать гасители + проволочный каркас.
	auto* row3 = new QHBoxLayout;
	row3->addWidget(noDepthBtn_);
	row3->addWidget(noFogBtn_);
	row3->addWidget(noSoftBtn_);
	row3->addWidget(noPremulBtn_);
	row3->addWidget(wireBtn_);
	row3->addWidget(flatBtn_);

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(6, 6, 6, 6);
	layout->addWidget(countLabel_);
	layout->addLayout(row1);
	layout->addLayout(row2);
	layout->addLayout(row3);
	layout->addStretch();

	connect(visibleBtn_, &QPushButton::toggled, this, &FxDebugPanel::toggleVisible);
	connect(emittingBtn_, &QPushButton::toggled, this, &FxDebugPanel::toggleEmitting);
	connect(pauseBtn_, &QPushButton::toggled, this, &FxDebugPanel::togglePaused);
	connect(isolateBtn_, &QPushButton::toggled, this, &FxDebugPanel::toggleIsolated);
	connect(noDepthBtn_, &QPushButton::toggled, this, &FxDebugPanel::toggleForceNoDepth);
	connect(noFogBtn_, &QPushButton::toggled, this, &FxDebugPanel::toggleForceNoFog);
	connect(noSoftBtn_, &QPushButton::toggled, this, &FxDebugPanel::toggleForceNoSoft);
	connect(noPremulBtn_, &QPushButton::toggled, this, &FxDebugPanel::toggleForceNoPremul);
	connect(wireBtn_, &QPushButton::toggled, this, &FxDebugPanel::toggleWireParticles);
	connect(flatBtn_, &QPushButton::toggled, this, &FxDebugPanel::toggleForceFlat);
	connect(restartBtn_, &QPushButton::clicked, this, &FxDebugPanel::restartAll);
	connect(refreshBtn_, &QPushButton::clicked, this, &FxDebugPanel::refresh);

	refresh();
}

void FxDebugPanel::refresh()
{
	if(!view_)
		return;
	countLabel_->setText(tr("Effects: %1").arg(view_->fxEffectCount()));
	// Синхрон кнопок с реальным состоянием (если мир перезагрузили).
	visibleBtn_->setChecked(view_->fxVisible());
	visibleBtn_->setText(view_->fxVisible() ? tr("Hide FX") : tr("Show FX"));
	pauseBtn_->setChecked(view_->fxPaused());
	isolateBtn_->setChecked(view_->fxIsolated());
	isolateBtn_->setText(view_->fxIsolated() ? tr("Show all") : tr("Isolate FX"));
	wireBtn_->setChecked(view_->fxWireParticles());
	flatBtn_->setChecked(view_->fxForceFlat());
}

void FxDebugPanel::toggleVisible(bool on)
{
	if(view_)
		view_->fxSetVisible(on);
	visibleBtn_->setText(on ? tr("Hide FX") : tr("Show FX"));
	refresh();
}

void FxDebugPanel::toggleEmitting(bool on)
{
	if(view_)
		view_->fxSetEmitting(on);
	emittingBtn_->setText(on ? tr("Stop emit") : tr("Start emit"));
	refresh();
}

void FxDebugPanel::togglePaused(bool on)
{
	if(view_)
		view_->fxSetPaused(on);
	refresh();
}

void FxDebugPanel::toggleIsolated(bool on)
{
	if(view_)
		view_->fxSetIsolated(on);
	isolateBtn_->setText(on ? tr("Show all") : tr("Isolate FX"));
	refresh();
}

void FxDebugPanel::toggleForceNoDepth(bool on)
{
	if(view_)
		view_->fxSetForceNoDepth(on);
	refresh();
}

void FxDebugPanel::toggleForceNoFog(bool on)
{
	if(view_)
		view_->fxSetForceNoFog(on);
	refresh();
}

void FxDebugPanel::toggleForceNoSoft(bool on)
{
	if(view_)
		view_->fxSetForceNoSoft(on);
	refresh();
}

void FxDebugPanel::toggleForceNoPremul(bool on)
{
	if(view_)
		view_->fxSetForceNoPremul(on);
	refresh();
}

void FxDebugPanel::toggleWireParticles(bool on)
{
	if(view_)
		view_->fxSetWireParticles(on);
	refresh();
}

void FxDebugPanel::toggleForceFlat(bool on)
{
	if(view_)
		view_->fxSetForceFlat(on);
	refresh();
}

void FxDebugPanel::restartAll()
{
	if(view_)
		view_->fxRestartAll();
	refresh();
}
