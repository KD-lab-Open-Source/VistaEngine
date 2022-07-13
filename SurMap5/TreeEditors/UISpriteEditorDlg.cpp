#include "stdafx.h"
#include ".\UISpriteEditorDlg.h"
#include "UISpriteEditor.h"
#include "..\AttribEditor\AttribEditorCtrl.h"
#include "..\..\UserInterface\UserInterface.h"
#include "..\..\Units\UnitAttribute.h"
#include "Dictionary.h"
#include "TypeLibraryImpl.h"

#include "SolidColorCtrl.h"
#include "mfc\SizeLayoutManager.h"


IMPLEMENT_DYNAMIC(CUISpriteEditorDlg, CDialog)
CUISpriteEditorDlg::CUISpriteEditorDlg(UI_Sprite& sprite, CWnd* pParent /*=NULL*/)
: CDialog(CUISpriteEditorDlg::IDD, pParent)
, sprite_(sprite)
, diffuseColorSelector_(new CSolidColorCtrl)
, backgroundColorSelector_(new CSolidColorCtrl)
, layout_(new CSizeLayoutManager)
, editor_(new CUISpriteEditor)
{
	editor_->setSprite(sprite_);
}

CUISpriteEditorDlg::~CUISpriteEditorDlg()
{
}

void CUISpriteEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VIEW, *editor_);
	DDX_Control(pDX, IDC_COORD_LEFT_EDIT, m_ctlCoordLeftEdit);
	DDX_Control(pDX, IDC_COORD_TOP_EDIT, m_ctlCoordTopEdit);
	DDX_Control(pDX, IDC_COORD_WIDTH_EDIT, m_ctlCoordWidthEdit);
	DDX_Control(pDX, IDC_COORD_HEIGHT_EDIT, m_ctlCoordHeightEdit);
	DDX_Control(pDX, IDC_TEXTURES_COMBO, m_ctlTexturesCombo);
	DDX_Control(pDX, IDC_TEXTURE_SIZE_LABEL, m_ctlTextureSizeLabel);
	DDX_Control(pDX, IDC_SATURATION_SLIDER, saturationSlider_);

	DDX_Control(pDX, IDC_DIFFUSE_COLOR, *diffuseColorSelector_);
	DDX_Control(pDX, IDC_BACKGROUND_COLOR, *backgroundColorSelector_);
}


BEGIN_MESSAGE_MAP(CUISpriteEditorDlg, CDialog)
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_TEXTURES_COMBO, OnTexturesComboSelChange)
	ON_EN_CHANGE(IDC_COORD_LEFT_EDIT, OnCoordChange)
	ON_EN_CHANGE(IDC_COORD_TOP_EDIT, OnCoordChange)
	ON_EN_CHANGE(IDC_COORD_WIDTH_EDIT, OnCoordChange)
	ON_EN_CHANGE(IDC_COORD_HEIGHT_EDIT, OnCoordChange)
	ON_BN_CLICKED(IDC_TEXTURE_ADD_BUTTON, OnTextureAddButtonClicked)
	ON_BN_CLICKED(IDC_TEXTURE_LOC_ADD_BUTTON, OnTextureLocAddButtonClicked)
	ON_BN_CLICKED(IDC_TEXTURE_REMOVE_BUTTON, OnTextureRemoveButtonClicked)
	ON_WM_KEYDOWN()
	ON_BN_CLICKED(IDC_PREV_FRAME, OnPrevFrameButton)
	ON_BN_CLICKED(IDC_NEXT_FRAME, OnNextFrameButton)
	ON_BN_CLICKED(IDC_PLAY_CHECK, OnPlayCheck)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CUISpriteEditorDlg message handlers

