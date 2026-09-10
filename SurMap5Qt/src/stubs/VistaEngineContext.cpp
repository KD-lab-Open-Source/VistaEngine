// SurMap5Qt's editor-side engine context — SurMap5/VistaEngineContext.cpp
// ported to the Qt editor.
//
// The original Game build provides the *game* versions of these symbols in
// Game/GameContext.cpp (the real Action activate() bodies, the real
// Condition check() bodies, isUnderEditor() = false). The MFC SurMap5 build
// did NOT compile GameContext.cpp: SurMap5/VistaEngineContext.cpp supplied
// the same symbols as editor no-ops (a map editor runs no missions, so
// starting a mission / joining a LAN game / checking net status are all
// "do nothing" / "false"), plus isUnderEditor() = true, which switches the
// engine into editor mode (no trigger scripts, no mission logic, no fog of
// war, no terrain auto-impermeability — see every `isUnderEditor()` site).
//
// SurMap5Qt follows the same split: EditorEngine pulls the Game sources it
// needs but NOT GameContext.cpp; this file is the single definition of the
// editor-side context, exactly as SurMap5/VistaEngineContext.cpp was. It also
// carries editorVisual() (EditorVisualImpl): the engine's EditorVisual
// no-op stand-in that SurMap5 kept in SurMap5/EditorVisualImpl.cpp (MFC),
// here engine-side — everything is visible and the draw helpers draw nothing,
// which is what the always-visible editor view needs until the View filters
// land.

#include <vector>
#include <string>
using namespace std;   // engine headers expect the StdAfx preamble

#include "xutil.h"      // xassert
#include "my_STL.h"     // FOR_EACH and friends

#include "Serialization/Serialization.h"   // Archive full type first — CommandsQueue.h /
                                          // TriggerExport.h use UniqueVector<Archive&>
#include "Game/Actions.h"
#include "Game/Conditions.h"
#include "Game/GameOptions.h"
#include "Serialization/SerializationFactory.h"
#include "Units/CommandsQueue.h"
#include "Render/src/VisGeneric.h"
#include "EditorVisual.h"
#include "Game/CameraManager.h"   // cameraManager (drawText projection)
#include "Game/Universe.h"        // universe()->circleManager() (drawRadius)
#include "Water/CircleManager.h"  // CircleManager::addCircle (drawRadius)
#include "Render/src/cCamera.h"   // ConvertorWorldToViewPort

namespace {
	class EditorVisualImpl : public EditorVisual::Interface {
	public:
		bool isVisible(UniverseObjectClass) override { return true; }
		void beforeQuant() override {}
		void afterQuant() override {}
		void drawImpassabilityRadius(UnitBase&) override {}
		// Port of SurMap5/EditorVisualImpl.cpp: the 3D aux marks the original
		// drew through gb_RenderDevice (DrawLine) and circleManager (addCircle).
		void drawCross(const Vect3f& position, float size, EditorVisual::CrossType, bool selected) override
		{
			const Color4c color = selected ? Color4c::RED : Color4c::GREEN;
			gb_RenderDevice->DrawLine(position + Vect3f(-size, 0.0f, 0.0f), position + Vect3f(size, 0.0f, 0.0f), color);
			gb_RenderDevice->DrawLine(position + Vect3f(0.0f, -size, 0.0f), position + Vect3f(0.0f, size, 0.0f), color);
		}
		void drawRadius(const Vect3f& position, float radius, EditorVisual::RadiusType radiusType, bool selected) override
		{
			// Port of SurMap5/EditorVisualImpl.cpp, but drawn directly through
			// DrawLine: the original's circleManager()->addCircle goes through
			// the world-quad pass, which never becomes visible in the editor
			// frame. A 36-segment ring like CSurToolTransform::drawCircle.
			const Color4c color = selected ? Color4c::RED : Color4c::GREEN;
			const float r = max(radius, 7.0f);
			const int segs = 36;
			const float step = 2.0f * 3.14159265f / (float)segs;
			Vect3f prev(position.x + r, position.y, position.z);
			for(int i = 1; i <= segs; ++i){
				const float a = step * (float)i;
				const Vect3f cur(position.x + cosf(a) * r, position.y + sinf(a) * r, position.z);
				gb_RenderDevice->DrawLine(prev, cur, color);
				prev = cur;
			}
			(void)radiusType;
		}
		void drawText(const Vect3f& position, const char* text, EditorVisual::TextType textType) override
		{
			if(!cameraManager)
				return;
			Vect3f e, w;
			cameraManager->GetCamera()->ConvertorWorldToViewPort(&position, &w, &e);
			Color4c color = Color4c::WHITE;
			if(textType == EditorVisual::TEXT_LABEL)
				color = Color4c::RED;
			else if(textType == EditorVisual::TEXT_PROPERTIES){
				color = Color4c::BLUE;
				e.y += 16;
			}
			gb_RenderDevice->OutText((int)round(e.x), (int)round(e.y), text, Color4f(color));
		}
		void drawOrientationArrow(const Se3f& pose, bool) override
		{
			const Vect3f pos3d = pose.trans();
			const float scale = 100.0f;
			static const Vect3f points[] = {
				Vect3f( 0.0f,  0.5f, 0.0f),
				Vect3f( 0.0f, -0.5f, 0.0f),
				Vect3f( 0.1f,  0.4f, 0.0f),
				Vect3f( 0.0f,  0.5f, 0.0f),
				Vect3f(-0.1f,  0.4f, 0.0f),
				Vect3f( 0.0f,  0.5f, 0.0f),
			};
			for(size_t i = 1; i < sizeof(points) / sizeof(points[0]); i += 2){
				Vect3f a = points[i - 1] * scale;
				Vect3f b = points[i] * scale;
				pose.rot().xform(a);
				pose.rot().xform(b);
				gb_RenderDevice->DrawLine(pos3d + a, pos3d + b, Color4c(0, 200, 0));
			}
		}
	};
}

