#include "stdafx.h"
#include "SurMap5.h"
#include ".\SurTool3DM.h"

#include ".\utl\shape3D.h"

#include "..\Render\inc\IVisGeneric.h"
#include "..\Render\inc\TerraInterface.inl"

#include "TreeInterface.h"

#include "..\Game\Universe.h"
#include "..\Game\RenderObjects.h"
#include "Serialization.h"
#include "EditArchive.h"
#include "XPrmArchive.h"
#include "MultiArchive.h"
#include "UnitEnvironment.h"
#include "ExternalShow.h"
#include "EventListeners.h"
#include "..\Terra\tools.h"

// CSurTool3DM dialog
const int MIN_SHAPE3D_CORNER=0;
const int MAX_SHAPE3D_CORNER=360;
const int MIN_SHAPE3D_SCALE=0;
const int MAX_SHAPE3D_SCALE=300;

//Aux function
static int convertEnvironmentType2Idx(EnvironmentType et)
{
	int result;
	switch(et){
	case ENVIRONMENT_PHANTOM:	result=0; break;
	case ENVIRONMENT_PHANTOM2: result=1; break;
	case ENVIRONMENT_BUSH:		result=2; break;
	case ENVIRONMENT_TREE:		result=3; break;
	case ENVIRONMENT_FENCE:		result=4; break;
	case ENVIRONMENT_FENCE2: result=5; break;
	case ENVIRONMENT_STONE:		result=6; break;
	case ENVIRONMENT_ROCK:		result=7; break;
	case ENVIRONMENT_BASEMENT:	result=8; break;
	case ENVIRONMENT_BARN:		result=9; break;
	case ENVIRONMENT_BUILDING:	result=10; break;
	case ENVIRONMENT_BRIDGE:	result=11; break;
	case ENVIRONMENT_INDESTRUCTIBLE: result=12; break;
	case ENVIRONMENT_BIG_BUILDING: result=13; break;
	default:
		xassert(0&&"Invalid enum EnvironmentType value");
		result=0; break;
	}
	return result;
}

static EnvironmentType convertIdx2EnvironmentType(int idx)
{
	EnvironmentType result=EnvironmentType(idx ? 1 << (idx - 1) : 0);
	xassert(result >= ENVIRONMENT_PHANTOM && result <= ENVIRONMENT_INDESTRUCTIBLE);
	return result;
}

/////////////////

IMPLEMENT_DYNAMIC(CSurTool3DM, CSurToolBase)
CSurTool3DM::CSurTool3DM(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
	, m_bSpread(false)
{	
	flag_repeatOperationEnable = false;
#ifdef _VISTA_ENGINE_EXTERNAL_
	popUpMenuRestriction = PUMR_NotPermission;
#else
	popUpMenuRestriction = PUMR_PermissionDelete;
#endif
	visualPreviewObject3DM = 0;
	angleSlider_.SetRange(MIN_SHAPE3D_CORNER, MAX_SHAPE3D_CORNER);
	angleSlider_.value = 0;
	angleDeltaSlider_.SetRange (0, 180);
	angleDeltaSlider_.value = 0;

	scaleSlider_.SetRange(MIN_SHAPE3D_SCALE, MAX_SHAPE3D_SCALE);
	scaleSlider_.value = 100;
	scaleDeltaSlider_.SetRange (0, 80);
	scaleDeltaSlider_.value = 0;

	spreadRadiusSlider_.SetRange (5, 200);
	spreadRadiusSlider_.value = 25;
	spreadRadiusDeltaSlider_.SetRange (1, 80);
	spreadRadiusDeltaSlider_.value = 10;

	m_bSpread = FALSE;
	m_bVertical = FALSE;

	m_idxCurAttribute = 0; //ENVIRONMENT_PHANTOM;

	m_cBoxPlaceMetod = 0;

	pose = Se3f::ID;
}

CSurTool3DM::~CSurTool3DM()
{
}


void CSurTool3DM::serialize(Archive& ar) 
{
	__super::serialize(ar);
#ifdef _VISTA_ENGINE_EXTERNAL_
	popUpMenuRestriction = PUMR_NotPermission;
#endif
	ar.serialize(dataFileName, "dataFileName", 0);
	ar.serialize(angleSlider_.value, "m_TurnZ", 0);
	ar.serialize(angleDeltaSlider_.value, "m_TurnZDelta", 0);
	ar.serialize(scaleSlider_.value, "m_ScalingXYZ", 0);
	ar.serialize(scaleDeltaSlider_.value, "m_ScalingXYZDelta", 0);
	ar.serialize(spreadRadiusSlider_.value, "spreadRadius", 0);
	ar.serialize(spreadRadiusDeltaSlider_.value, "spreadRadiusDelta", 0);

	EnvironmentType et(convertIdx2EnvironmentType(m_idxCurAttribute));
	//ar.serialize(EnvironmentType(m_idxCurAttribute), "environmentType", 0);
	ar.serialize(et, "environmentType", 0);
	m_idxCurAttribute=convertEnvironmentType2Idx(et);

	ar.serialize(m_bSpread, "spread", 0);
	ar.serialize(m_bVertical, "vertical", 0);
}

void CSurTool3DM::DoDataExchange(CDataExchange* pDX)
{
	DDX_Check (pDX, IDC_SPREAD_CHECK, m_bSpread);
	DDX_Check (pDX, IDC_VERTICAL_CHECK, m_bVertical);
    CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurTool3DM, CSurToolBase)
	ON_BN_CLICKED(IDC_BTN_BROWSE_FILE, OnBnClickedBtnBrowseModel)
	ON_WM_DESTROY()
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_CBOX_ATTRIBUTES, OnCbnSelchangeCboxAttributes)
	ON_CBN_SELCHANGE(IDC_COMBO_MODELSHAPE3D, OnCbnSelchangeComboModelshape3d)
	ON_BN_CLICKED(IDC_SPREAD_CHECK, OnSpreadCheckClicked)
	ON_BN_CLICKED(IDC_VERTICAL_CHECK, OnVerticalCheckClicked)
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_CBOX_PLACEMETOD, OnCbnSelchangeCboxPlacemetod)
END_MESSAGE_MAP()


