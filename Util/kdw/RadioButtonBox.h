#ifndef __KDW_RADIOBUTTONBOX_H_INCLUDED__
#define __KDW_RADIOBUTTONBOX_H_INCLUDED__

#include "kdw/Widget.h"
Kdw/Frame.h
kdw/VBox.h
kdw/RadioButton.h
#include <string>
#include <vector>


namespace kdw{

	class KDW_API RadioButtonBox : public kdw::Frame 
	{
	public:
		RadioButtonBox(const char* text = "RadioButtonBox", int border = 0);
		~RadioButtonBox();

		void addRadioButton(const char * name);

		void setSelectedIndex(int index);
		int selectedIndex() const;

		virtual void onChangedSelection();
		sigslot::signal0& signalChangedSelection() { return signalChangedSelection_; }

		void serialize(Archive& ar);
		
	protected:
		sigslot::signal0 signalChangedSelection_;

		std::vector<kdw::RadioButton*> radioButtons_;
		kdw::VBox* box_;
	};
}

#endif
