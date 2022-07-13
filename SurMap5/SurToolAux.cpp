#include "stdafx.h"

#include "MainFrame.h"
#include "ToolsTreeWindow.h"
#include "ToolsTreeCtrl.h"
#include "MiniMapWindow.h"
#include "Serialization\Dictionary.h"
#include "SurMapOptions.h"
#include "SurToolAux.h"
#include "ConsoleWindow.h"
#include "Serialization\XPrmArchive.h"
#include "Serialization\MultiArchive.h"
#include "Environment\Environment.h"
#include "Game\CameraManager.h"
#include "Game\GameOptions.h"
#include "FileUtils\FileUtils.h"
#include "Water\CircleManager.h"
#include "Game\Universe.h"
#include "Serialization\SerializationFactory.h"

BEGIN_ENUM_DESCRIPTOR(ePopUpMenuRestriction, "ePopUpMenuRestriction")
REGISTER_ENUM(PUMR_PermissionAll, "PUMR_PermissionAll");
REGISTER_ENUM(PUMR_Permission3DM, "PUMR_Permission3DM");
REGISTER_ENUM(PUMR_Permission3DM_2W, "PUMR_Permission3DM_2W");
REGISTER_ENUM(PUMR_PermissionColorPic, "PUMR_PermissionColorPic");
REGISTER_ENUM(PUMR_PermissionZone, "PUMR_PermissionZone");
REGISTER_ENUM(PUMR_PermissionEffect, "PUMR_PermissionEffect");
REGISTER_ENUM(PUMR_PermissionToolzerBlurGeo, "PUMR_PermissionToolzerBlurGeo");
REGISTER_ENUM(PUMR_PermissionDelete, "PUMR_PermissionDelete");
REGISTER_ENUM(PUMR_NotPermission, "PUMR_NotPermission");
END_ENUM_DESCRIPTOR(ePopUpMenuRestriction)


IMPLEMENT_DYNAMIC(CSurToolBase, CExtResizableDialog)

bool flag_TREE_BAR_EXTENED_MODE=0;

CSurToolBase::CSurToolBase(int IDD, CWnd* pParent)
: CExtResizableDialog(IDD, pParent)
, treeItem_(0)
{
	cursorPosition_ = Vect3f::ZERO;
	flag_init_dialog=0;
	flag_repeatOperationEnable=true;
	popUpMenuRestriction=PUMR_NotPermission;//PUMR_PermissionDelete;
	iconInSurToolTree=IconISTT_Tool;
	subfolderExpandPermission=SEP_NotPermission;
	flag_select=false;
}

void CSurToolBase::serialize(Archive& ar) 
{
	ar.serialize(name_, "NodeName", 0);
	ar.serialize(popUpMenuRestriction, "popUpMenuRestriction", 0);
	ar.serialize(flag_select, "flag_select", 0);
	if(!ar.isEdit())
		ar.serialize(children_, "treeSDTB", 0);
}

void CSurToolBase::FillIn ()
{
    for (size_t i = 0; i < children_.size (); ++i) {
		CSurToolBase* toolzer = children_[i];
		if(toolzer)
			children_[i]->FillIn();
		else{
			children_.erase(children_.begin() + i);
			--i;
		}
    }
    return;
}

void CSurToolBase::deleteAllChilds()
{
    while(!children_.empty())
		deleteChildNode(children_.front());
}

string requestResourceAndPut2InternalResource(const char* resourceHomePath, const char* filter, const char* defaultName, const char* title)
{
	string result;
	string filterstr= string("(") + filter + ")|" + filter + "||";
	CFileDialog fileDlg(TRUE, filter, defaultName, OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR, filterstr.c_str());
	fileDlg.m_ofn.lpstrTitle= title;
	std::string initial_dir = surMapOptions.getLastDirResource(resourceHomePath);
	fileDlg.m_ofn.lpstrInitialDir = initial_dir.c_str ();//NULL;// (curDir???)
	if((fileDlg.DoModal()==IDOK) && testExistingFile(fileDlg.GetPathName()) ){
		string tmp=fileDlg.GetPathName();
		string path2file;
		string fileName;
		string::size_type i=tmp.find_last_of("\\");
		if(i!=string::npos){
			path2file=tmp.substr(0, i);
			fileName=&(tmp.c_str()[i+1]);
	        surMapOptions.setLastDirResource(resourceHomePath, path2file.c_str ());
		}
		else{
			path2file="";
			fileName=tmp;
		}
		char fileFullPath[_MAX_PATH];
		char homeFullPath[_MAX_PATH];

		if(_fullpath(fileFullPath, path2file.c_str(), _MAX_PATH) == 0)
			return result; // ""
		if(_fullpath(homeFullPath, resourceHomePath, _MAX_PATH) == 0)
			return result; // ""

		result = resourceHomePath;
		if(!result.empty())
            result += "\\";
		result+=fileName;
		
		int filePathLen = strlen(fileFullPath);
		int homePathLen = strlen(homeFullPath);

		if(homePathLen <= filePathLen && strnicmp(fileFullPath, homeFullPath, homePathLen) == 0){
            result = resourceHomePath;
            result += std::string(fileFullPath + homePathLen, fileFullPath + filePathLen);
            result += "\\";
            result += fileName;
            return result; //ok(файл уже во внутренних ресурсах)
		}
		//копирование ресурса
		if(!CopyFile(fileDlg.GetPathName(), result.c_str(), FALSE)){
			result.clear();//err ошибка копировани€ во внутренние ресурсы
		}
	}
	return result;
}


