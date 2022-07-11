#include "StdAfx.h"
#include "kdw/PropertyRow.h"
#include "kdw/PropertyTreeModel.h"
#include "kdw/PropertyTree.h"

#include "kdw/Dialog.h"
#include "kdw/Button.h"
#include "kdw/Entry.h"
#include "kdw/Label.h"
#include "kdw/HBox.h"
#include "kdw/VBox.h"

#include "kdw/TreeSelector.h"
#include "kdw/ReferenceTreeBuilder.h"

#include "Serialization/SerializationFactory.h"
#include "Serialization/StringTable.h"
#include "Units/Parameters.h"
#include "FormulaString.h"

struct BadNameLookup{
	BadNameLookup(std::string& name)
	: name_(name)
	{}
	void operator()(const char* start, const char* end) const{
		name_ = std::string(start, end);
	}
	mutable std::string& name_;
};

class FormulaDialog : public kdw::Dialog{
public:
	FormulaDialog(kdw::Widget* parent, const ParameterFormulaString formula = ParameterFormulaString())
	: kdw::Dialog(parent)
	{
		setTitle(TRANSLATE("Редактор формул"));
		setDefaultSize(Vect2i(600, 100));
		addButton(TRANSLATE("OK"), kdw::RESPONSE_OK);
		addButton(TRANSLATE("Отмена"), kdw::RESPONSE_CANCEL);
		setResizeable(true);

		kdw::VBox* vbox = new kdw::VBox();
		add(vbox);
		{
			vbox->add(new kdw::Label(TRANSLATE("Формула:")));

			kdw::HBox* hbox = new kdw::HBox();
			vbox->add(hbox);
			{
				formulaEntry_ = new kdw::Entry();
				formulaEntry_->signalChanged().connect(this, &FormulaDialog::onFormulaChanged);
				hbox->add(formulaEntry_, true, true, true);

				kdw::Button* addValueButton = new kdw::Button(TRANSLATE("Добавить"));
				addValueButton->signalPressed().connect(this, &FormulaDialog::onAddButton);
				hbox->add(addValueButton);
			}

			statusLabel_ = new kdw::Label("");
			vbox->add(statusLabel_);
		}
		set(formula);
	}

	void onAddButton()
	{
		ParameterValueReference ref;
		kdw::TreeSelectorDialog dialog(this);
		kdw::ReferenceTreeBuilder<ParameterValueReference> builder(ref);
		dialog.setBuilder(&builder);
		if(dialog.showModal() == kdw::RESPONSE_OK){
			std::string str = "'";
			str += ref.c_str();
			str += "'";
			formulaEntry_->replace(formulaEntry_->selection(), str.c_str());
			formulaEntry_->setFocus();
		}
	}

	void onFormulaChanged()
	{
		formula_.set(formulaEntry_->text());
		const char* buf = formula_.c_str();
		
		std::string badName;
		BadNameLookup badNameLookup(badName);

		float value = 0.0f;
		float xValue = 1.0f;
		ParameterFormulaString::EvalResult result =
			formula_.evaluate(value, xValue, LookupParameter(formula_.group_),
							  ParameterFormulaString::Dummy(), badNameLookup, ParameterFormulaString::Dummy());
		XBuffer msg;
		switch (result) {
		case ParameterFormulaString::EVAL_SUCCESS:
			msg < "Результат: " <= value < " (X = 1.0)";
			break;
		case ParameterFormulaString::EVAL_UNDEFINED_NAME:
			msg < "Не найден параметр: " < badName.c_str();
			break;
		case ParameterFormulaString::EVAL_SYNTAX_ERROR:
			msg < "Ошибка: Плохой синтаксис";
			break;
		};
		statusLabel_->setText(msg);
	}

	void set(const ParameterFormulaString& formula){
		formula_ = formula;
		formulaEntry_->setText(formula_.c_str());
	}
	const ParameterFormulaString& get() const{ return formula_; }
protected:
	kdw::Entry* formulaEntry_;
	kdw::Label* statusLabel_;
	ParameterFormulaString formula_;
};

class PropertyRowFormula : public kdw::PropertyRowImpl<ParameterFormulaString, PropertyRowFormula>{
public:
	PropertyRowFormula(void* object, int size, const char* name, const char* nameAlt, const char* typeName)
	: kdw::PropertyRowImpl<ParameterFormulaString, PropertyRowFormula>(object, size, name, nameAlt, typeName)
	{}
	PropertyRowFormula() {}

	bool onActivateWidget(kdw::PropertyTree* tree, kdw::PropertyRow* hostRow){ return onActivate(tree); }
	std::string valueAsString() const{
		return value().c_str();
	}
	bool onActivate(kdw::PropertyTree* tree){
		FormulaDialog dialog(tree, value());
		
		if(dialog.showModal() == kdw::RESPONSE_OK){
			value() = dialog.get();
			tree->model()->rowChanged(this);
		}
		return true;
	}

};

DECLARE_SEGMENT(PropertyRowFormula)
REGISTER_PROPERTY_ROW(ParameterFormulaString, PropertyRowFormula); 
