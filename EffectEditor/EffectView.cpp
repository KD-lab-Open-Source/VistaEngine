#include "StdAfx.h"

#include "Render/src/Scene.h"
#include "Render/src/TileMap.h"
#include "Render/Src/cCamera.h"
#include "Render/inc/IRenderDevice.h"

#include "EffectView.h"
#include "EffectDocument.h"

#include "kdw/Document.h"
#include "kdw/Navigator.h"
#include "kdw/Serialization.h"
#include "Serialization/SerializationFactory.h"
#include "kdw/CommandManager.h"

#include "kdw/Toolbar.h"
#include "kdw/ImageStore.h"

#include "kdw/HBox.h"
#include "kdw/ComboBox.h"
#include "kdw/Entry.h"
#include "kdw/Label.h"
#include "kdw/ColorChooserDialog.h"
#include "kdw/Tool.h"

extern RENDER_API cInterfaceRenderDevice* gb_RenderDevice;
extern RENDER_API cVisGeneric*            gb_VisGeneric;
// ---------------------------------------------------------------------------
namespace kdw{
	REGISTER_CLASS(Widget, EffectView, "ModelViewer\\Окно просмотра")
}


EffectView::EffectView(bool continuousUpdate)
: ToolViewport(gb_RenderDevice, globalDocument, continuousUpdate)
, showTerrain_(true)
, showGrid_(true)
, showOverdraw_(false)
, enableLighting_(true)
{
	setFillColor(64, 92, 64);
	setScene(document()->scene());
	setCamera(document()->scene()->CreateCamera());
	setSandbox(document()->sandbox());
	camera()->setAttribute(ATTRCAMERA_PERSPECTIVE); // перспектива
	xassert(camera());
}

EffectView::~EffectView()
{
    RELEASE(camera_);
}

void EffectView::onIntialize()
{
	globalDocument->onRenderDeviceInitialize();

	__super::onInitialize();
}

void EffectView::onRedraw()
{
	if(cEffect* effect = document()->effect())
		effect->enableOverDraw = showOverdraw_;

	//globalDocument->sandbox()->

	//globalDocument->environment()->draw

	if(scene()->GetTileMap()){
		setEnableLighting(enableLighting_);
		if(showTerrain_)
			scene()->GetTileMap()->clearAttribute(ATTRUNKOBJ_IGNORE);
		else
			scene()->GetTileMap()->setAttribute(ATTRUNKOBJ_IGNORE);
	}
	__super::onRedraw();
}


void EffectView::setShowGrid(bool showGrid)
{
	showGrid_ = showGrid;
}


void EffectView::setShowOverdraw(bool overdraw)
{
	showOverdraw_ = overdraw;
}

void EffectView::setShowTerrain(bool showTerrain)
{
	showTerrain_ = showTerrain;

}

void EffectView::setBackgroundColor(const Color4f& color)
{
	Color4c c(color);
	setFillColor(c.r, c.g, c.b);
}

void EffectView::setEnableLighting(bool enableLighting)
{
	enableLighting_ = enableLighting;
	if(enableLighting_){
		if(scene()->GetTileMap())
			scene()->GetTileMap()->SetDiffuse(Color4f(0.98468125f,0.98468125f,0.85692370f,0.47104770f));
		    scene()->SetSunColor(Color4f(0.94209540f,0.94209540f,0.94209540f,1.f),
			                     Color4f(0.98468125f,0.98468125f,0.85692370f,1.f),
								 Color4f(0.98468125f,0.98468125f,0.85692370f,1.f));
		scene()->SetSunDirection(Vect3f(0.0f, -1.0f, -1.0f));
	}
	else{
		if(scene()->GetTileMap())
			scene()->GetTileMap()->SetDiffuse(Color4f(0.80000001f,0.80000001f,0.80000001f,0.19999999f));
		scene()->SetSunColor(Color4f(1.0f, 1.0f, 1.0f, 1.0f),
							 Color4f(1.0f, 1.0f, 1.0f, 1.0f),
							 Color4f(1.0f, 1.0f, 1.0f, 1.0f));
		scene()->SetSunDirection(Vect3f(0.0f, -1.0f, -1.0f));
	}
}

EffectDocument* EffectView::document()
{
	return globalDocument;
}

// ---------------------------------------------------------------------------
namespace kdw{
	REGISTER_CLASS(Space, EffectViewSpace, "3D Вид")
}

class TransformOptionsWidget : public kdw::HBox{
public:
	TransformOptionsWidget(){
		add(new kdw::Label(TRANSLATE("Коорд.:")), false, true, true);
		kdw::ComboBox* spaceBox = new kdw::ComboBox(true);
		spaceBox->set("Local|World|Space");
		spaceBox->setRequestSize(Vect2i(100, 1));
		add(spaceBox, false, true, true);
	}
};