string requestModelAndPut2InternalResource(const char* resourceHomePath, const char* filter, const char* defaultName, const char* title)
{
	string result;
	string filterstr= string("(") + filter + ")|" + filter + "||";
	CFileDialog fileDlg(TRUE, filter, defaultName, OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR, filterstr.c_str());
	fileDlg.m_ofn.lpstrTitle = title;
	fileDlg.m_ofn.lpstrInitialDir = resourceHomePath;
	if((fileDlg.DoModal()==IDOK) && testExistingFile(fileDlg.GetPathName()) ){
		string tmp=fileDlg.GetPathName();
		string path2file;
		string fileName;

		string::size_type i = tmp.find_last_of("\\");
		if(i != string::npos){
			path2file = tmp.substr(0, i);
			fileName = &(tmp.c_str()[i+1]);
		}
		else{
			path2file = "";
			fileName = tmp;
		}
		char fileFullPath[_MAX_PATH];
		char homeFullPath[_MAX_PATH];

		if(_fullpath(fileFullPath, path2file.c_str(), _MAX_PATH) == 0)
			return result;//error //"";
		if(_fullpath(homeFullPath, resourceHomePath, _MAX_PATH) == 0)
			return result;//error //"";

		result = resourceHomePath;
		if(!result.empty())
            result += "\\";
		result+=fileName;
		
		int filePathLen = strlen(fileFullPath);
		int homePathLen = strlen(homeFullPath);

		if(homePathLen <= filePathLen && strnicmp(fileFullPath, homeFullPath, homePathLen) == 0){
            result = resourceHomePath;
            result += std::string(fileFullPath + homePathLen, fileFullPath + filePathLen);
            result += "\\";
            result += fileName;
            return result; //ok(файл уже во внутренних ресурсах)
		}
		//копирование моделей
		if(!CopyFile(fileDlg.GetPathName(), result.c_str(), FALSE)){
			result.clear();//err ошибка копировани€ во внутренние ресурсы
			CString errorMessage;
			errorMessage = TRANSLATE("ќшибка копировани€ во внутренние ресурсы");
			AfxMessageBox (errorMessage, MB_OK | MB_ICONERROR);
		}
		
		// копируем .furinfo, если такой имеетс€
		{
		std::string fileName = extractFileBase(fileDlg.GetPathName());
		std::string filePath = extractFilePath(fileDlg.GetPathName());
		std::string sourceFilename = filePath + fileName + ".furinfo";
		std::string destinationFilename = resourceHomePath;
		destinationFilename+= "\\";
		destinationFilename += fileName + ".furinfo";
		CopyFile(sourceFilename.c_str(), destinationFilename.c_str(), FALSE);
		}

		//извлечение названий текстур из модели и копирование
		vector<string> textureNames;
		GetAllTextureNames(fileDlg.GetPathName(), textureNames);
		vector<string>::iterator p;
		for(p=textureNames.begin(); p!=textureNames.end(); p++){
			tmp=resourceHomePath;
			if(!tmp.empty()) tmp+="\\";
			tmp+="Textures\\";

			string::size_type i=p->find_last_of("\\");
			if(i!=string::npos)
				tmp+=&(p->c_str()[i+1]);
			else
				tmp+=*p;

			if(!CopyFile(p->c_str(), tmp.c_str(), FALSE)){
				result.clear();//err ошибка копировани€ во внутренние ресурсы
				CString errorMessage;
				errorMessage.Format (TRANSLATE("Ќе могу скопировать текстуру\nиз: %s\nв: %s"),
									 p->c_str(), tmp.c_str());
				AfxMessageBox (errorMessage, MB_OK | MB_ICONERROR);
				break;
			}
		}
	}
	return result;
}

