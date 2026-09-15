// TriggerDebuggerPanel.cpp — see header.

#include "TriggerDebuggerPanel.h"

#include <QListWidget>
#include <QSlider>
#include <QVBoxLayout>

#include "RenderViewWidget.h"
#include "TriggerGraphView.h"

namespace {
// Trigger::debugColors (TriggerExport.cpp:452): SLEEPING gray, CHECKING
// yellow, WORKING red, DONE green — the legend the debugger explains.
QString stateName(int state)
{
	switch(state){
	case 0:
		return QStringLiteral("sleeping");
	case 1:
		return QStringLiteral("checking");
	case 2:
		return QStringLiteral("working");
	case 3:
		return QStringLiteral("done");
	default:
		return QStringLiteral("?");
	}
}
}

TriggerDebuggerPanel::TriggerDebuggerPanel(RenderViewWidget* view, TriggerGraphView* graph,
                                           QWidget* parent)
	: QWidget(parent)
	, view_(view)
	, graph_(graph)
{
	list_ = new QListWidget(this);
	slider_ = new QSlider(Qt::Horizontal, this);
	slider_->setMinimum(0);
	slider_->setMaximum(0);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(slider_);
	layout->addWidget(list_, 1);

	connect(list_, &QListWidget::currentRowChanged, this, &TriggerDebuggerPanel::onRowChanged);
	connect(slider_, &QSlider::valueChanged, this, &TriggerDebuggerPanel::onSliderChanged);
}

bool TriggerDebuggerPanel::reload()
{
	records_.clear();
	if(view_)
		view_->triggerLogRecords(records_);
	syncing_ = true;
	list_->clear();
	for(const TriggerLogRecord& rec : records_){
		list_->addItem(QString("%1 — %2 [%3]")
			.arg(QString::fromStdString(rec.event))
			.arg(QString::fromStdString(rec.triggerName))
			.arg(stateName(rec.state)));
	}
	slider_->setMaximum(records_.empty() ? 0 : (int)records_.size() - 1);
	slider_->setValue(records_.empty() ? 0 : (int)records_.size() - 1);
	if(!records_.empty())
		list_->setCurrentRow((int)records_.size() - 1);
	syncing_ = false;
	return !records_.empty();
}

void TriggerDebuggerPanel::onRowChanged(int row)
{
	if(syncing_ || row < 0 || row >= (int)records_.size())
		return;
	// TriggerDebugger::onSelectionChanged: the slider follows the list.
	syncing_ = true;
	slider_->setValue(row);
	syncing_ = false;
	// Scrub the graph to the record's trigger (center on it).
	if(graph_){
		std::vector<TriggerInfo> triggers;
		if(view_)
			view_->triggerList(triggers);
		for(size_t i = 0; i < triggers.size(); ++i){
			if(triggers[i].name == records_[(size_t)row].triggerName){
				graph_->selectTrigger((int)i, false);
				graph_->centerOn((float)triggers[i].cellX * TriggerGraphView::stepX(),
				                 (float)triggers[i].cellY * TriggerGraphView::stepY());
				break;
			}
		}
	}
}

void TriggerDebuggerPanel::onSliderChanged(int value)
{
	if(syncing_ || value < 0 || value >= (int)records_.size())
		return;
	// TriggerDebugger::onSliderChanged: the list follows the slider.
	syncing_ = true;
	list_->setCurrentRow(value);
	syncing_ = false;
	onRowChanged(value);
}
