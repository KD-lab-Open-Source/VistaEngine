// EditorApplication.cpp — see header.

#include "EditorApplication.h"

EditorApplication::EditorApplication()
{
	// ~60 Hz, mirroring the repaint-driven loop of CSurMap5App::OnIdle.
	loopTimer_.setInterval(16);
	loopTimer_.start();
}

EditorApplication::~EditorApplication() = default;

bool EditorApplication::initialize()
{
	// Qt already provides what SetRegistryKey/AfxOleInit did:
	//   - QSettings (set up in main.cpp via setOrganizationName/ApplicationName)
	//   - clipboard, drag&drop, rich text — no explicit init required.
	//
	// Phase 2+: engine startup moves here, in dependency order:
	//   ZipConfig::initArchives();
	//   UI_Render::create();
	//   TranslationManager::instance().setTranslationsDir(...);
	//   TranslationManager::instance().setDefaultLanguage("english");
	//   TranslationManager::instance().setLanguage(GameOptions::instance().getLanguage());
	//   EffectContainer::setTexturesPath("Resource\\FX\\Textures");
	return true;
}