// CSurTool3DM message handlers

const int CST3DM_STEP_TURNING=15;
const int CST3DM_STEP_ZOOMING=1;

struct CircleInRadius {
	CircleInRadius (float radius) : radius_ (radius) {}
	inline bool operator() (const ObjectSpreader::Circle& circle) const {
		if (circle.position.norm() + circle.radius < radius_)
			return true;
		else 
			return false;
	}
	float radius_;
};

void CSurTool3DM::hide2WControls ()
{
	/*
	GetDlgItem (IDC_RADIUS_EDIT)->ShowWindow (SW_HIDE);
	GetDlgItem (IDC_RADIUS_SLIDER)->ShowWindow (SW_HIDE);
	GetDlgItem (IDC_RADIUS_LESS_BUTTON)->ShowWindow (SW_HIDE);
	GetDlgItem (IDC_RADIUS_MORE_BUTTON)->ShowWindow (SW_HIDE);
	GetDlgItem (IDC_D_RADIUS_EDIT)->ShowWindow (SW_HIDE);
	GetDlgItem (IDC_D_RADIUS_SLIDER)->ShowWindow (SW_HIDE);
	GetDlgItem (IDC_D_RADIUS_LESS_BUTTON)->ShowWindow (SW_HIDE);
	GetDlgItem (IDC_D_RADIUS_MORE_BUTTON)->ShowWindow (SW_HIDE);
	*/
}

