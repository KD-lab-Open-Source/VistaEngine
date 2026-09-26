// PropertyDelegates.cpp — see header.

#include "PropertyDelegates.h"

#include <QAbstractItemModel>

#include <climits>

#include <QApplication>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QSpinBox>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>

#include "editor/PropertyRows.h"   // concrete rows (engine-free)
#include "PropertyText.h"           // display/encoding round-trip

PropertyRowDelegate::PropertyRowDelegate(QObject* parent)
	: QStyledItemDelegate(parent)
{
}

editor::PropertyRow* PropertyRowDelegate::row(const QModelIndex& index)
{
	// PropertyTree stores the row pointer on column 0 only (buildItem's
	// setData(0, Qt::UserRole, row)); the value editor lives on column 1, so
	// read the sibling's data, not the (empty) column-1 UserRole.
	const QModelIndex nameIndex = index.sibling(index.row(), 0);
	QVariant data = nameIndex.data(Qt::UserRole);
	if(!data.isValid())
		return nullptr;
	return data.value<editor::PropertyRow*>();
}

using propertytext::displayBytes;

bool PropertyRowDelegate::editFlags(editor::PropertyRowBitVector* row, QWidget* parent)
{
	if(!row)
		return false;
	QDialog dlg(parent);
	dlg.setWindowTitle(QObject::tr("Flags"));
	auto* layout = new QVBoxLayout(&dlg);
	auto* list = new QListWidget(&dlg);
	for(size_t i = 0; i < row->entries().size(); ++i){
		auto* item = new QListWidgetItem(displayBytes(row->entries()[i]), list);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState((row->flags() & row->keys()[i]) ? Qt::Checked : Qt::Unchecked);
		item->setData(Qt::UserRole, row->keys()[i]);
	}
	layout->addWidget(list);
	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
	QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
	QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
	layout->addWidget(buttons);
	if(dlg.exec() != QDialog::Accepted)
		return false;
	int flags = 0;
	for(int i = 0; i < list->count(); ++i){
		if(QListWidgetItem* item = list->item(i)){
			if(item->checkState() == Qt::Checked)
				flags |= item->data(Qt::UserRole).toInt();
		}
	}
	row->setFlags(flags);
	return true;
}

bool PropertyRowDelegate::editColor(editor::PropertyRowColor* row, QWidget* parent)
{
	if(!row)
		return false;
	double r, g, b, a;
	row->color(r, g, b, a);
	QColorDialog dlg(QColor((int)r, (int)g, (int)b, (int)a), parent);
	dlg.setWindowTitle(QObject::tr("Color"));
	if(row->hasAlpha())
		dlg.setOptions(QColorDialog::ShowAlphaChannel);
	// ComboListColor palettes show as custom colors (the original's color
	// combo entries).
	std::vector<std::array<double, 4>> palette;
	if(row->colorPalette(palette)){
		for(size_t i = 0; i < palette.size() && i < 16; ++i)
			QColorDialog::setCustomColor((int)i, QColor((int)palette[i][0], (int)palette[i][1],
			                                           (int)palette[i][2], (int)palette[i][3]));
	}
	if(dlg.exec() != QDialog::Accepted)
		return false;
	const QColor c = dlg.currentColor();
	if(!c.isValid())
		return false;
	row->setColor(c.red(), c.green(), c.blue(), row->hasAlpha() ? c.alpha() : a);
	return true;
}

// A slider + spinbox pair editing a ranged row (kdw::PropertyRowRanged's
// floor slider over the numeric entry). Commits through the delegate's
// commitData on release / edit finish.
class RangedEditor : public QWidget
{
public:
	RangedEditor(editor::PropertyRowRanged* row, QWidget* parent)
		: QWidget(parent), row_(row)
	{
		auto* layout = new QHBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);
		slider_ = new QSlider(Qt::Horizontal, this);
		slider_->setRange(0, 1000);
		slider_->setValue(toSlider(row->value()));
		layout->addWidget(slider_, 1);
		if(row->isInteger()){
			intSpin_ = new QSpinBox(this);
			intSpin_->setRange((int)std::ceil(row->minimum() - 0.5), (int)std::floor(row->maximum() + 0.5));
			intSpin_->setSingleStep(row->step() != 0 ? qMax(1, (int)row->step()) : 1);
			intSpin_->setValue((int)row->value());
			layout->addWidget(intSpin_);
			connect(intSpin_, &QSpinBox::valueChanged, this, [this](int v){
				slider_->blockSignals(true);
				slider_->setValue(toSlider(v));
				slider_->blockSignals(false);
			});
		}
		else{
			doubleSpin_ = new QDoubleSpinBox(this);
			doubleSpin_->setRange(row->minimum(), row->maximum());
			doubleSpin_->setDecimals(6);
			doubleSpin_->setSingleStep(row->step() != 0 ? row->step() : (row->maximum() - row->minimum()) / 100.0);
			doubleSpin_->setValue(row->value());
			layout->addWidget(doubleSpin_);
			connect(doubleSpin_, &QDoubleSpinBox::valueChanged, this, [this](double v){
				slider_->blockSignals(true);
				slider_->setValue(toSlider(v));
				slider_->blockSignals(false);
			});
		}
		connect(slider_, &QSlider::valueChanged, this, [this](int pos){
			const double v = toValue(pos);
			if(intSpin_)
				intSpin_->setValue((int)v);
			if(doubleSpin_){
				doubleSpin_->blockSignals(true);
				doubleSpin_->setValue(v);
				doubleSpin_->blockSignals(false);
			}
		});
	}

	double editedValue() const
	{
		if(intSpin_)
			return intSpin_->value();
		if(doubleSpin_)
			return doubleSpin_->value();
		return row_ ? row_->value() : 0.0;
	}

	QSlider* slider_ = nullptr;
	QSpinBox* intSpin_ = nullptr;
	QDoubleSpinBox* doubleSpin_ = nullptr;

