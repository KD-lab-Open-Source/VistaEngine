#include "stdafx.h"
#include ".\SurToolPathEditor.h"
#include "..\Game\RenderObjects.h"
#include "..\Game\CameraManager.h"

#include "..\Environment\SourceBase.h"

#include "DebugUtil.h"
#include "NameComboDlg.h"
#include "TypeLibraryImpl.h"
#include "UnitAttribute.h" // FIXME

IMPLEMENT_DYNAMIC(CSurToolPathEditor, CSurToolBase)

BEGIN_MESSAGE_MAP(CSurToolPathEditor, CSurToolBase)
    ON_WM_SIZE()
	ON_EN_CHANGE(IDC_SPEED_EDIT, OnSpeedEditChange)
	ON_BN_CLICKED(IDC_ADD_TO_LIBRARY_BUTTON, OnAddToLibraryButton)
END_MESSAGE_MAP()

CSurToolPathEditor::CSurToolPathEditor(BaseUniverseObject* object)
: CSurToolBase(0, 0)
, source_(safe_cast<SourceBase*>(object))
, selectedNode_(-1)
, moving_(true)
{

}

CSurToolPathEditor::~CSurToolPathEditor()
{

}

bool CSurToolPathEditor::CallBack_TrackingMouse (const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	static Vect3f lastPos = worldCoord;

	if(moving_ && selectedNode_ != -1) {
		if(source_) {
			Vect3f& pos = source_->path()[selectedNode_];
			source_->orientation().xform(pos);
			pos += worldCoord - lastPos;
			source_->orientation().invXform(pos);
		}
		lastPos = worldCoord;
		return true;
	} else {
		lastPos = worldCoord;
	    return false;
	}
}

int CSurToolPathEditor::nodeUnderPoint(const Vect3f& worldCoord)
{
	Vect3f worldPos = worldCoord;
	worldPos.z = 0.0f;

	float grabDistance = 10.0f;

	const SourceBase::Path& path = source_->path();
	for(int i = 0; i < path.size(); ++i) {
		Vect3f pos = path[i];
		source_->orientation().xform(pos);
		pos += source_->origin();
		pos.z = 0.0f;
		if(pos.distance2(worldPos) <= grabDistance*grabDistance) {
			return i;
		}
	}
	return -1;
}

bool CSurToolPathEditor::CallBack_LMBDown (const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	setSelectedNode(nodeUnderPoint(worldCoord));
	if(selectedNode_ == -1) {
		GetAsyncKeyState(VK_SHIFT);
		if(GetAsyncKeyState(VK_SHIFT)) {
			if(source_) {
				Vect3f pos = worldCoord;
				Vect3f origin = source_->origin();
				pos -= origin;
				source_->orientation().invXform(pos);
				source_->path().push_back(SourceBase::PathPosition());
				source_->path().back().set(pos.x, pos.y, pos.z);
				setSelectedNode (source_->path().size() - 1);
			}
			return true;
		} else {
			return false;
		}
	} else {
		moving_ = true;
		return true;
	}
}
bool CSurToolPathEditor::CallBack_LMBUp (const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	moving_ = false;
	if(selectedNode_ == -1) {
		return false;
	} else {
		return true;
	}
}
bool CSurToolPathEditor::CallBack_RMBDown (const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	int node = nodeUnderPoint(worldCoord);
	if(node != -1) {
		GetAsyncKeyState(VK_SHIFT);
		if(GetAsyncKeyState(VK_SHIFT)) {
			if(source_) {
				Vect3f pos = worldCoord;
                source_->path().erase(source_->path().begin() + node);
                setSelectedNode(selectedNode_);
                return true;
			}
		}
    }
    return false;
}
void CSurToolPathEditor::CallBack_SelectionChanged ()
{
}
bool CSurToolPathEditor::CallBack_Delete ()
{
    return false;
}
bool CSurToolPathEditor::CallBack_KeyDown (unsigned int keyCode, bool shift, bool control, bool alt)
{
    return false;
}

