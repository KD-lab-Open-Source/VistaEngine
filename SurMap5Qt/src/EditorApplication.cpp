// EditorApplication.cpp — see header.

#include "EditorApplication.h"

#include <QDir>

EditorApplication* EditorApplication::s_instance = nullptr;

EditorApplication::EditorApplication()
{
	// The one editor object (main.cpp constructs it before the window).
	s_instance = this;

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

	// The worlds directory must be absolute: vMap::load builds
	// <worldsDir>\<world>\world.cls relative to the working directory, and the
	// editor's working directory is wherever the exe was launched from.
	worldsDir_ = QDir::current().absoluteFilePath(worldsDir_);
	return true;
}