void CSurTool3DM::initControls ()
{
	CComboBox* cbAtr;
	cbAtr=(CComboBox*)GetDlgItem(IDC_CBOX_ATTRIBUTES);
	CString str;
	int nItem;
	for(nItem=0; nItem < ENVIRONMENT_TYPE_MAX; nItem++){
		str.Format(_T(getEnumNameAlt(EnvironmentType(nItem ? 1 << nItem - 1 : 0))), nItem);
		cbAtr->AddString(str);
	}
	cbAtr->SetCurSel(m_idxCurAttribute);
	xassert(m_idxCurAttribute >= 0 && m_idxCurAttribute < ENVIRONMENT_TYPE_MAX);

	angleSlider_.Create (this, IDC_ANGLE_SLIDER, IDC_ANGLE_EDIT, IDC_ANGLE_LESS_BUTTON, IDC_ANGLE_MORE_BUTTON, CST3DM_STEP_TURNING);
	angleDeltaSlider_.Create (this, IDC_D_ANGLE_SLIDER, IDC_D_ANGLE_EDIT, IDC_D_ANGLE_LESS_BUTTON, IDC_D_ANGLE_MORE_BUTTON, CST3DM_STEP_TURNING);
	scaleSlider_.Create (this, IDC_SCALE_SLIDER, IDC_SCALE_EDIT, IDC_SCALE_LESS_BUTTON, IDC_SCALE_MORE_BUTTON, CST3DM_STEP_ZOOMING);
	scaleDeltaSlider_.Create (this, IDC_D_SCALE_SLIDER, IDC_D_SCALE_EDIT, IDC_D_SCALE_LESS_BUTTON, IDC_D_SCALE_MORE_BUTTON, CST3DM_STEP_ZOOMING);
	spreadRadiusSlider_.Create (this, IDC_RADIUS_SLIDER, IDC_RADIUS_EDIT, IDC_RADIUS_LESS_BUTTON, IDC_RADIUS_MORE_BUTTON, 2);
	spreadRadiusDeltaSlider_.Create (this, IDC_D_RADIUS_SLIDER, IDC_D_RADIUS_EDIT, IDC_D_RADIUS_LESS_BUTTON, IDC_D_RADIUS_MORE_BUTTON, 5);

	//Установка комбо бокса PlaceMetod
	CComboBox * ComBox = (CComboBox *) GetDlgItem(IDC_CBOX_PLACEMETOD);
	int idx=0;
	cBoxPlaceMetodArr[idx++] = ComBox->AddString("absolutely"); //Создание элемента бокса и сохранение его индекса
	cBoxPlaceMetodArr[idx++] = ComBox->AddString("absolutely MAX"); //Создание элемента бокса и сохранение его индекса
	cBoxPlaceMetodArr[idx++] = ComBox->AddString("absolutely MIN"); //Создание элемента бокса и сохранение его индекса
	cBoxPlaceMetodArr[idx++] = ComBox->AddString("relatively"); //Создание элемента бокса и сохранение его индекса
	xassert(idx <= PLACEMETOD_MAX_METODS);
	ComBox->SetCurSel(m_cBoxPlaceMetod); //Установка текущего элемента бокса


	flag_init_dialog=true;
}

BOOL CSurTool3DM::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

    initControls ();
    hide2WControls ();

	CRect rt;
	GetWindowRect (&rt);

	m_layout.init(this);
	m_layout.add(1, 1, 1, 0, IDC_EDT_MODEL);
	m_layout.add(0, 1, 1, 0, IDC_BTN_BROWSE_FILE);
	m_layout.add(1, 1, 1, 0, IDC_CBOX_ATTRIBUTES);
	m_layout.add(1, 1, 1, 0, IDC_H_LINE);

	m_layout.add(1, 0, 1, 0, IDC_ANGLE_SLIDER);
	m_layout.add(1, 0, 1, 0, IDC_SCALE_SLIDER);
	m_layout.add(1, 0, 1, 0, IDC_RADIUS_SLIDER);
	m_layout.add(1, 0, 1, 0, IDC_D_ANGLE_SLIDER);
	m_layout.add(1, 0, 1, 0, IDC_D_SCALE_SLIDER);
	m_layout.add(1, 0, 1, 0, IDC_D_RADIUS_SLIDER);


	m_layout.add(1, 0, 1, 0, IDC_STATIC_LINE2);
	m_layout.add(1, 0, 1, 0, IDC_CBOX_PLACEMETOD);

#ifdef _VISTA_ENGINE_EXTERNAL_
	GetDlgItem(IDC_BTN_BROWSE_FILE)->EnableWindow(FALSE);
#endif


	ReloadM3D();
	return FALSE;
}

/*
bool CSurTool3DM::copyM3D2InternalResource(const char* _fullFileName, const char* _path2InternalResource)
{
	string tmp=_fullFileName;
	string path2file;
	string fileName;
	string::size_type i=tmp.find_last_of("\\");
	if(i!=string::npos){
		path2file=tmp.substr(0, i);
		fileName=&(tmp.c_str()[i+1]);
	}
	else{
		path2file="";
		fileName=tmp;
	}
	char buffer1[_MAX_PATH];
	char buffer2[_MAX_PATH];
	if(_fullpath(buffer1, path2file.c_str(), _MAX_PATH) == NULL) return false;
	if(_fullpath(buffer2, _path2InternalResource, _MAX_PATH) == NULL) return false;

	dataFileName = _path2InternalResource;
	if(!dataFileName.empty()) dataFileName+="\\";
	dataFileName+=fileName;
	if(strcmp(buffer1, buffer2)==0){
		return true;
	}
	//копирование моделей
	if(!CopyFile(_fullFileName, dataFileName.c_str(), FALSE)){
		dataFileName.clear();
		return false;
	}

	vector<string> textureNames;
	GetAllTextureNames(_fullFileName, textureNames);
	vector<string>::iterator p;
	for(p=textureNames.begin(); p!=textureNames.end(); p++){
		tmp=_path2InternalResource;
		if(!tmp.empty()) tmp+="\\";
		tmp+="Textures\\";

		string::size_type i=p->find_last_of("\\");
		if(i!=string::npos)
			tmp+=&(p->c_str()[i+1]);
		else
			tmp+=*p;

		if(!CopyFile(p->c_str(), tmp.c_str(), FALSE)){
			xassert(0&&"Error coping textures file");
			dataFileName.clear();
			return false;
		}
	}
	return true;
}*/

