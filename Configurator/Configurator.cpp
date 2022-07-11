#include "stdafx.h"
#include "Configurator.h"
#include "ConfiguratorDlg.h"
#include "FileUtils\FileUtils.h"
#include "Game\GameOptions.h"

#include "Serialization\Dictionary.h"
#include "ZipConfig.h"
#include "kdw/Win32/Window.h"
#include "kdw/kdWidgetsLib.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


BEGIN_MESSAGE_MAP(CConfiguratorApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


CConfiguratorApp::CConfiguratorApp()
{
}


CConfiguratorApp theApp;


BOOL CConfiguratorApp::InitInstance()
{
	InitCommonControls();
	CWinApp::InitInstance();
	AfxEnableControlContainer();
	
	Win32::_setGlobalInstance(m_hInstance);

	if(__argc > 1){
		std::string workingDir = extractFilePath(__argv[0]);
		SetCurrentDirectory(workingDir.c_str());
	}
	ZipConfig::initArchives();

	TranslationManager::instance().setTranslationsDir("Scripts\\Engine\\Translations");
	TranslationManager::instance().setDefaultLanguage("english");
	GameOptions::instance().setTranslate();
	GameOptions::instance().filterBaseGraphOptions();
	TranslationManager::instance().setLanguage(GameOptions::instance().getLanguage());

	if(__argc > 1){
		AfxMessageBox(TRANSLATE("Конфигуратор не поддерживает аргументов коммандной строки"));
	}
	else{
		CConfiguratorDlg dlg;
		m_pMainWnd = &dlg;
		dlg.DoModal();
	}
	return FALSE;
}

bool isUnderEditor()
{
	return false;
}


void GameOptions::defineGlobalVars()
{

}

const char* getLocDataPath() 
{
	return GameOptions::instance().getLocDataPath();
}

bool MT_IS_LOGIC() { return false; }
bool MT_IS_GRAPH() { return true; }
