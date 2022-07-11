#include "StdAfx.h"
#include "Serialization\RangedWrapper.h"
#include "ComboVectorString.h"
#include "GameOptions.h"
#include "RenderObjects.h"

#include "Serialization\Serialization.h"

#include "Game\Universe.h"
#include "Environment\Environment.h"
#include "Game\CameraManager.h"
#include "Game\SoundApp.h"
#include "UserInterface\UI_Logic.h"
#include "VistaRender\postEffects.h"
#include "Render\Src\TexLibrary.h"
#include "Render\src\Scene.h"
#include "Render\src\Grass.h"
#include "Render\src\VisGeneric.h"
#include "Water\Fallout.h"

void UpdateSilhouettes();


void GameOptions::filterGraphOptions()
{
	OptionPrm& prmAF = gameOptionPrms_[OPTION_ANISOTROPY];
	dassert(anisatropicFiltering_.size() == prmAF.valid_.size());

	for(int i = 1; i < anisatropicFiltering_.size(); ++i)
		prmAF.valid_[i] = anisatropicFiltering_[i] < gb_VisGeneric->GetMaxAnisotropyLevel();

	Option* opt = getOptionObject(OPTION_ANISOTROPY);
	xassert(opt);
	opt->data_ = raw2filtered(OPTION_ANISOTROPY, opt->data_);

	optionsSaved_ = options_;
}

void GameOptions::loadPresets(int number)
{
	if(number >= presets_.size())
		return;

	Options::const_iterator it;
	FOR_EACH(presets_[number].preset_, it)
		if(Option* opt = getOptionObject(it->type_))
			opt->data_ = raw2filtered(it->type_, it->data_);
}

void GameOptions::defineGlobalVars()
{
	locDataPath_ = locDataRoot_ + getLanguage() + "\\";

	terFullScreen  = getBool(OPTION_FULL_SCREEN);

	currentResolution_ = resolutions_[getInt(OPTION_SCREEN_SIZE)];
}

void GameOptions::graphSetup()
{
	gb_VisGeneric->SetMapLevel(getInt(OPTION_MAP_LEVEL_LOD));

	int sh = getInt(OPTION_SHADOW);
	bool shadowEnable = (sh > 0); // Disable|Circle|bad=Small2x|good=Big4x
	int shadowSize = (sh == 2 ? 4 : 3); // 4 - Big, 3 - Small
	bool shadow4x4 = (gb_VisGeneric->PossibilityShadowMapSelf4x4() && sh == 2);

	gb_VisGeneric->SetShadowType(shadowEnable, shadowSize);
	gb_VisGeneric->SetShadowMapSelf4x4(shadow4x4);
	gb_VisGeneric->SetMaximalShadowObject(shadowEnable ? OST_SHADOW_REAL : OST_SHADOW_NONE);

	gb_VisGeneric->SetEnableBump(gb_VisGeneric->PossibilityBump() ? getBool(OPTION_BUMP) : false);

	gb_VisGeneric->setTileMapVertexLight(!getBool(OPTION_TILEMAP_TYPE_NORMAL));

	gb_VisGeneric->SetTilemapDetail(getBool(OPTION_TILEMAP_DETAIL));

	gb_VisGeneric->SetGlobalParticleRate(getFloat(OPTION_PARTICLE_RATE));

	UpdateSilhouettes();

	gb_VisGeneric->SetFloatZBufferType(getInt(OPTION_SOFT_SMOKE));

	int texLevel = 2 - getInt(OPTION_TEXTURE_DETAIL_LEVEL);
	if (gb_VisGeneric->GetTextureDetailLevel() != texLevel){
		gb_VisGeneric->SetTextureDetailLevel(texLevel);
		GetTexLibrary()->ReloadAllTexture();
	}

	gb_VisGeneric->SetAnisotropic(getInt(OPTION_ANISOTROPY));

	gb_RenderDevice->SetGamma(getFloat(OPTION_GAMMA));
}