EditorVisual::Interface& editorVisual()
{
	static EditorVisualImpl impl;
	return impl;
}

// --- The Action queue: the map editor never runs a mission, so every
// action body is empty (SurMap5/VistaEngineContext.cpp). ---

void ActionSetDirectControl::activate()
{
}

void ActionShowReel::activate()
{
}

bool ActionShowReel::workedOut()
{
	return true;
}

void ActionShowLogoReel::activate()
{
}

void ActionStartMission::activate()
{
}

void ActionLoadGameAuto::activate()
{
}

void ActionSetControlEnabled::activate()
{
}

void ActionSetCurrentMission::activate()
{
}

void ActionResetCurrentMission::activate()
{
}

void ActionSetGamePause::activate()
{
}

void ActionExitFromMission::activate()
{
}

void ActionSetCursor::activate()
{
}

void ActionFreeCursor::activate()
{
}

void ActionChangeUnitCursor::activate()
{
}

void ActionCreateNetClient::activate()
{
}

void ActionOnlineLogout::activate()
{
}

void ActionUI_GameStart::activate()
{
}

void ActionUI_LanGameStart::activate()
{
}

void ActionUI_LanGameJoin::activate()
{
}

void ActionUI_LanGameCreate::activate()
{
}

void ActionResetNetCenter::activate()
{
}

void ActionKillNetCenter::activate()
{
}

void ActionToggleBuildingInstaller::activate()
{
}

void ActionUI_UnitCommand::activate()
{
}

void ActionDeselect::activate()
{
}

void ActionGameQuit::activate()
{
}

void ActionSelectUnit::activate()
{
}

void ActionSetInt::activate()
{
}

void ActionSetCutScene::activate()
{
}

void ActionSetCurrentMissionAsPassed::activate()
{
}

// --- The Condition queue: all "no" for the editor
// (SurMap5/VistaEngineContext.cpp). ---

bool ConditionLastNetStatus::check() const
{
	return false;
}

bool ConditionSelected::check() const
{
	return false;
}

bool ConditionSquadSelected::check() const
{
	return false;
}

bool ConditionCheckPause::check() const
{
	return false;
}

bool ConditionIsMultiplayer::check() const
{
	return false;
}

bool ConditionPlayerByNumberDefeat::check() const
{
	return false;
}

// applicationHasFocus() and updateResolution() are NOT here: Game/Runtime.cpp
// (also in EditorEngine) defines both — the editor runs the real Runtime, so
// their live bodies win.

bool isUnderEditor()
{
	return true;
}

// --- Engine functions the editor provides instead of the game build. ---

void UnitBase::showPathTrackingMap()
{
}

MissionDescription* MissionDescriptionForTrigger::operator ()() const
{
	return 0;
}

void UnitActing::setActiveDirectControl(DirectControlMode activeDirectControl, int transitionTime)
{
}

void UpdateSilhouettes()
{
	gb_VisGeneric->EnableSilhouettes(GameOptions::instance().getBool(OPTION_SILHOUETTE));
}

CommandColorManager::CommandColorManager()
{
	for(int i = 0; i < COMMAND_MAX; i++){
		float hue = (360.f * i) / COMMAND_MAX;
		Color4f color;
		color.setHSV(hue, 1.f, 1.f);
		colors_.push_back(Color3c(color));
	}
}
