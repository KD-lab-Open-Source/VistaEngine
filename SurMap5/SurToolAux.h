#ifndef __SURTOOLAUX_H__
#define __SURTOOLAUX_H__

#include "Handle.h"
#include "Dictionary.h"
#include "..\terra\terra.h"

class Archive;
class cTexture;

extern bool flag_TREE_BAR_EXTENED_MODE;
enum ePopUpMenuRestriction {
	PUMR_PermissionAll,
	PUMR_Permission3DM,
	PUMR_Permission3DM_2W,
	PUMR_PermissionZone,
	PUMR_PermissionEffect,
	PUMR_PermissionColorPic,
	PUMR_PermissionToolzerBlurGeo,
	PUMR_PermissionDelete,
	PUMR_NotPermission
};
enum eIconInSurToolTree {
	IconISTT_FolderTools,
	IconISTT_Tool,
	IconISTT_Other
};
enum eSubfolderExpandPermission{
	SEP_Permission,
	SEP_NotPermission
};

struct PoseRadius : Se3f {
	PoseRadius(const Se3f& _pose = Se3f::ID, float _radius = 100.0f)
	: Se3f(_pose), radius(_radius) {}
	float radius;
};

enum e_BrushForm {
	BRUSHFORM_CIRCLE = 0,
	BRUSHFORM_SQUARE
};

class CObjectsManagerWindow;
class cScene;
class CSurToolBase;

typedef vector< ShareHandle<CSurToolBase> > SurTools;

class CSurToolBase : public CExtResizableDialog, public ShareHandleBase {
	DECLARE_DYNAMIC(CSurToolBase)
public:
	string dataFileName;

	ePopUpMenuRestriction popUpMenuRestriction;
	eIconInSurToolTree iconInSurToolTree;
	eSubfolderExpandPermission subfolderExpandPermission;
	bool flag_repeatOperationEnable;
	bool flag_init_dialog;

	CSurToolBase(int IDD, CWnd* pParent);
	bool CreateExt(CWnd* pParentWnd = 0);

	ePopUpMenuRestriction getPopUpMenuRestriction() const{ return popUpMenuRestriction; }

	void drawCursorCircle();
	void drawPreviewTexture(cTexture* texture, int width, int height, const sRectangle4f& rect = sRectangle4f(0.0f, 0.0f, 0.0f, 0.0f));
	
	static CObjectsManagerWindow& objectsManager();
	int getBrushRadius() const;

	cScene* getPreviewScene();

	Vect2i cursorScreenPosition() const{ return cursorScreenPosition_; }
	Vect3f cursorPosition() const{ return cursorPosition_; }
	void setCursorPosition(const Vect3f& position){ cursorPosition_ = position; }

	float miniMapZoom();
	void setMiniMapZoom(float zoom);

	void enableTransformTools(bool enable);

	virtual int getIDD() const =0;
	virtual bool isLabelEditable () const { return false; }
	virtual bool isLabelTranslatable() const { return true; }

	virtual void serialize(Archive& ar);

	virtual void quant() {}
	virtual void CallBack_BrushRadiusChanged(){}
	virtual void CallBack_ReleaseScene(){}
	virtual void CallBack_CreateScene(){}
	virtual bool CallBack_OperationOnMap(int x, int y){ return false; }
	virtual bool CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord) { return false;}

	virtual bool CallBack_LMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord) { return false; }
	virtual bool CallBack_LMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord) { return false; }

	virtual bool CallBack_RMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord) { return false; }
	virtual bool CallBack_RMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord) { return false; }

	virtual bool CallBack_KeyDown(unsigned int keyCode, bool shift, bool control, bool alt) { return false; }

	virtual void CallBack_SelectionChanged() {}
	virtual bool CallBack_Delete() { return false; }
	virtual bool CallBack_DrawAuxData() { return false; }

	virtual bool CallBack_DrawPreview(int width, int height){ return false; }
	virtual bool CallBack_PreviewLMBDown(const Vect2f& point){ return false; }
	virtual bool CallBack_PreviewLMBUp(const Vect2f& point){ return false; }
	virtual bool CallBack_PreviewTrackingMouse(const Vect2f& point){ return false; }

	virtual void FillIn();

	void TrackMouse(const Vect3f& worldCoord, const Vect2i& screenCoord);

	void deleteAllChilds();
	void deleteChildNode(CSurToolBase* node);
	void updateTreeNode();


	virtual BOOL DestroyWindow();

	SurTools& children() { return children_; }

	const char* name() const{ return name_.c_str(); }
	void setName(const char* name) { name_ = name; }
	HTREEITEM treeItem() const{ return treeItem_; }
	void setTreeItem(HTREEITEM item){ treeItem_ = item; }
protected:
	void popEditorMode();
	void pushEditorMode(CSurToolBase* editorMode);
	HTREEITEM treeItem_;

	SurTools children_;
	std::string name_;
private:
	Vect2i cursorScreenPosition_;
	Vect3f cursorPosition_;
};