void CSurTool3DM::releaseModels ()
{
	VisualObjectsList::iterator it;
	FOR_EACH (visualObjects, it) {
		RELEASE (*it);
	}
	visualObjects.clear ();
	FOR_EACH (logicObjects, it) {
		RELEASE (*it);
	}
	logicObjects.clear ();
}

void CSurTool3DM::createModels (const char* fileName, int count)
{
	for (int i = 0; i < count; ++i) {
		visualObjects.push_back(terScene->CreateObject3dx(fileName));
		logicObjects.push_back(terScene->CreateLogic3dx(fileName));
	}
}

bool CSurTool3DM::ReloadM3D()
{
    if (m_bSpread) {
        float scale = float(max (5, spreadRadiusSlider_.value));
        float scaleDelta = float(spreadRadiusDeltaSlider_.value) / 100.0f;
        objectSpreader.setRadius (Rangef(scale * (1.0f - scaleDelta), scale * (1.0f + scaleDelta)));
        int radius = getBrushRadius();
        objectSpreader.fill (CircleInRadius (radius));
    }

	releaseModels ();
	RELEASE(visualPreviewObject3DM);
	if(!dataFileName.empty()){
		CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_MODEL);
		ew->SetWindowText(dataFileName.c_str());
		if (terScene) {
			if (m_bSpread) {
				createModels (dataFileName.c_str(), objectSpreader.circles().size());
			} else {
				createModels (dataFileName.c_str(), 1);
			}
		}
		if (getPreviewScene()) {
			if (visualPreviewObject3DM = getPreviewScene()->CreateObject3dx(dataFileName.c_str())) {
				sBox6f box;
				visualPreviewObject3DM->GetBoundBox (box);
				float scale = 80.0f / box.GetRadius ();
				visualPreviewObject3DM->SetScale(scale);
				visualPreviewObject3DM->SetPosition (MatXf::ID);
			}
		}

		UpdateShapeModel();
		return 1;
	}
	return 0;
}

void CSurTool3DM::CallBack_CreateScene(void)
{
	ReloadM3D();
}
void CSurTool3DM::CallBack_ReleaseScene(void)
{
	releaseModels ();
	RELEASE(visualPreviewObject3DM);
}

//IDC_CBOX_PLACEMETOD
bool CSurTool3DM::CallBack_OperationOnMap(int x, int y)
{
	wrkMesh.load(dataFileName.c_str());
	float kScale=scaleSlider_.value/100.f;
	wrkMesh.rotateAndScaling(0,0,angleSlider_.value, kScale,kScale,kScale);
	wrkMesh.moveModel2(x, y);
	switch(m_cBoxPlaceMetod){
	case 0:	TerrainMetod.mode=sTerrainMetod::PM_Absolutely; break;
	case 1:	TerrainMetod.mode=sTerrainMetod::PM_AbsolutelyMAX; break;
	case 2:	TerrainMetod.mode=sTerrainMetod::PM_AbsolutelyMIN; break;
	case 3:	TerrainMetod.mode=sTerrainMetod::PM_Relatively; break;
	}
	wrkMesh.put2Earch();
	return true;
}

bool CSurTool3DM::CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	pose.trans() = worldCoord;
	UpdateShapeModel();
	return true;
}

void CSurTool3DM::OnBnClickedBtnBrowseModel()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	dataFileName = requestModelAndPut2InternalResource ("Resource\\TerrainData\\Shapes", "*.3dx", "model.3dx",
                                                        TRANSLATE("Выбрать файл с 3DX моделью..."));
	if (dataFileName.empty()) {
		AfxMessageBox(TRANSLATE("Ошибка загрузки 3DX модели."));
	} else {
		std::string::size_type pos = dataFileName.rfind ("\\");
		if (pos != std::string::npos) {
			std::string new_name (dataFileName.begin () + pos + 1, dataFileName.end());
			setName(new_name.c_str());
			updateTreeNode ();
		} else {
		}
	}

	ReloadM3D();
