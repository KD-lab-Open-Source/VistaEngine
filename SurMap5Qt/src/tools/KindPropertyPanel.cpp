// KindPropertyPanel.cpp — see header.

#include "KindPropertyPanel.h"

#include <QFormLayout>
#include <QIcon>
#include <QPixmap>

#include <string>
#include <vector>

#include "KindTool.h"
#include "editor/EditorTool.h"   // IWorldBridge

namespace {
// A solid-colour swatch the way the original's IDC_COLORTERRAINTYPE static
// showed TerrainTypeDescriptor::getColors()[kind].
QIcon swatch(unsigned colorRGB)
{
	QPixmap pm(16, 16);
	pm.fill(QColor((colorRGB >> 16) & 0xff, (colorRGB >> 8) & 0xff, colorRGB & 0xff));
	return QIcon(pm);
}
}

KindPropertyPanel::KindPropertyPanel(QWidget* parent)
	: QWidget(parent)
{
	kind_ = new QComboBox(this);

	auto* layout = new QFormLayout(this);
	layout->addRow(tr("Surface"), kind_);

	connect(kind_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
		if(tool_ && index >= 0)
			tool_->setKind(index);
	});

	reloadTypes();
}

void KindPropertyPanel::setBridge(IWorldBridge* bridge)
{
	bridge_ = bridge;
	reloadTypes();
}

void KindPropertyPanel::reloadTypes()
{
	kind_->clear();
	if(bridge_){
		std::vector<std::string> names;
		std::vector<unsigned> colors;
		bridge_->terrainTypeNames(names, colors);
		for(size_t i = 0; i < names.size(); ++i){
			kind_->addItem(swatch(i < colors.size() ? colors[i] : 0x808080u),
			               QString::fromUtf8(names[i].c_str()));
		}
	}
	// The original always had 16 entries; fall back to numbered placeholders
	// when the descriptor is not loaded yet.
	if(kind_->count() == 0)
		for(int i = 0; i < 16; ++i)
			kind_->addItem(tr("Type %1").arg(i));
	if(tool_)
		kind_->setCurrentIndex(tool_->kind());
}

void KindPropertyPanel::setTool(KindTool* tool)
{
	tool_ = tool;
	// The type names/colors come from the same bridge the tool paints through.
	bridge_ = tool ? tool->bridge() : nullptr;
	reloadTypes();
	if(tool_)
		kind_->setCurrentIndex(tool_->kind());
}