bool CSurToolPathEditor::CallBack_DrawAuxData()
{
	const SourceBase::Path& path = source_->path();
	for(int i = 0; i < path.size(); ++i) {
		sColor4f color(1.0f, 1.0f, 1.0f, 1.0f);
		Vect3f pos = path[i];

		source_->orientation().xform(pos);
		pos += source_->origin();
		Vect3f next_pos = path[(i+1) % path.size()];
		source_->orientation().xform(next_pos);
		next_pos += source_->origin();
				
		pos.z = vMap.GetApproxAlt(pos.x, pos.y);
		next_pos.z = vMap.GetApproxAlt(next_pos.x, next_pos.y);

		if(path.cycled_ || i < path.size()-1) {
			gb_RenderDevice->DrawLine(pos, next_pos, color);
		}

		if(selectedNode_ == i) {
			color = sColor4f(1.0f, 0.0f, 0.0f, 1.0f);
		}
		gb_RenderDevice->DrawLine(Vect3f(pos.x-10, pos.y, pos.z),
								  Vect3f(pos.x+10, pos.y, pos.z), color );
		gb_RenderDevice->DrawLine(Vect3f(pos.x, pos.y-10, pos.z),
								  Vect3f(pos.x, pos.y+10, pos.z), color );
		XBuffer buf;
		buf <= path[i].velocity_;
		const char* text = buf;

		Vect3f pos3d = pos;
		Vect3f e,w;
		cameraManager->GetCamera()->ConvertorWorldToViewPort(&pos3d,&w,&e);
		if(w.z > 0)
			gb_RenderDevice->OutText(round(e.x), round(e.y), text, color);
		
	}

	return true;
}

void CSurToolPathEditor::DoDataExchange(CDataExchange* pDX)
{
    CSurToolBase::DoDataExchange(pDX);
}


BOOL CSurToolPathEditor::OnInitDialog()
{
    CSurToolBase::OnInitDialog();

    layout_.init(this);
    layout_.add(1, 0, 0, 0, IDC_SPEED_LABEL);
    layout_.add(0, 0, 1, 0, IDC_SPEED_EDIT);
	layout_.add(1, 0, 1, 0, IDC_ADD_TO_LIBRARY_BUTTON);
#ifdef _VISTA_ENGINE_EXTERNAL_
	GetDlgItem(IDC_ADD_TO_LIBRARY_BUTTON)->EnableWindow(FALSE);
#endif
    return FALSE;
}


void CSurToolPathEditor::OnSize(UINT nType, int cx, int cy)
{
    CSurToolBase::OnSize(nType, cx, cy);

    layout_.onSize(cx, cy);
}

void CSurToolPathEditor::OnSpeedEditChange()
{
	if(!source_)
		return;
	SourceBase::Path& path = source_->path();
	if(selectedNode_ != -1) {
		CString txt;
		GetDlgItemText(IDC_SPEED_EDIT, txt);
		path[selectedNode_].velocity_ = atof (txt);
		GetDlgItem(IDC_SPEED_EDIT)->EnableWindow(TRUE);
	} else {
		GetDlgItem(IDC_SPEED_EDIT)->EnableWindow(FALSE);
	}
}


void CSurToolPathEditor::setSelectedNode(int node)
{
	selectedNode_ = node;
	if(selectedNode_ != -1) {
		SourceBase::Path& path = source_->path();
		CString txt;
		txt.Format("%g", path[selectedNode_].velocity_);
		SetDlgItemText(IDC_SPEED_EDIT, txt);
		GetDlgItem(IDC_SPEED_EDIT)->EnableWindow(TRUE);
	} else {
		SetDlgItemText(IDC_SPEED_EDIT, "");
		GetDlgItem(IDC_SPEED_EDIT)->EnableWindow(FALSE);
	}
}

void CSurToolPathEditor::OnAddToLibraryButton()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	SourceBase* source = source_;
	CNameComboDlg dlg("Source Name", "New Source", SourcesLibrary::instance().comboList());
	if(dlg.DoModal() == IDOK){
		const SourceBase* result = SourceReference(dlg.name());
		if(result){
			if(MessageBox (TRANSLATE("Желаете заменить существующий источник?"), 0, MB_YESNO | MB_ICONQUESTION) == IDYES)
				SourcesLibrary::instance().remove(dlg.name());
			else
				return;
		}
		SourceBase* toAdd = source->clone();
		toAdd->setActivity(false);
		SourcesLibrary::instance().add(SourcesLibrary::StringType(dlg.name(), toAdd));
		SourcesLibrary::instance().saveLibrary();
	}
#endif
}