bool CSurToolBase::CreateExt(CWnd* parentWnd)
{
	return Create(getIDD(), parentWnd); //ѕодразумеваетс€ CDialog но может быть и переопределенна€ в конкретном классе
}

void CSurToolBase::drawCursorCircle()
{
	int radius = getBrushRadius();
	universe()->circleManager()->addCircle(cursorPosition_, radius, CircleManagerParam(Color4c(0, 0, 255, 255)));
}

CObjectsManagerWindow& CSurToolBase::objectsManager()
{
	CMainFrame* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());
	return mainFrame->objectsManager();
}

int CSurToolBase::getBrushRadius() const
{
	CMainFrame* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());
	return float(mainFrame->getToolWindow().getBrushRadius());
}

cScene* CSurToolBase::getPreviewScene ()
{
	return ((CMainFrame*)AfxGetMainWnd())->miniMapWindow().getScene ();
}


void CSurToolBase::updateTreeNode ()
{
	if(treeItem())
		if(CMainFrame* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd()))
			mainFrame->getToolWindow().getTree().GetTreeCtrl().SetItemText(treeItem(), name());
}

void CSurToolBase::TrackMouse (const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	cursorPosition_ = worldCoord;
	cursorScreenPosition_ = screenCoord;
	onTrackingMouse(worldCoord, screenCoord);
}

BOOL CSurToolBase::DestroyWindow()
{
	flag_init_dialog=0;
	return CExtResizableDialog::DestroyWindow();
}


void CSurToolBase::deleteChildNode(CSurToolBase* node)
{
	CSurToolBase* curNode = this;
	vector< ShareHandle<CSurToolBase> >::iterator i;
	for(i = curNode->children().begin(); i != curNode->children().end();){
		(*i)->deleteChildNode(node);
		if(*i == node)
			i = curNode->children().erase(i);
		else
            ++i;
	}
}


float CSurToolBase::miniMapZoom()
{
	return ((CMainFrame*)AfxGetMainWnd())->miniMapWindow().zoom();
}

void CSurToolBase::setMiniMapZoom(float zoom)
{
	((CMainFrame*)AfxGetMainWnd())->miniMapWindow().setZoom(zoom);
}

void CSurToolBase::enableTransformTools(bool enable)
{
    CToolsTreeWindow& toolWindow = ((CMainFrame*)AfxGetMainWnd())->getToolWindow();
	toolWindow.enableTransformTools(enable);
}

void CSurToolBase::popEditorMode()
{
    CToolsTreeWindow& toolWindow = ((CMainFrame*)AfxGetMainWnd())->getToolWindow();
	toolWindow.popEditorMode();
}

void CSurToolBase::pushEditorMode(CSurToolBase* editorMode)
{
    CToolsTreeWindow& toolWindow = ((CMainFrame*)AfxGetMainWnd())->getToolWindow();
	toolWindow.pushEditorMode(editorMode);
}

void CSurToolBase::replaceEditorModeSelect()
{
    CToolsTreeWindow& toolWindow = ((CMainFrame*)AfxGetMainWnd())->getToolWindow();
	toolWindow.replaceEditorMode(toolWindow.tools()[CToolsTreeWindow::TOOL_SELECT]);
}

void CSurToolBase::drawPreviewTexture(cTexture* texture, int width, int height, const sRectangle4f& rect)
{
	if(texture){
		int textureWidth = texture->GetWidth();
		int textureHeight = texture->GetHeight();

		if(!textureWidth || !textureHeight)
			return;
		
		int partWidth = round(textureWidth * (rect.xmax() - rect.xmin()));
		int partHeight = round(textureHeight * (rect.ymax() - rect.ymin()));

		if(partWidth == 0)
			partWidth = textureWidth;
		if(partHeight == 0)
			partWidth = textureHeight;

		Vect2f size(partWidth, partHeight);
		if (size.x > float(width)) {
			size.y *= float(width) / size.x;
			size.x = float(width);
		}
		if (size.y > float(height)) {
			size.x *= float(height) / size.y;
			size.y = float(height);
		}

		gb_RenderDevice->DrawSprite((width - size.xi()) / 2, (height - size.yi()) / 2,
									size.xi(), size.yi(),
									rect.xmin(), rect.ymin(), rect.xmax()-rect.xmin(), rect.ymax()-rect.ymin(), texture);
	}
}
