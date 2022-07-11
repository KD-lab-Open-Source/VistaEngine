#include "stdafx.h"
#include "ConfiguratorAttribEditor.h"
#include "ConfiguratorDlg.h"
#include "XTL\SafeCast.h"

void CConfiguratorAttribEditor::beforeResave()
{
    __super::beforeResave();
	CWnd* window = AfxGetMainWnd();
	if(window && window->IsKindOf(RUNTIME_CLASS(CConfiguratorDlg))){
		if(CConfiguratorDlg* dlg = safe_cast<CConfiguratorDlg*>(window))
			dlg->onConfigChanged();
	}
}