#endif
}


void CSurTool3DM::OnDestroy()
{
	m_layout.reset();
	UpdateData (TRUE);

	CSurToolBase::OnDestroy();

	releaseModels ();
	RELEASE(visualPreviewObject3DM);
}

void CSurTool3DM::OnCbnSelchangeCboxPlacemetod()
{
	// TODO: Add your control notification handler code here
	if(flag_init_dialog) {
		CComboBox* comBox;
		comBox=(CComboBoxEx*)GetDlgItem(IDC_CBOX_PLACEMETOD);
		xassert(comBox->GetCurSel()<PLACEMETOD_MAX_METODS);
		m_cBoxPlaceMetod=cBoxPlaceMetodArr[comBox->GetCurSel()];
	}
}

void CSurTool3DM::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int ctrlID = pScrollBar->GetDlgCtrlID();

	static char fl_Recursion=0;
	if(fl_Recursion==0){//предотвращает рекурсию
		fl_Recursion=1;
		switch(ctrlID){
		case IDC_ANGLE_SLIDER:
		case IDC_D_ANGLE_SLIDER:
		case IDC_SCALE_SLIDER:
		case IDC_D_SCALE_SLIDER:
			{
				UpdateShapeModel();
				break;
			}
		case IDC_RADIUS_SLIDER:
		case IDC_D_RADIUS_SLIDER:
			{
				ReloadM3D ();
				UpdateShapeModel ();
				break;
			}
		}
		fl_Recursion=0;
	}

	CSurToolBase::OnHScroll(nSBCode, nPos, pScrollBar);
}

float CSurTool3DM::getModelRadius (cObject3dx* model)
{
	sBox6f box;
	model->GetBoundBox(box);
	return max((box.max.x - box.min.x)/2, (box.max.y - box.min.y)/2);
}

inline float CSurTool3DM::getScale ()
{
    float sv = float(scaleSlider_.value);
    float sd = float(scaleDeltaSlider_.value);
    return (sv * (100.0f + random_.frnd (sd)) / 100.0f) / 100.0f;
}

void CSurTool3DM::UpdateShapeModel()
{
	random_.set (seed_);
	if(vMap.isLoad() && !visualObjects.empty()){
		int index = 0;
		if (m_bSpread) {
			for(int i = 0; i < visualObjects.size(); ++i) {
				cObject3dx* model = visualObjects[i];
				cObject3dx* logicModel = logicObjects[i] ? logicObjects[i] : model;
				const ObjectSpreader::Circle& circle = objectSpreader.circles()[index];

				model->SetScale(1.0f);
				logicModel->SetScale(1.0f);
				//float radius = model->GetBoundRadius();
				//float logic_radius = logicModel->GetBoundRadius();
				//model->SetScale(circle.radius * getScale () / logic_radius);
				model->SetScale(getScale()*circle.radius/spreadRadiusSlider_.value);
				float radius = model->GetBoundRadius();

                Vect2f pos(Vect2f(pose.trans()) + Vect2f(circle.position));

				model->SetPosition(calculateObjectPose (pos, radius));
				++index;
			}
		} else if (cObject3dx* model = visualObjects.front()) {
            cObject3dx* logicModel = logicObjects.front() ? logicObjects.front() : model;

			model->SetScale(1.0f);
			logicModel->SetScale(1.0f);
			//float radius = model->GetBoundRadius();
			//float logic_radius = logicModel->GetBoundRadius();
			model->SetScale(getScale());
			float radius = model->GetBoundRadius();
			
			Vect3f normal;
			Se3f objectPose = pose;
			Vect2f analyze_pos (clamp (objectPose.trans().x, radius * 1.1f, float(vMap.H_SIZE) - radius * 1.1f),
								clamp (objectPose.trans().y, radius * 1.1f, float(vMap.V_SIZE) - radius * 1.1f));
			objectPose.trans().z = vMap.analyzeArea(analyze_pos, radius, normal);

			float angle = float(angleSlider_.value) + random_.frnd (float(angleDeltaSlider_.value));
			objectPose.rot() = QuatF(angle * (M_PI/180.0f), Vect3f::K);
			
			if (!m_bVertical) {
				Vect3f cross = Vect3f::K % normal;
				float len = cross.norm();

				if(len > FLT_EPS)
					objectPose.rot().premult(QuatF(Acos(dot(Vect3f::K, normal)/(normal.norm() + 1e-5)), cross));
			}

			model->SetPosition(objectPose);
			++index;
			pose = objectPose;
		}
	}
}