EffectViewSpace::EffectViewSpace()
: view_(0)
, showGrid_(false)
, showTerrain_(false)
, showOverdraw_(false)
, backgroundColor_(0.2f, 0.3f, 0.3f, 1.0f)
{
	view_ = new EffectView(true);

	updateMenus();
	setMenu("view");
	
	//addToHeader(new TransformOptionsWidget());
	kdw::Toolbar* toolbar = new kdw::Toolbar(&commands());
	kdw::ImageStore* imageStore = new kdw::ImageStore(24, 24);
	imageStore->addFromResource("VIEW", RGB(255, 0, 255));
	toolbar->setImageStore(imageStore);

	toolbar->addButton("view.view.focusEffect", 9);
	toolbar->addButton("view.view.enableLighting", 8);
	toolbar->addSeparator();
	toolbar->addButton("view.view.backgroundColor", 5);
	toolbar->addButton("view.view.showOverdraw", 6);
	toolbar->addSeparator();
	addToHeader(toolbar);
		
	add(view_);
}

EffectViewSpace::~EffectViewSpace()
{
	view_ = 0;
}


namespace kdw{
	class ToolNavigator;
	class ToolSelect;
	class ToolMove;
}

void EffectViewSpace::updateMenus()
{

	commands().get("view.view.focusEffect")
		.connect(this, &EffectViewSpace::onMenuViewFocusEffect);

	commands().get("view.view.focusAll")
		.connect(this, &EffectViewSpace::onMenuViewFocusAll);
	
	commands().get("view.view.focusSelected")
		.connect(this, &EffectViewSpace::onMenuViewFocusSelected);

	/*
	commands().get("view.view.showGrid")
		.connect(this, &EffectViewSpace::onMenuViewShowGrid)
		.check(showGrid_)
		.setAutoCheck(true);
		*/

	commands().get("view.view.backgroundColor")
		.connect(this, &EffectViewSpace::onMenuViewBackgroundColor);

	commands().get("view.view.showOverdraw")
		.check(showOverdraw_)
		.setAutoCheck(true)
		.connect(this, &EffectViewSpace::onMenuViewShowOverdraw);

	commands().get("view.view.showTerrain")
		.connect(this, &EffectViewSpace::onMenuViewShowTerrain)
		.check(showTerrain_)
		.setAutoCheck(true);
	commands().get("view.view.enableLighting")
		.connect(this, &EffectViewSpace::onMenuViewEnableLighting)
		.check(enableLighting_)
		.setAutoCheck(true);
}

void EffectViewSpace::onMenuViewFocusSelected()
{
	view_->focusSelected();
}

void EffectViewSpace::onMenuViewFocusAll()
{
	view_->focusAll();
}

void EffectViewSpace::onMenuViewFocusEffect()
{
	Vect3f target = Vect3f(globalDocument->worldCenter()) + globalDocument->effectOrigin();

	Se3f cameraPose = view_->cameraPose();
	Vect3f direction = Vect3f::K * view_->cameraDistance();
	cameraPose.rot().xform(direction);
	cameraPose.trans() = target - direction;
	view_->setCameraPose(cameraPose);
}

void EffectViewSpace::onMenuViewBackgroundColor()
{
	kdw::ColorChooserDialog dialog(widget()->mainWindow(), backgroundColor_);
	if(dialog.showModal() == kdw::RESPONSE_OK){
		backgroundColor_ = dialog.get();
		view_->setBackgroundColor(backgroundColor_);
	}
}

void EffectViewSpace::onMenuViewEnableLighting()
{
	enableLighting_ = commands().get("view.view.enableLighting").checked();
    view_->setEnableLighting(enableLighting_);
}

void EffectViewSpace::onMenuViewShowTerrain()
{
	showTerrain_ = commands().get("view.view.showTerrain").checked();
	view_->setShowTerrain(showTerrain_);
}


void EffectViewSpace::onMenuViewShowOverdraw()
{
	showOverdraw_ = commands().get("view.view.showOverdraw").checked();
	view_->setShowOverdraw(showOverdraw_);
}


void EffectViewSpace::onMenuViewShowGrid()
{
	//showGrid_ = commands().get("view.view.showGrid").checked();
	//view_->setShowGrid(showGrid_);
}






void EffectViewSpace::serialize(Archive& ar)
{
	__super::serialize(ar);
	xassert(view_);
	ar.serialize(*view_, "view", "Вид");
	if(ar.filter(kdw::SERIALIZE_STATE)){
		ar.serialize(showGrid_, "showGrid", "Показывать сетку");
		ar.serialize(showTerrain_, "showTerrain", "Показывать мир");
		ar.serialize(showOverdraw_, "showOverdraw", "Рисовать Overdraw");
		ar.serialize(enableLighting_, "enableLighting", "Включить освещение");
		ar.serialize(backgroundColor_, "backgroundColor", "Цвет фона");
		view_->serialize(ar);
		if(ar.isInput()){
			updateMenus();
			onMenuViewEnableLighting();
			onMenuViewShowTerrain();
			onMenuViewShowGrid();
			onMenuViewShowOverdraw();
			view_->setBackgroundColor(backgroundColor_);
		}
	}
	
}