private:
	int toSlider(double v) const
	{
		if(!row_ || row_->maximum() <= row_->minimum())
			return 0;
		return (int)((v - row_->minimum()) / (row_->maximum() - row_->minimum()) * 1000.0);
	}
	double toValue(int pos) const
	{
		if(!row_)
			return 0.0;
		return row_->minimum() + (row_->maximum() - row_->minimum()) * pos / 1000.0;
	}
	editor::PropertyRowRanged* row_ = nullptr;
};

// A line edit + "..." button editing a file row (kdw::FileDialog).
class FileEditor : public QWidget
{
public:
	FileEditor(editor::PropertyRowFile* row, QWidget* parent)
		: QWidget(parent), row_(row)
	{
		auto* layout = new QHBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);
		edit_ = new QLineEdit(propertytext::displayRowText(row), this);
		auto* button = new QToolButton(this);
		button->setText(QStringLiteral("..."));
		layout->addWidget(edit_, 1);
		layout->addWidget(button);
		connect(button, &QToolButton::clicked, this, [this]{
			if(!row_)
				return;
			// Start from the current path when absolute, else below the
			// selector's initial directory (kdw::FileDialog rooted there).
			QString start = edit_->text();
			if(!QFileInfo(start).isAbsolute()){
				QString dir = QString::fromStdString(row_->initialDir());
				start = dir + QLatin1Char('/') + start;
			}
			QString picked;
			const QString filter = QString::fromStdString(row_->filter());
			if(row_->save())
				picked = QFileDialog::getSaveFileName(this, QString::fromStdString(row_->title()),
				                                      start, filter);
			else
				picked = QFileDialog::getOpenFileName(this, QString::fromStdString(row_->title()),
				                                      start, filter);
			if(!picked.isEmpty())
				edit_->setText(picked);
		});
	}

	QLineEdit* edit_ = nullptr;

private:
	editor::PropertyRowFile* row_ = nullptr;
};

QWidget* PropertyRowDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/,
                                          const QModelIndex& index) const
{
	editor::PropertyRow* r = row(index);
	if(!r || index.column() != 1)
		return nullptr;
	switch(r->kind()){
	case editor::RowKind::Enum: {
		auto* enumRow = static_cast<editor::PropertyRowEnum*>(r);
		auto* combo = new QComboBox(parent);
		for(const std::string& e : enumRow->entries())
			combo->addItem(propertytext::displayBytes(e));
		combo->setCurrentIndex(enumRow->comboIndex());
		return combo;
	}
	case editor::RowKind::Combo: {
		auto* comboRow = static_cast<editor::PropertyRowCombo*>(r);
		auto* combo = new QComboBox(parent);
		combo->setEditable(true);
		for(const std::string& e : comboRow->entries())
			combo->addItem(propertytext::displayBytes(e));
		combo->setCurrentText(propertytext::displayRowText(comboRow));
		return combo;
	}
	case editor::RowKind::Number: {
		const bool floating = r->typeName() == "float" || r->typeName() == "double";
		if(floating){
			auto* spin = new QDoubleSpinBox(parent);
			spin->setRange(-1e9, 1e9);
			spin->setDecimals(6);
			return spin;
		}
		auto* spin = new QSpinBox(parent);
		spin->setRange(INT_MIN, INT_MAX);
		return spin;
	}
	case editor::RowKind::Ranged: {
		auto* ranged = static_cast<editor::PropertyRowRanged*>(r);
		auto* editor = new RangedEditor(ranged, parent);
		auto* self = const_cast<PropertyRowDelegate*>(this);
		connect(editor->slider_, &QSlider::sliderReleased, self, [self, editor]{
			emit self->commitData(editor);
		});
		if(editor->intSpin_)
			connect(editor->intSpin_, &QSpinBox::editingFinished, self, [self, editor]{
				emit self->commitData(editor);
			});
		if(editor->doubleSpin_)
			connect(editor->doubleSpin_, &QDoubleSpinBox::editingFinished, self, [self, editor]{
				emit self->commitData(editor);
			});
		return editor;
	}
	case editor::RowKind::Text: {
		return new QLineEdit(parent);
	}
	case editor::RowKind::File: {
		auto* fileRow = static_cast<editor::PropertyRowFile*>(r);
		auto* editor = new FileEditor(fileRow, parent);
		auto* self = const_cast<PropertyRowDelegate*>(this);
		connect(editor->edit_, &QLineEdit::editingFinished, self, [self, editor]{
			emit self->commitData(editor);
		});
		return editor;
	}
	default:
		// Bool toggles through the item checkbox, Color/Flags through their
		// dialogs, containers and unknown leaves are display only.
		return nullptr;
	}
}

void PropertyRowDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	editor::PropertyRow* r = row(index);
	if(!r)
		return;
	switch(r->kind()){
	case editor::RowKind::Enum:
		// Current index set at creation; nothing more to sync.
		break;
	case editor::RowKind::Combo:
		if(auto* combo = qobject_cast<QComboBox*>(editor)){
			auto* comboRow = static_cast<editor::PropertyRowCombo*>(r);
			combo->setCurrentText(propertytext::displayRowText(comboRow));
		}
		break;
	case editor::RowKind::Number: {
		const bool floating = r->typeName() == "float" || r->typeName() == "double";
		if(floating){
			if(auto* spin = qobject_cast<QDoubleSpinBox*>(editor))
				spin->setValue(QString::fromStdString(r->valueAsString()).toDouble());
		}
		else{
			if(auto* spin = qobject_cast<QSpinBox*>(editor))
				spin->setValue(QString::fromStdString(r->valueAsString()).toInt());
		}
		break;
	}
	case editor::RowKind::Text:
		if(auto* edit = qobject_cast<QLineEdit*>(editor))
			edit->setText(propertytext::displayRowText(r));
		break;
	default:
		break;
	}
}

void PropertyRowDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                       const QModelIndex& index) const
{
	editor::PropertyRow* r = row(index);
	if(!r)
		return;
	switch(r->kind()){
	case editor::RowKind::Enum:
		if(auto* combo = qobject_cast<QComboBox*>(editor))
			static_cast<editor::PropertyRowEnum*>(r)->setComboIndex(combo->currentIndex());
		break;
	case editor::RowKind::Combo:
		if(auto* combo = qobject_cast<QComboBox*>(editor))
			propertytext::commitRowText(r, combo->currentText());
		break;
	case editor::RowKind::Number:
		if(auto* spin = qobject_cast<QDoubleSpinBox*>(editor))
			r->setValueFromString(std::to_string(spin->value()));
		else if(auto* intSpin = qobject_cast<QSpinBox*>(editor))
			r->setValueFromString(std::to_string(intSpin->value()));
		break;
	case editor::RowKind::Ranged:
		if(auto* ranged = static_cast<RangedEditor*>(editor))
			static_cast<editor::PropertyRowRanged*>(r)->setValue(ranged->editedValue());
		break;
	case editor::RowKind::Text:
		if(auto* edit = qobject_cast<QLineEdit*>(editor))
			propertytext::commitRowText(r, edit->text());
		break;
	case editor::RowKind::File:
		if(auto* file = static_cast<FileEditor*>(editor))
			propertytext::commitRowText(r, file->edit_->text());
		break;
	default:
		return;
	}
	r->setTouched(true);   // the common-tree write-back applies touched rows
	// Update the display cell. Do NOT block the tree's signals: the
	// PropertyTree::itemChanged carries the edit to the panels' write-back
	// (SelectPropertyPanel::applyEdits); blocking it silently dropped edits.
	model->setData(index, propertytext::displayRowText(r), Qt::DisplayRole);
}

void PropertyRowDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                                              const QModelIndex& /*index*/) const
{
	editor->setGeometry(option.rect);
}

void PropertyRowDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
	// Color rows show a swatch next to the hex text (kdw::PropertyRowColor::
	// redraw painted the color chip in the value column).
	if(index.column() == 1){
		if(editor::PropertyRow* r = row(index)){
			if(r->kind() == editor::RowKind::Color){
				auto* colorRow = static_cast<editor::PropertyRowColor*>(r);
				double cr, cg, cb, ca;
				colorRow->color(cr, cg, cb, ca);
				QStyleOptionViewItem opt(option);
				initStyleOption(&opt, index);
				opt.text.clear();
				QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter);
				const int swatch = opt.rect.height() - 6;
				const QRect chip(opt.rect.left() + 3, opt.rect.top() + 3, swatch * 2, swatch);
				painter->save();
				painter->setPen(Qt::darkGray);
				painter->setBrush(QColor((int)cr, (int)cg, (int)cb, (int)ca));
				painter->drawRect(chip);
				painter->restore();
				QRect textRect = opt.rect.adjusted(chip.right() + 6, 0, 0, 0);
				QApplication::style()->drawItemText(painter, textRect, opt.displayAlignment,
				                                    opt.palette, true,
				                                    QString::fromStdString(r->valueAsString()));
				return;
			}
		}
	}
	QStyledItemDelegate::paint(painter, option, index);
}