void CSurTool3DM::OnCbnSelchangeCboxAttributes()
{
	if(flag_init_dialog) { 
		CComboBoxEx* cbAtr;
		cbAtr=(CComboBoxEx*)GetDlgItem(IDC_CBOX_ATTRIBUTES);
		m_idxCurAttribute=cbAtr->GetCurSel();
	}

}

//////////////////////////////////////////////////////////////////////////////
//
Se3f CSurTool3DM::calculateObjectPose (Vect2f position, float radius)
{
    Vect3f normal;
    Vect3f pos (position.x, position.y, 0.0f);
    pos.z = vMap.analyzeArea(position, radius, normal);

	float angle = float(angleSlider_.value) + random_.frnd (float(angleDeltaSlider_.value));
    Se3f result(QuatF(angle * (M_PI/180.0f), Vect3f::K), pos);

	if (!m_bVertical) {
		Vect3f cross = Vect3f::K % normal;
		float len = cross.norm();

		if(len > FLT_EPS)
			result.rot().premult(QuatF(acosf(dot(Vect3f::K, normal)/(normal.norm() + 1e-5)), cross));
	}
    return result;
}

bool CSurToolEnvironment::CallBack_OperationOnMap(int x, int y)
{
	random_.set (seed_);
	if(visualObjects.size()){
		if(m_bSpread){
			int index = 0;

			Player* player = universe()->worldPlayer ();
			const UnitList units =  player->units ();

			for(int i = 0; i < visualObjects.size(); ++i) {
				cObject3dx* model = visualObjects[i];
				cObject3dx* logicModel = logicObjects[i] ? logicObjects[i] : model;
				const ObjectSpreader::Circle& circle = objectSpreader.circles()[index];

				model->SetScale(1.0f);
				model->SetScale (circle.radius / logicModel->GetBoundRadius() * getScale ());

				float radius = logicModel->GetBoundRadius();
                Vect2f pos(Vect2f(pose.trans()) + Vect2f(circle.position));
				
				UnitList::const_iterator unit_it;
				bool can_be_placed = true;
				FOR_EACH(units, unit_it){
					UnitBase* unit_base = *unit_it;
					UnitEnvironment* unit = dynamic_cast<UnitEnvironment*>(unit_base);
					if (unit && unit->environmentType() != ENVIRONMENT_PHANTOM) {
						float unit_radius = unit->radius ();
						Vect2f unit_position (unit->pose().trans());
						if ((unit_position - pos).norm() < (unit_radius + circle.radius) * 0.8f) {
							can_be_placed = false;
						}
					}
                }
				if (can_be_placed) {
					EnvironmentType environmentType = convertIdx2EnvironmentType(m_idxCurAttribute);
					UnitEnvironment* unit = safe_cast<UnitEnvironment*>(universe()->worldPlayer()->buildUnit(AuxAttributeReference(
						isEnvironmentSimple(environmentType) ? AUX_ATTRIBUTE_ENVIRONMENT_SIMPLE : AUX_ATTRIBUTE_ENVIRONMENT)));
					unit->setEnvirontmentType(environmentType);
					unit->setModel(dataFileName.c_str());
					
					float logic_radius = getScale()*(circle.radius/spreadRadiusSlider_.value)*unit->radius();
					unit->setRadius(logic_radius);
					unit->setPose(calculateObjectPose (pos, logic_radius), true);
					unit->mapUpdate(unit->position2D().x - unit->radius(), unit->position2D().x + unit->radius(), unit->position2D().y - unit->radius(), unit->position2D().y + unit->radius());
				    loadPreset(unit);
				}
				++index;
			}
			ReloadM3D ();
			UpdateShapeModel ();
			eventMaster().eventObjectChanged().emit();
		} 
		else{
			EnvironmentType environmentType = convertIdx2EnvironmentType(m_idxCurAttribute);
			UnitEnvironment* unit = safe_cast<UnitEnvironment*>(universe()->worldPlayer()->buildUnit(AuxAttributeReference(
				isEnvironmentSimple(environmentType) ? AUX_ATTRIBUTE_ENVIRONMENT_SIMPLE : AUX_ATTRIBUTE_ENVIRONMENT)));
			xassert(unit && "Unabled to build AUX_ATTRIBUTE_ENVIRONMENT");
			if(unit){
				unit->setEnvirontmentType(environmentType);
				/*
				if(TreeNode* node = staticSettings.presets[environmentType]){
					EditIArchive ia(node);
                    ia.serialize(*unit, "", "");
				}
				*/
				unit->setModel(dataFileName.c_str());

				float logic_radius = getScale()*unit->radius();
				unit->setRadius(logic_radius);
				
				//float radius = model->GetBoundRadius();
				unit->setPose(calculateObjectPose(pose.trans(), logic_radius), true); // уменьшилась зона сканирования
				unit->mapUpdate(unit->position2D().x - unit->radius(), unit->position2D().x + unit->radius(), unit->position2D().y - unit->radius(), unit->position2D().y + unit->radius());
                loadPreset(unit);
			}
			seed_ = rand();
			UpdateShapeModel();
			eventMaster().eventObjectChanged().emit();
		}
	}
	return true;
}