BOOL CUISpriteEditorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	UpdateTexturesCombo ();

	// layout
	layout_->init(this);

	layout_->add(1, 1, 1, 1, IDC_VIEW);

	layout_->add(0, 1, 1, 0, IDC_TEXTURE_ADD_BUTTON);
	layout_->add(0, 1, 1, 0, IDC_TEXTURE_LOC_ADD_BUTTON);
	layout_->add(0, 1, 1, 0, IDC_TEXTURE_REMOVE_BUTTON);
	layout_->add(0, 1, 1, 0, IDC_TEXTURES_COMBO);
	layout_->add(0, 1, 1, 0, IDC_TEXTURE_LABEL);
	layout_->add(0, 1, 1, 0, IDC_TEXTURE_SIZE_LABEL);
	//layout_->add(1, 1, 1, 0, IDC_UPPER_LINE);
                            
                            
	layout_->add(0, 1, 1, 0, IDC_H_LINE1);

	layout_->add(0, 1, 1, 0, IDC_COORD_LABEL);
	layout_->add(0, 1, 1, 0, IDC_COORD_COMMA_LABEL);
	layout_->add(0, 1, 1, 0, IDC_COORD_X_LABEL);
	layout_->add(0, 1, 1, 0, IDC_COORD_LEFT_EDIT);
	layout_->add(0, 1, 1, 0, IDC_COORD_TOP_EDIT);
	layout_->add(0, 1, 1, 0, IDC_COORD_WIDTH_EDIT);
	layout_->add(0, 1, 1, 0, IDC_COORD_HEIGHT_EDIT);

	layout_->add(0, 1, 1, 0, IDC_H_LINE2);

	layout_->add(0, 1, 1, 0, IDC_SATURATION_LABEL);
	layout_->add(0, 1, 1, 0, IDC_SATURATION_SLIDER);
	layout_->add(0, 1, 1, 0, IDC_SATURATION_EDIT);


	layout_->add(0, 1, 1, 0, IDC_H_LINE3);

	layout_->add(0, 1, 1, 0, IDC_DIFFUSE_COLOR_LABEL);
	layout_->add(0, 1, 1, 0, IDC_DIFFUSE_COLOR);

                            
	layout_->add(0, 0, 1, 1, IDC_PREV_FRAME);
	layout_->add(0, 0, 1, 1, IDC_PLAY_CHECK);
	layout_->add(0, 0, 1, 1, IDC_NEXT_FRAME);

	layout_->add(0, 1, 0, 1, IDC_V_LINE);
	layout_->add(0, 0, 1, 1, IDC_BACKGROUND_COLOR);
	layout_->add(0, 0, 1, 1, IDOK);
	layout_->add(0, 0, 1, 1, IDCANCEL);

	CButton* button = static_cast<CButton*>(GetDlgItem(IDC_PLAY_CHECK));
	button->SetCheck(TRUE);

	saturationSlider_.SetRange(0, 10);
	saturationSlider_.SetPageSize(1);
	saturationSlider_.SetPos(round(sprite_.saturation() * float(SATURATION_SLIDER_MAX_VALUE)));

	CRect editorRect;
	editor_->GetClientRect(&editorRect);
	editor_->initRenderDevice(editorRect.Width(), editorRect.Height());

	diffuseColorSelector_->signalColorChanged() = bindMethod(*this, &CUISpriteEditorDlg::onDiffuseColorChanged);
	backgroundColorSelector_->signalColorChanged() = bindMethod(*this, &CUISpriteEditorDlg::onBackgroundColorChanged);
	backgroundColorSelector_->setColor(sColor4f(editor_->backgroundColor()));

	UpdateControls();
	ShowWindow(SW_MAXIMIZE);
	return TRUE;
}

void CUISpriteEditorDlg::OnSize(UINT nType, int cx, int cy)
{
	layout_->onSize (cx, cy);
	CDialog::OnSize(nType, cx, cy);
	Invalidate ();
}

