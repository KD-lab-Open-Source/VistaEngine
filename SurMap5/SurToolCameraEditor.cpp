#include "stdafx.h"

#include "SurMap5.h"
#include "SurToolCameraEditor.h"
#include "Game\CameraManager.h"
#include "Game\RenderObjects.h"
#include "Render\Inc\IRenderDevice.h"
#include "SystemUtil.h"

// ---------------------------------------------------------------------------------------------- //
IMPLEMENT_DYNAMIC(CSurToolCameraEditor, CSurToolBase)
BEGIN_MESSAGE_MAP(CSurToolCameraEditor, CSurToolBase)
    ON_WM_SIZE()

	ON_BN_CLICKED(IDC_PLAY_CAMERA_BUTTON, OnPlayCameraButton)
	ON_BN_CLICKED(IDC_VIEW_TO_POINT_BUTTON, OnViewToPointButton)
	ON_BN_CLICKED(IDC_POINT_TO_VIEW_BUTTON, OnPointToViewButton)

	ON_BN_CLICKED(IDC_ORIGIN_TO_VIEW_BUTTON, OnMoveOriginToViewButton)

	ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
	ON_BN_CLICKED(IDC_DELETE_BUTTON, OnDeleteButton)

	ON_BN_CLICKED(IDC_PREV_POINT_BUTTON, OnPrevButton)
	ON_BN_CLICKED(IDC_NEXT_POINT_BUTTON, OnNextButton)

	ON_BN_CLICKED(IDC_DONE_BUTTON, OnDoneButton)
END_MESSAGE_MAP()

// ---------------------------------------------------------------------------------------------- //
CSurToolCameraEditor::CSurToolCameraEditor(BaseUniverseObject* object, bool playAndQuit)
: CSurToolBase(0, 0)
, spline_(safe_cast<CameraSpline*>(object))
, moving_(false)
, selectedPoint_(-1)
, playAndQuit_(playAndQuit)
{
	if(spline_ && !spline_->empty())
		selectedPoint_ = spline_->size() - 1;
}

// ---------------------------------------------------------------------------------------------- //
CSurToolCameraEditor::~CSurToolCameraEditor()
{

}

// ---------------------------------------------------------------------------------------------- //

bool CSurToolCameraEditor::onTrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	static Vect3f lastPos = worldCoord;

	if(moving_ && selectedPoint_ >= 0){
		Vect3f delta = worldCoord - lastPos;
		lastPos = worldCoord;
		spline_->setPointPosition(selectedPoint_, (*spline_)[selectedPoint_].position() + delta);
		return true;
	} else {
		lastPos = worldCoord;
	    return false;
	}
	return false;
}

// ---------------------------------------------------------------------------------------------- //
void CSurToolCameraEditor::addPoint(const Vect3f& worldCoord)
{
	CameraCoordinate currentCoord(cameraManager->coordinate());
	currentCoord.position() -= spline_->position();
	if(spline_->size() <= 1){
		currentCoord.setPosition(worldCoord);
		spline_->push_back(currentCoord);
		selectedPoint_ = spline_->size() - 1;
	}
	else{
		if(selectedPoint_ < 0)
			selectedPoint_ = spline_->size() - 1;
		spline_->addPointAfter(selectedPoint_, currentCoord);
		++selectedPoint_;
	}
}

// ---------------------------------------------------------------------------------------------- //
bool CSurToolCameraEditor::onLMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	if(::isPressed(VK_SHIFT)){
		addPoint(worldCoord);
		moving_ = true;
	}
	else{
		if(moving_)
			moving_ = false;
		
		int index = nodeUnderPoint(worldCoord);
		selectedPoint_ = index;
		
		if(selectedPoint_ >= 0)
			moving_ = true;

	}
	updateControls();
	return true;
}

// ---------------------------------------------------------------------------------------------- //
bool CSurToolCameraEditor::onLMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	moving_ = false;
	if(selectedPoint_ == -1)
		return false;
	else
		return true;
}

// ---------------------------------------------------------------------------------------------- //
bool CSurToolCameraEditor::onRMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
    return false;
}

// ---------------------------------------------------------------------------------------------- //
void CSurToolCameraEditor::onSelectionChanged ()
{
	spline_ = 0;
	popEditorMode();
}

// ---------------------------------------------------------------------------------------------- //
bool CSurToolCameraEditor::onDelete ()
{
    return false;
}

// ---------------------------------------------------------------------------------------------- //
bool CSurToolCameraEditor::onKeyDown(unsigned int keyCode, bool shift, bool control, bool alt)
{
	switch(keyCode){
	case VK_ESCAPE:
	case VK_RETURN:
		OnDoneButton();
		return true;
	case VK_DELETE:
		OnDeleteButton();
		return true;
	}
    return false;
}

