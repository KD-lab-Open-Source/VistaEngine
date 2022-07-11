#include "stdafx.h"
#include "Configurator.h"
#include "ConfiguratorDlg.h"
#include "FileUtils.h"
#include "GameOptions.h"

#include "AttribEditorInterface.h"
#include "Dictionary.h"
#include "ZipConfig.h"

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
	

	if(__argc > 1){
		std::string workingDir = extractFilePath(__argv[0]);
		SetCurrentDirectory(workingDir.c_str());
	}
	ZipConfig::initArchives();

	LocLibraryWrapperBase::locDataRootPath() = ".\\Resource\\LocData";
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

unsigned long tls_is_graph = 1; // для Console