void CUISpriteEditorDlg::UpdateControls ()
{
	xassert (this);
	Rectf rect = editor_->getCoords ();
    CString str;

    str.Format ("%i", round (rect.left ()));
    m_ctlCoordLeftEdit.SetWindowText (str);
    str.Format ("%i", round (rect.top ()));
    m_ctlCoordTopEdit.SetWindowText (str);
    str.Format ("%i", round (rect.width ()));
    m_ctlCoordWidthEdit.SetWindowText (str);
    str.Format ("%i", round (rect.height ()));
    m_ctlCoordHeightEdit.SetWindowText (str);

	UI_Sprite& sprite = editor_->sprite_;
	if(!sprite.isEmpty() && sprite.texture()){
		str.Format("%ix%i", sprite.texture()->GetWidth(),
							sprite.texture()->GetHeight());
	}
	else {
		str = " --- ";
	}

	if(sprite.isAnimated()){
		GetDlgItem(IDC_PREV_FRAME)->ShowWindow(SW_SHOWNOACTIVATE);
		GetDlgItem(IDC_PLAY_CHECK)->ShowWindow(SW_SHOWNOACTIVATE);
		GetDlgItem(IDC_NEXT_FRAME)->ShowWindow(SW_SHOWNOACTIVATE);
	}else{
		GetDlgItem(IDC_PREV_FRAME)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_PLAY_CHECK)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_NEXT_FRAME)->ShowWindow(SW_HIDE);
	}

	int pos = round(float(SATURATION_SLIDER_MAX_VALUE) * sprite.saturation());
	saturationSlider_.SetPos(pos);
	diffuseColorSelector_->setColor(sColor4f(sprite.diffuseColor()));

	m_ctlTextureSizeLabel.SetWindowText (str);
}

void CUISpriteEditorDlg::OnTexturesComboSelChange()
{
	int index = m_ctlTexturesCombo.GetCurSel ();
	if (index == 0) { // нет текстуры
		editor_->setTexture(0);
	} else {
		CString str;
		m_ctlTexturesCombo.GetWindowText(str);
		editor_->setTexture(str);
		sprite_ = editor_->getSprite();
	}
	UpdateControls ();
}

void CUISpriteEditorDlg::OnCoordChange()
{
	CString strLeft, strTop, strWidth, strHeight;
	m_ctlCoordLeftEdit.GetWindowText (strLeft);
	m_ctlCoordTopEdit.GetWindowText (strTop);
	m_ctlCoordWidthEdit.GetWindowText (strWidth);
	m_ctlCoordHeightEdit.GetWindowText (strHeight);
    editor_->setCoords (Rectf (atoi (strLeft), atoi (strTop), atoi (strWidth), atoi (strHeight)));
}

std::string selectAndCopyResource(const char* internalResourcePath, const char* filter, const char* defaultName, const char* title);

void CUISpriteEditorDlg::OnTextureLocAddButtonClicked()
{
	string locPath = getLocDataPath("UI_Textures");
	std::string filename = selectAndCopyResource(locPath.c_str(), "*.tga;*.dds;*.avi", "", "Selecte loc texture...");

	if(!filename.empty()){

		std::string::size_type pos = filename.rfind ("\\") + 1;
		std::string fileTitle = std::string(filename.begin() + pos, filename.end());

		UI_Texture* texture = new UI_Texture(filename.c_str ());
		texture->addRef ();
		UI_TextureLibrary::instance().add(UI_TextureLibrary::StringType(fileTitle.c_str(), texture));
		editor_->setTexture(fileTitle.c_str());
		UI_Dispatcher::instance().init ();
	}
	UpdateTexturesCombo ();
	if (m_ctlTexturesCombo.GetCount ()) {
		m_ctlTexturesCombo.SetCurSel (m_ctlTexturesCombo.GetCount () - 1);
	}
}