void CSurToolEnvironment::OnBnClickedBtnBrowseModel()
{
	dataFileName = requestModelAndPut2InternalResource ("Resource\\TerrainData\\Models", "*.3dx",
                                                        "model.3dx", "Will select location of 3DX model");

	if(dataFileName.empty()){
		AfxMessageBox(TRANSLATE("Ошибка загрузки 3DX файла."));
	} else {
		std::string::size_type pos = dataFileName.rfind ("\\");
		std::string::size_type end_pos = dataFileName.rfind (".");
		if (pos != std::string::npos) {
			std::string new_name(dataFileName.begin () + pos + 1, end_pos == std::string::npos ? dataFileName.end() : (dataFileName.begin() + end_pos));
			setName(new_name.c_str());
			updateTreeNode ();
		} else {
		}
	}

	ReloadM3D();
}


void CSurTool3DM::OnCbnSelchangeComboModelshape3d()
{

}

bool CSurTool3DM::CallBack_DrawPreview(int cx, int cy)
{
	if (visualPreviewObject3DM) {
		typedef sVertexXYZDT1 Vertex;
		cQuadBuffer<Vertex>* buffer = gb_RenderDevice->GetQuadBufferXYZDT1();
		gb_RenderDevice->SetNoMaterial(ALPHA_BLEND, MatXf::ID);

		buffer->BeginDraw();
		for (int i = -2; i <= 2; ++i)
		for (int j = -2; j <= 2; ++j)
		{
			float cell_size = 50.0f;
			Vertex* vertices = buffer->Get ();
			vertices [2].pos.set (-0.5f * cell_size + cell_size * (float)i, -0.5f * cell_size + cell_size * (float)j, -10.0f);
			vertices [3].pos.set ( 0.5f * cell_size + cell_size * (float)i, -0.5f * cell_size + cell_size * (float)j, -10.0f);
			vertices [1].pos.set ( 0.5f * cell_size + cell_size * (float)i,  0.5f * cell_size + cell_size * (float)j, -10.0f);
			vertices [0].pos.set (-0.5f * cell_size + cell_size * (float)i,  0.5f * cell_size + cell_size * (float)j, -10.0f);
			sColor4c cell_color = (abs(i + j) % 2) ? sColor4c (255, 255, 255) : sColor4c (100, 0, 0);
			vertices [0].diffuse = vertices [1].diffuse = vertices [2].diffuse = vertices [3].diffuse = cell_color;
		}
		buffer->EndDraw();
	}
	return true;
}