bool CSurToolCameraEditor::onDrawAuxData()
{
    if(spline_)
        spline_->showEditor(selectedPoint_);

	int width = gb_RenderDevice->GetSizeX();
	int height = gb_RenderDevice->GetSizeY();

	Rectf rect = aspectedWorkArea(Rectf(0.0f, 0.0f, width, height), 4.0f / 3.0f);

	Color4c color(0, 255, 0, 255);

    gb_RenderDevice->DrawLine(round(rect.left()), round(rect.top()), round(rect.right()), round(rect.top()), color);
    gb_RenderDevice->DrawLine(round(rect.left()), round(rect.top()), round(rect.left()), round(rect.bottom()), color);
    gb_RenderDevice->DrawLine(round(rect.right()), round(rect.top()), round(rect.right()), round(rect.bottom()), color);
    gb_RenderDevice->DrawLine(round(rect.left()), round(rect.bottom()), round(rect.right()), round(rect.bottom()), color);

	float crossSize = 10.0f;
	Vect3f pos = cameraManager->coordinate().position();
	gb_RenderDevice->DrawLine(pos - Vect3f::I * crossSize, pos + Vect3f::I * crossSize, Color4c(255, 255, 255, 64));
	gb_RenderDevice->DrawLine(pos - Vect3f::J * crossSize, pos + Vect3f::J * crossSize, Color4c(255, 255, 255, 64));
	return true;
}

void CSurToolCameraEditor::DoDataExchange(CDataExchange* pDX)
{
    CSurToolBase::DoDataExchange(pDX);
}


BOOL CSurToolCameraEditor::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	layout_.init(this);


	layout_.add(1, 1, 1, 0, IDC_PLAY_CAMERA_BUTTON);
	SetDlgItemText(IDC_PLAY_CAMERA_BUTTON, TRANSLATE("Проиграть сплайн"));
	layout_.add(1, 1, 1, 0, IDC_VIEW_TO_POINT_BUTTON);
	SetDlgItemText(IDC_VIEW_TO_POINT_BUTTON, TRANSLATE("Установить точку по виду"));
	layout_.add(1, 1, 1, 0, IDC_POINT_TO_VIEW_BUTTON);
	SetDlgItemText(IDC_POINT_TO_VIEW_BUTTON, TRANSLATE("Установить вид по точке"));

	layout_.add(1, 1, 1, 0, IDC_ORIGIN_TO_VIEW_BUTTON);
	SetDlgItemText(IDC_ORIGIN_TO_VIEW_BUTTON, TRANSLATE("Установить опорную точку по виду"));

	layout_.add(1, 0, 1, 0, IDC_ADD_BUTTON);
	SetDlgItemText(IDC_ADD_BUTTON, TRANSLATE("Добавить точку [Shift+LMB]"));
	layout_.add(1, 0, 1, 0, IDC_DELETE_BUTTON);
	SetDlgItemText(IDC_DELETE_BUTTON, TRANSLATE("Удалить точку [Delete]"));

	layout_.add(1, 1, 0, 0, IDC_PREV_POINT_BUTTON);
	layout_.add(0, 1, 1, 0, IDC_NEXT_POINT_BUTTON);
	
	layout_.add(1, 1, 1, 0, IDC_POINT_LABEL);

	layout_.add(0, 0, 1, 1, IDC_DONE_BUTTON);
	SetDlgItemText(IDC_DONE_BUTTON, TRANSLATE("Готово [Enter]"));

	cameraManager->selectSpline(spline_);

	if(spline_ && !spline_->empty()){
		if(playAndQuit_){
			if(cameraManager->isPlayingBack())
				cameraManager->stopReplayPath();

			cameraManager->loadPath(*spline_, false);
			cameraManager->startReplayPath(spline_->stepDuration(), 1);
			selectedPoint_ = -1;
		}
		else{
			selectedPoint_ = spline_->size() - 1;
		}
	}
	updateControls();
	return FALSE;
}


void CSurToolCameraEditor::OnSize(UINT nType, int cx, int cy)
{
    CSurToolBase::OnSize(nType, cx, cy);

    layout_.onSize(cx, cy);
}

int CSurToolCameraEditor::nodeUnderPoint(const Vect2f& point)
{
	xassert(spline_);
	float pointRadius = 15.0f;

	// Если кликаем в радиусе заселекченнок точки, не выделяем других
	if(selectedPoint_ >= 0 && selectedPoint_ < spline_->size()){
		if(Vect2f((*spline_)[selectedPoint_].position() + spline_->position()).distance2(point) < sqr(pointRadius))
			return selectedPoint_;
	}

	xassert(spline_);
	CameraSpline::Spline::const_iterator it;
	int index = 0;
	FOR_EACH(spline_->spline(), it){
		const CameraCoordinate& coord = *it;
		if(Vect2f(coord.position() + spline_->position()).distance2(point) < sqr(pointRadius))
			return index;
		++index;
	}    
	return -1;
}