void CUISpriteEditorDlg::OnTextureAddButtonClicked()
{
	//CFileDialog fileDlg (TRUE, 0, 0, OFN_NOCHANGEDIR, "TGA images|*.tga|DDS images|*.dds|AVI files|*.avi||");
	
	std::string filename = selectAndCopyResource(".\\Resource\\UI\\Textures", "*.tga;*.dds;*.avi", "", "Selecte texture...");

	if(!filename.empty()){

		std::string::size_type pos = filename.rfind ("\\") + 1;
		std::string fileTitle = std::string(filename.begin() + pos, filename.end());
		
		/*
		char full_path[MAX_PATH];
		char cw_path[MAX_PATH];
		_fullpath (full_path, fileDlg.GetPathName (), sizeof(full_path) - 1);
		_fullpath (cw_path, ".", sizeof(cw_path) - 1);
		std::size_t cw_len = strlen (cw_path);
		if (strnicmp (full_path, cw_path, cw_len) == 0) {
			std::string relative_path = std::string(".") + std::string(full_path + cw_len, full_path + strlen (full_path));
				filename = relative_path.c_str();
		} else {
			int result = AfxMessageBox ("Selected file lies outside project directory.\nIt will be stored as absolute path...\nProceed?", MB_OK | MB_YESNO, 0);
			if (result == IDYES) {
				filename = fileDlg.GetPathName ();
			} else {
				return;
			}
		}
*/
		UI_Texture* texture = new UI_Texture(filename.c_str ());
		texture->addRef ();
		UI_TextureLibrary::instance().add(UI_TextureLibrary::StringType(fileTitle.c_str(), texture));
		editor_->setTexture(fileTitle.c_str());
		UI_Dispatcher::instance().init ();
	}
	UpdateTexturesCombo ();
	if (m_ctlTexturesCombo.GetCount ()) {
		m_ctlTexturesCombo.SetCurSel (m_ctlTexturesCombo.GetCount () - 1);
	}
}

void CUISpriteEditorDlg::OnTextureRemoveButtonClicked()
{
	int combo_sel = m_ctlTexturesCombo.GetCurSel();
	if (combo_sel > 0) {
		CString text;
		m_ctlTexturesCombo.GetLBText (combo_sel, text);
		UI_TextureLibrary::instance().remove (static_cast<const char*>(text));
		UI_Dispatcher::instance().init ();
		editor_->setTexture (0);
		UpdateTexturesCombo ();
	}
}

void CUISpriteEditorDlg::UpdateTexturesCombo ()
{
	m_ctlTexturesCombo.ResetContent ();
	const char* comboList = UI_TextureLibrary::instance().comboList ();
	int cur_sel = 0;
	int index = 0;

	m_ctlTexturesCombo.InsertString (-1, TRANSLATE("[ Нет текстуры ]"));

	for (;;) {
		++index;
		std::string token = getComboToken (comboList);
		if (token.empty ())
			break;

		if (!sprite_.isEmpty ()) {
			std::string name = sprite_.textureReference ().c_str();
			if (token == name)
				cur_sel = index;
		}

		m_ctlTexturesCombo.InsertString (-1, token.c_str ());
	}
	
	m_ctlTexturesCombo.SetCurSel (cur_sel);
}
void CUISpriteEditorDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CUISpriteEditorDlg::OnOK()
{
	sprite_ = editor_->getSprite ();
	CDialog::OnOK();
}

void CUISpriteEditorDlg::OnPrevFrameButton()
{
	editor_->prevFrame();
}

void CUISpriteEditorDlg::OnNextFrameButton()
{
    editor_->nextFrame();	
}

void CUISpriteEditorDlg::OnPlayCheck()
{
	CButton* checkBox  = static_cast<CButton*>(GetDlgItem(IDC_PLAY_CHECK));
	bool enabled(checkBox->GetCheck());
	editor_->enableAnimation(enabled);
	UpdateControls();
}

void CUISpriteEditorDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int pos = saturationSlider_.GetPos();
	float saturation = float(pos) / float(SATURATION_SLIDER_MAX_VALUE);
	sprite_ = editor_->getSprite();
    sprite_.setSaturation(saturation);
	editor_->setSprite(sprite_);

	CString str;
	str.Format("%g", saturation);
	GetDlgItem(IDC_SATURATION_EDIT)->SetWindowText(str);

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CUISpriteEditorDlg::onDiffuseColorChanged()
{
	sprite_ = editor_->getSprite();
	sprite_.setDiffuseColor(sColor4c(diffuseColorSelector_->getColor()));	
	editor_->setSprite(sprite_);
}

void CUISpriteEditorDlg::onBackgroundColorChanged()
{
	editor_->setBackgroundColor(backgroundColorSelector_->getColor());
}