bool CSurToolEnvironment::CallBack_DrawAuxData()
{
	if (vMap.isWorldLoaded ()) {
		if (m_bSpread) {
			int radius = getBrushRadius();
			universe()->circleShow()->Circle (pose.trans(), radius, CircleColor (sColor4c (0, 0, 255, 255)));

			ObjectSpreader::CirclesList::const_iterator it;
			int index = 1;
			FOR_EACH (objectSpreader.circles(), it) {
				if (!it->active) {
					Vect3f position (it->position.x, it->position.y, 10.0f);
					position += pose.trans();
					universe()->circleShow()->Circle (position, it->radius, CircleColor (sColor4c (255, 0, 0, 96)));
					++index;
				} else {
					Vect3f position (it->position.x, it->position.y, 10.0f);
					position += pose.trans();
					universe()->circleShow()->Circle (position, it->radius, CircleColor (sColor4c (0, 128, 255, 96)));
					++index;
				}
			}
			/*
			ObjectSpreader::Node* outline = objectSpreader.getOutline ();
			ObjectSpreader::Node* node = outline;
			do {
				xassert (node && node->next);
				Vect2f c1 (objectSpreader.getCircle (node).position);
				Vect2f c2 (objectSpreader.getCircle (node->next).position);
				Vect3f point1 (c1.x, c1.y, pose.trans().z);
				Vect3f point2 (c2.x, c2.y, pose.trans().z);
				gb_RenderDevice->DrawLine (point1 + pose.trans(), point2 + pose.trans(), sColor4f (200, 0, 0));

				node = node->next;
			} while (node != outline);
			*/
		}
	}
	return true;
}

bool CSurToolEnvironment::ReloadM3D()
{
	seed_ = rand ();
	if (m_bSpread) {
		objectSpreader.setSeed (seed_);
		float scale = float(max (5, spreadRadiusSlider_.value));
		float scaleDelta = float(spreadRadiusDeltaSlider_.value);
		objectSpreader.setRadius (Rangef(scale - scale * scaleDelta / 100.0f, scale + scale * scaleDelta / 100.0f));
		int radius = getBrushRadius ();
		objectSpreader.fill(CircleInRadius(radius));
	}

    releaseModels ();
    RELEASE(visualPreviewObject3DM);
    if(!dataFileName.empty()){
        CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_MODEL);
        ew->SetWindowText(dataFileName.c_str());
        if (terScene) {
            if (m_bSpread) {
                createModels (dataFileName.c_str(), objectSpreader.circles().size());
            } else {
                createModels (dataFileName.c_str(), 1);
            }
        }
        if (getPreviewScene()) {
            if (visualPreviewObject3DM = getPreviewScene()->CreateObject3dx(dataFileName.c_str())) {
                sBox6f box;
                visualPreviewObject3DM->GetBoundBox (box);
                float scale = 80.0f / box.GetRadius ();
                visualPreviewObject3DM->SetScale(scale);
                visualPreviewObject3DM->SetPosition (MatXf::ID);
            }
        }

        UpdateShapeModel();
        return true;
    }
    return false;
}


void CSurTool3DM::OnSpreadCheckClicked()
{
	UpdateData (TRUE);
	ReloadM3D ();
	UpdateShapeModel ();
}

void CSurTool3DM::OnVerticalCheckClicked()
{
	UpdateData (TRUE);
	UpdateShapeModel ();
}

void CSurTool3DM::OnSize(UINT nType, int cx, int cy)
{
	CSurToolBase::OnSize(nType, cx, cy);

	m_layout.onSize (cx, cy);
	Invalidate ();
}

void CSurToolEnvironment::CallBack_BrushRadiusChanged()
{
	ReloadM3D ();
	UpdateShapeModel ();
}


BOOL CSurToolEnvironment::OnInitDialog()
{
	CSurTool3DM::OnInitDialog();

	GetDlgItem(IDC_STATIC_LINE2)->ShowWindow (SW_HIDE);
	GetDlgItem(IDC_CBOX_PLACEMETOD)->ShowWindow (SW_HIDE);

	return FALSE; // return TRUE unless you set the focus to a control
}

namespace{
const char* presetPathPrefix = "Scripts\\TreeControlSetups\\EnvironmentPreset_";
};

void CSurToolEnvironment::savePreset(UnitEnvironment* unit)
{
	XBuffer buf(256, 1);
	buf < presetPathPrefix < getEnumDescriptor(unit->environmentType()).name(unit->environmentType());

	XPrmOArchive oa(buf);
	oa.serialize(*unit, "unit", 0);
}

void CSurToolEnvironment::loadPreset(UnitEnvironment* unit)
{
	XBuffer buf(256, 1);
	buf < presetPathPrefix < getEnumDescriptor(unit->environmentType()).name(unit->environmentType());

	XPrmIArchive ia;
	if(ia.open(buf)){
		std::string model = unit->modelName();
		float radius = unit->radius();
		Se3f pose = unit->pose();
		ia.serialize(*unit, "unit", 0);
		unit->setModel(model.c_str());
		unit->setRadius(radius);
		unit->setPose(pose, true);
		if(unit->rigidBody())
			unit->rigidBody()->awake();
	}
}