void GameOptions::environmentSetup()
{
	xassert(environment);

	environment->scene()->EnableReflection(getBool(OPTION_REFLECTION));

	environment->PEManager()->setActive(PE_BLOOM,getBool(OPTION_BLOOM));
	environment->PEManager()->setActive(PE_MIRAGE,getBool(OPTION_MIRAGE));
	environment->PEManager()->setActive(PE_UNDER_WATER,getBool(OPTION_MIRAGE));
	environment->fallout()->Enable(getBool(OPTION_WEATHER));

	if(environment->grass())
		environment->grass()->SetDensity(getFloat(OPTION_GRASSDENSITY));

	if(!isUnderEditor())
		universe()->setShowFogOfWar(!debugDisableFogOfWar);
	else
		universe()->setShowFogOfWar(getBool(OPTION_FOG_OF_WAR));

	water->setTechnique();
}

void GameOptions::gameSetup()
{
	voiceManager().setEnabled(getBool(OPTION_VOICE_ENABLE));

	void InitSound(bool, bool, HWND, const char*);
	InitSound(getBool(OPTION_SOUND_ENABLE), getBool(OPTION_MUSIC_ENABLE), gb_RenderDevice->GetWindowHandle(), this->getLocDataPath());

	if(getBool(OPTION_MUSIC_ENABLE))
		musicManager.Resume();
	else
		musicManager.Pause();

	extern CameraManager* cameraManager;
	if(cameraManager){
		cameraManager->setRestriction(getBool(OPTION_CAMERA_RESTRICTION));

		if(!getBool(OPTION_CAMERA_UNIT_FOLLOW))
			cameraManager->disableDirectControl();

		cameraManager->setUnitFollowMode(getBool(OPTION_CAMERA_UNIT_DOWN_FOLLOW), getBool(OPTION_CAMERA_UNIT_ROTATE));

		cameraManager->setInvertMouse(getBool(OPTION_CAMERA_INVERT_MOUSE));
	}

	UI_LogicDispatcher::instance().setShowTips(getBool(OPTION_SHOW_HINTS));
	UI_LogicDispatcher::instance().setShowMessages(getBool(OPTION_SHOW_MESSAGES));
}    


void GameOptions::userApply(bool silent)
{
	SECUROM_MARKER_HIGH_SECURITY_ON(11);

	bool critical_changes = false;
	xassert(options_.size() == optionsSaved_.size());
	for(int idx = 0; idx < options_.size(); ++idx){
		xassert(options_[idx].type_ == optionsSaved_[idx].type_);
		if(gameOptionPrms_[options_[idx].type_].isCritical())
			if(options_[idx].data_ != optionsSaved_[idx].data_){
				critical_changes = true;
				break;
			}
	}

	
	defineGlobalVars();
	
	if(gb_VisGeneric && gb_RenderDevice)
		graphSetup();

	gameSetup();

	if(environment)
		environmentSetup();

	if(critical_changes || silent)
		updateResolution(getScreenSize(), true);

	//Эта строчка введена для предотвращения бага с трешем видеопамяти.
	//Должна вызываться после применения всех графических настроек.
	gb_RenderDevice->RestoreDeviceForce();
	
	if(silent || !critical_changes)
		commitSettings();
	else
		commitTimer_ = COMMIT_TIME;

	SECUROM_MARKER_HIGH_SECURITY_OFF(11);
}

void GameOptions::setPartialOptionsApply()
{
	UpdateSound();
	
	if(gb_RenderDevice)
		gb_RenderDevice->SetGamma(getFloat(OPTION_GAMMA));
}

void GameOptions::revertChanges()
{
	setOptions(optionsSaved_);
}

void GameOptions::commitSettings()
{
	commitTimer_ = 0.f;
	optionsSaved_ = options_;
	saveLibrary();
}

void GameOptions::restoreSettings()
{
	if(needCommit() || needRollBack()){
		setOptions(optionsSaved_);
		userApply(true);
	}
}

void GameOptions::commitQuant(float dt) 
{
	if(needCommit())
		commitTimer_ = (dt >= commitTimer_ ? -1.f : commitTimer_ - dt);
}
