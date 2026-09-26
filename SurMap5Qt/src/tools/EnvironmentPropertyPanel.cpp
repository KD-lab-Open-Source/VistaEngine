// EnvironmentPropertyPanel.cpp — see header.

#include "EnvironmentPropertyPanel.h"

#include <QFormLayout>
#include <QLabel>
#include <QString>

#include <string>
#include <vector>

#include "EnvironmentTool.h"
#include "panels/PropertyText.h"   // propertytext::displayBytes (cp1251 type names)
#include "editor/EditorTool.h"      // IWorldBridge, EnvironmentParams

namespace {
// The display name of a model path: the file part after the last separator.
QString modelLabel(const std::string& path)
{
	const size_t pos = path.find_last_of("\\/");
	const std::string base = pos == std::string::npos ? path : path.substr(pos + 1);
	QString label = QString::fromStdString(base);
	if(label.endsWith(".3dx", Qt::CaseInsensitive))
		label.chop(4);
	return label;
}
}

EnvironmentPropertyPanel::EnvironmentPropertyPanel(QWidget* parent)
	: QWidget(parent)
{
	model_ = new QComboBox(this);
	model_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	type_ = new QComboBox(this);
	type_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

	auto makeSpin = [this](int lo, int hi, int step){
		auto* s = new QSpinBox(this);
		s->setRange(lo, hi);
		s->setSingleStep(step);
		return s;
	};
	angle_ = makeSpin(0, 360, 15);
	angleDelta_ = makeSpin(0, 180, 15);
	scale_ = makeSpin(0, 300, 1);
	scaleDelta_ = makeSpin(0, 80, 1);
	spreadRadius_ = makeSpin(5, 200, 2);
	spreadRadiusDelta_ = makeSpin(1, 80, 5);

	spread_ = new QCheckBox(tr("Spread"), this);
	vertical_ = new QCheckBox(tr("Vertical"), this);

	auto* layout = new QFormLayout(this);
	layout->addRow(tr("Model"), model_);
	layout->addRow(tr("Type"), type_);
	layout->addRow(tr("Angle"), angle_);
	layout->addRow(tr("Angle spread"), angleDelta_);
	layout->addRow(tr("Scale %"), scale_);
	layout->addRow(tr("Scale spread %"), scaleDelta_);
	layout->addRow(tr("Spread radius"), spreadRadius_);
	layout->addRow(tr("Spread radius delta %"), spreadRadiusDelta_);
	layout->addRow(spread_);
	layout->addRow(vertical_);

	connect(model_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]{
		if(loading_ || !tool_)
			return;
		// The tool sets the model and pulls its type/vertical from the name;
		// re-read the widgets to show the inferred type.
		tool_->setModel(model_->currentData().toString().toStdString());
		loading_ = true;
		readParams();
		loading_ = false;
	});
	connect(type_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]{ writeParams(); });
	connect(angle_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]{ writeParams(); });
	connect(angleDelta_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]{ writeParams(); });
	connect(scale_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]{ writeParams(); });
	connect(scaleDelta_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]{ writeParams(); });
	connect(spreadRadius_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]{ writeParams(); });
	connect(spreadRadiusDelta_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]{ writeParams(); });
	connect(spread_, &QCheckBox::toggled, this, [this]{ writeParams(); });
	connect(vertical_, &QCheckBox::toggled, this, [this]{ writeParams(); });
}

void EnvironmentPropertyPanel::reloadModels()
{
	model_->blockSignals(true);
	model_->clear();
	IWorldBridge* bridge = tool_ ? tool_->bridge() : nullptr;
	const std::string selected = tool_ ? tool_->params().model : std::string();
	if(bridge){
		std::vector<std::string> names;
		bridge->environmentModelNames(names);
		for(const std::string& n : names)
			model_->addItem(modelLabel(n), QString::fromStdString(n));
	}
	if(model_->count() == 0)
		model_->addItem(tr("(load a world to list models)"), QString());
	model_->blockSignals(false);

	if(tool_){
		const int index = model_->findData(QString::fromStdString(selected));
		if(index >= 0)
			model_->setCurrentIndex(index);
	}
}

void EnvironmentPropertyPanel::reloadTypes()
{
	type_->blockSignals(true);
	type_->clear();
	IWorldBridge* bridge = tool_ ? tool_->bridge() : nullptr;
	if(bridge){
		std::vector<std::string> names;
		bridge->environmentTypeNames(names);
		for(const std::string& n : names)
			type_->addItem(propertytext::displayBytes(n));
	}
	if(type_->count() == 0)
		type_->addItem(tr("(load a world to list types)"));
	type_->blockSignals(false);
}

void EnvironmentPropertyPanel::readParams()
{
	if(!tool_)
		return;
	const EnvironmentParams& p = tool_->params();
	angle_->setValue((int)p.angle);
	angleDelta_->setValue((int)p.angleDelta);
	scale_->setValue((int)p.scale);
	scaleDelta_->setValue((int)p.scaleDelta);
	spreadRadius_->setValue((int)p.spreadRadius);
	spreadRadiusDelta_->setValue((int)p.spreadRadiusDelta);
	spread_->setChecked(p.spread);
	vertical_->setChecked(p.vertical);
	if(!p.model.empty()){
		const int index = model_->findData(QString::fromStdString(p.model));
		if(index >= 0)
			model_->setCurrentIndex(index);
	}
	if(p.typeIndex >= 0 && p.typeIndex < type_->count())
		type_->setCurrentIndex(p.typeIndex);
}

void EnvironmentPropertyPanel::writeParams()
{
	if(loading_ || !tool_)
		return;
	EnvironmentParams& p = tool_->params();
	// An invalid item (the placeholder) carries empty data — leave the model.
	const QVariant data = model_->currentData();
	if(data.isValid() && !data.toString().isEmpty())
		p.model = data.toString().toStdString();
	p.typeIndex = type_->currentIndex() >= 0 ? type_->currentIndex() : 0;
	p.angle = (float)angle_->value();
	p.angleDelta = (float)angleDelta_->value();
	p.scale = (float)scale_->value();
	p.scaleDelta = (float)scaleDelta_->value();
	p.spreadRadius = (float)spreadRadius_->value();
	p.spreadRadiusDelta = (float)spreadRadiusDelta_->value();
	p.spread = spread_->isChecked();
	p.vertical = vertical_->isChecked();
	tool_->applyParams();
}

void EnvironmentPropertyPanel::setTool(EnvironmentTool* tool)
{
	tool_ = tool;
	loading_ = true;
	reloadModels();
	reloadTypes();
	readParams();
	loading_ = false;
	// A fresh tool with no model gets the first list entry (the original picked
	// model.3dx by default); push it (which also sets its type/vertical) so the
	// first click has something to place.
	if(tool_ && tool_->params().model.empty() && !model_->currentData().toString().isEmpty()){
		tool_->setModel(model_->currentData().toString().toStdString());
		loading_ = true;
		readParams();
		loading_ = false;
	}
}