void CSurToolCameraEditor::updateControls()
{
	if(selectedPoint_ >= 0 && !cameraManager->isPlayingBack()){
		GetDlgItem(IDC_VIEW_TO_POINT_BUTTON)->EnableWindow(TRUE);
		GetDlgItem(IDC_POINT_TO_VIEW_BUTTON)->EnableWindow(TRUE);
		GetDlgItem(IDC_DELETE_BUTTON)->EnableWindow(TRUE);
	}
	else{
		GetDlgItem(IDC_VIEW_TO_POINT_BUTTON)->EnableWindow(FALSE);
		GetDlgItem(IDC_POINT_TO_VIEW_BUTTON)->EnableWindow(FALSE);
		GetDlgItem(IDC_DELETE_BUTTON)->EnableWindow(FALSE);
	}

    BOOL playing = cameraManager->isPlayingBack() ? TRUE : FALSE;
	GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(!playing);
	GetDlgItem(IDC_ORIGIN_TO_VIEW_BUTTON)->EnableWindow(!playing);

	XBuffer buf;
	if(selectedPoint_ >= 0)
		buf <= (selectedPoint_ + 1);
	else
		buf < "-";
	buf < " / ";
	buf <= spline_->size();

	SetDlgItemText(IDC_POINT_LABEL, buf);
}

// ---------------------------------------------------------------------------------------------- //
void CSurToolCameraEditor::OnPlayCameraButton()
{
	if(spline_){
		if(cameraManager->isPlayingBack())
			cameraManager->stopReplayPath();
		else{
			cameraManager->loadPath(*spline_, false);
			cameraManager->startReplayPath(spline_->stepDuration(), 1);
			selectedPoint_ = -1;
		}
		updateControls();
	}
}

// ---------------------------------------------------------------------------------------------- //
void CSurToolCameraEditor::OnViewToPointButton()
{
	if(spline_ && selectedPoint_ >= 0){
		(*spline_)[selectedPoint_] = cameraManager->coordinate();
		spline_->setPointPosition(selectedPoint_, cameraManager->coordinate().position() - spline_->position());
	}
	else
		xassert(0);
}

// ---------------------------------------------------------------------------------------------- //
void CSurToolCameraEditor::OnPointToViewButton()
{
	if(spline_ && selectedPoint_ >= 0)
		cameraManager->setCoordinate((*spline_)[selectedPoint_] + CameraCoordinate(spline_->position(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	else
		xassert(0);
}
// ---------------------------------------------------------------------------------------------- //
void CSurToolCameraEditor::OnMoveOriginToViewButton()
{
    if(spline_ && selectedPoint_ >= 0){
        CameraSpline& spline = *spline_;
        Vect3f offset = cameraManager->coordinate().position() - spline.position();
        for(int i = 0; i < spline.size(); ++i){
            spline.setPointPosition(i, spline[i].position() - offset);
        }
        spline.setPose(Se3f(spline.orientation(), spline.position() + offset), true);
    }
    else
        xassert(0);
}

// ---------------------------------------------------------------------------------------------- //
void CSurToolCameraEditor::OnNextButton()
{
	if(!spline_->empty())
		selectedPoint_ = clamp(selectedPoint_ + 1, 0, spline_->size() - 1);
	updateControls();
}

// ---------------------------------------------------------------------------------------------- //
void CSurToolCameraEditor::OnPrevButton()
{
	if(!spline_->empty())
		selectedPoint_ = clamp(selectedPoint_ - 1, 0, spline_->size() - 1);
	updateControls();
}

// ---------------------------------------------------------------------------------------------- //
void CSurToolCameraEditor::OnAddButton()
{
	addPoint(cameraManager->coordinate().position() - spline_->position());
	updateControls();
}

// ---------------------------------------------------------------------------------------------- //
void CSurToolCameraEditor::OnDeleteButton()
{
	if(spline_ && selectedPoint_ >= 0){
		spline_->erasePoint(selectedPoint_);
		selectedPoint_ = -1;

		updateControls();
	}
}

// ---------------------------------------------------------------------------------------------- //
void CSurToolCameraEditor::OnDoneButton()
{
	if(cameraManager->isPlayingBack())
		cameraManager->stopReplayPath();

	replaceEditorModeSelect();
}

// ---------------------------------------------------------------------------------------------- //
void CSurToolCameraEditor::quant()
{
	bool playing = cameraManager->isPlayingBack();
	if(playAndQuit_ && !playing)
		popEditorMode();

	static bool lastTimePlaying = playing;
	if(::IsWindow(GetSafeHwnd()) && ::IsWindowVisible(GetSafeHwnd())){
		if(playing != lastTimePlaying)
			updateControls();
		lastTimePlaying = playing;
	}
}