struct CIntVarWin{
	int value;
	int MAX,MIN;
	int idWin;
	CDialog* dlg;
	CIntVarWin(void){};
	CIntVarWin(int _min, int _max, int _idWin, CDialog* _dlg){
		MIN=_min; MAX=_max; idWin=_idWin; dlg=_dlg;
	}
	void init (int _min, int _max, int _idWin, CDialog* _dlg){
		MIN=_min; MAX=_max; idWin=_idWin; dlg=_dlg;
	};
	int MinMax(void){
		unsigned char fl_correction=0;
		if(value > MAX){ value=MAX; fl_correction=1; }
		else if(value < MIN){ value=MIN; fl_correction=1; }
		return fl_correction;
	};
	int get(void){
		CEdit * editV;
		editV=(CEdit *)dlg->GetDlgItem(idWin);
		const int tb_str_lenght=20;
		char txtBuf[tb_str_lenght];
		int cntChar=editV->GetLine(0,txtBuf,tb_str_lenght-1);
		unsigned char fl_correction=0;
		if(cntChar!=0){ //Если что-то введено
			txtBuf[cntChar]='\x0';
			value=atoi(txtBuf);
			if(value > MAX){ value=MAX; fl_correction=1; }
			else if(value < MIN){ value=MIN; fl_correction=1; }
		}
		else{ //Если ничего не введено
				value=MIN; //fl_correction=1;
		}

		return fl_correction;
	};

	int put(void){
		unsigned char fl_correction=0;
		if(value > MAX){ value=MAX; fl_correction=1; }
		else if(value < MIN){ value=MIN; fl_correction=1; }

		CEdit * editV;
		editV=(CEdit *)dlg->GetDlgItem(idWin);
		const int tb_str_lenght=20;
		char txtBuf[tb_str_lenght];
		itoa(value, txtBuf, 10);
		editV->SetWindowText(txtBuf);

		return fl_correction;
	};
};

struct CVoxVarWin{
	int value;
	int MAX,MIN;
	int idWin;
	CDialog* dlg;
	CVoxVarWin(void){};
	CVoxVarWin(int _min, int _max, int _idWin, CDialog* _dlg){
		MIN=_min; MAX=_max; idWin=_idWin; dlg=_dlg;
	};
	void init (int _min, int _max, int _idWin, CDialog* _dlg){
		MIN=_min; MAX=_max; idWin=_idWin; dlg=_dlg;
	};
	int MinMax(void){
		unsigned char fl_correction=0;
		if(value > MAX){ value=MAX; fl_correction=1; }
		else if(value < MIN){ value=MIN; fl_correction=1; }
		return fl_correction;
	};
	int get(void){

		CEdit * editV;
		editV=(CEdit *)dlg->GetDlgItem(idWin);
		const int tb_str_lenght=20;
		char txtBuf[tb_str_lenght];
		int cntChar=editV->GetLine(0,txtBuf,tb_str_lenght-1);
		unsigned char fl_correction=0;
		if(cntChar!=0){ //Если что-то введено
			txtBuf[cntChar]='\x0';
			value=convert_vid2vox(txtBuf);
			if(value > MAX){ value=MAX; fl_correction=1; }
			else if(value < MIN){ value=MIN; fl_correction=1; }
		}
		else{ //Если ничего не введеноoi
				value=MIN; //fl_correction=1;
		}

		return fl_correction;
	};

	int put(void){
		unsigned char fl_correction=0;
		if(value > MAX){ value=MAX; fl_correction=1; }
		else if(value < MIN){ value=MIN; fl_correction=1; }

		CEdit * editV;
		editV=(CEdit *)dlg->GetDlgItem(idWin);
		const int tb_str_lenght=20;
		char txtBuf[tb_str_lenght];
		convert_vox2vid(value, txtBuf);
		editV->SetWindowText(txtBuf);

		return fl_correction;
	};
};

struct CFloatVarWin{
	float value;
	float MAX,MIN;
	int idWin;
	CDialog* dlg;
	CFloatVarWin(void){};
	CFloatVarWin(float _min, float _max, int _idWin, CDialog* _dlg){
		MIN=_min; MAX=_max; idWin=_idWin; dlg=_dlg;
	};
	void init (float _min, float _max, int _idWin, CDialog* _dlg){
		MIN=_min; MAX=_max; idWin=_idWin; dlg=_dlg;
	};
	int MinMax(void){
		unsigned char fl_correction=0;
		if(value > MAX){ value=MAX; fl_correction=1; }
		else if(value < MIN){ value=MIN; fl_correction=1; }
		return fl_correction;
	};
	int get(void){
		CEdit * editV;
		editV=(CEdit *)dlg->GetDlgItem(idWin);
		const int tb_str_lenght=30;
		char txtBuf[tb_str_lenght];
		int cntChar=editV->GetLine(0,txtBuf,tb_str_lenght-1);
		unsigned char fl_correction=0;
		if(cntChar!=0){ //Если что-то введено
			txtBuf[cntChar]='\x0';
			XBuffer cnvrt;
			cnvrt < txtBuf;
			cnvrt.set(0,XB_BEG);
			cnvrt >=value;
			if(value > MAX){ value=MAX; fl_correction=1; }
			else if(value < MIN){ value=MIN; fl_correction=1; }
		}
		else{ //Если ничего не введено
				value=MIN; //fl_correction=1;
		}

		return fl_correction;
	};
	int put(void){
		unsigned char fl_correction=0;
		if(value > MAX){ value=MAX; fl_correction=1; }
		else if(value < MIN){ value=MIN; fl_correction=1; }

		CEdit * editV;
		editV=(CEdit *)dlg->GetDlgItem(idWin);
		//const int tb_str_lenght=30;
		//char txtBuf[tb_str_lenght];
		XBuffer cnvrt;
		cnvrt <= value;
		cnvrt.set(0,XB_BEG);
		editV->SetWindowText(cnvrt);

		return fl_correction;
	};
};

string requestResourceAndPut2InternalResource(const char* internalResourcePath, const char* filter, const char* defaultName, const char* title);
string requestModelAndPut2InternalResource(const char* internalResourcePath, const char* filter, const char* defaultName, const char* title);

#endif //__SURTOOLAUX_H__
