#include "StdAfx.h"

#include "MainDlg.h"
#include "resource.h"

#define IGAMEEXPORTER_CLASS_ID	Class_ID(0x79d613a4, 0x4f21c3ad)

class IGameExporter : public SceneExport {
	public:

		static HWND hParams;

		bool showPrompts;
		bool exportSelected;

		int				ExtCount();					// Number of extensions supported
		const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
		const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
		const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
		const TCHAR *	AuthorName();				// ASCII Author name
		const TCHAR *	CopyrightMessage();			// ASCII Copyright message
		const TCHAR *	OtherMessage1();			// Other message #1
		const TCHAR *	OtherMessage2();			// Other message #2
		unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
		void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box

		BOOL SupportsOptions(int ext, DWORD options);
		int	DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0);

		
		void ExportMaterials();

		IGameExporter();
		~IGameExporter();	

		CMainDlg m_dlg;
};

class IGameExporterClassDesc:public ClassDesc2 {
	public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new IGameExporter(); }
	const TCHAR *	ClassName() { return "Exporter3dx"; }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID() { return IGAMEEXPORTER_CLASS_ID; }
	const TCHAR* 	Category() { return "Standard"; }

	const TCHAR*	InternalName() { return _T("Exporter3dx"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle

};



static IGameExporterClassDesc IGameExporterDesc;
ClassDesc2* GetIGameExporterDesc() { return &IGameExporterDesc; }
RootExport* pRootExport=NULL;

//--- IGameExporter -------------------------------------------------------
IGameExporter::IGameExporter()
{
	pRootExport=NULL;
}

IGameExporter::~IGameExporter() 
{
	pRootExport=NULL;
}

int IGameExporter::ExtCount()
{
	//TODO: Returns the number of file name extensions supported by the plug-in.
	return 1;
}

const TCHAR *IGameExporter::Ext(int n)
{		
	//TODO: Return the 'i-th' file name extension (i.e. "3DS").
	return _T("3dx");
}

const TCHAR *IGameExporter::LongDesc()
{
	//TODO: Return long ASCII description (i.e. "Targa 2.0 Image File")
	return _T("3dx Exporter");
}
	
const TCHAR *IGameExporter::ShortDesc() 
{			
	//TODO: Return short ASCII description (i.e. "Targa")
	return _T("3dx Exporter (K-D Lab)");
}

const TCHAR *IGameExporter::AuthorName()
{			
	//TODO: Return ASCII Author name
	return _T("Balmer (Poskryakov Dmitry)");
}

const TCHAR *IGameExporter::CopyrightMessage() 
{	
	// Return ASCII Copyright message
	return _T("3dx Exporter (K-D Lab)");
}

const TCHAR *IGameExporter::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("3dx Exporter (K-D Lab)");
}

const TCHAR *IGameExporter::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("3dx Exporter (K-D Lab)");
}

unsigned int IGameExporter::Version()
{				
	//TODO: Return Version number * 100 (i.e. v3.01 = 301)
	return 100;
}

void IGameExporter::ShowAbout(HWND hWnd)
{			
	// Optional
}

BOOL IGameExporter::SupportsOptions(int ext, DWORD options)
{
	// TODO Decide which options to support.  Simply return
	// true for each option supported by each Extension 
	// the exporter supports.

	return TRUE;
}

int	IGameExporter::DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts, DWORD options)
{

	Interface * ip = GetCOREInterface();

	IVisExporter * pIgame = GetVisExporter();
	pIgame->Initialize(i);
	HRESULT hr_coinit = CoInitialize(NULL); 
	// Set a global prompt display switch
	showPrompts = suppressPrompts ? FALSE : TRUE;

	exportSelected = (options & SCENE_EXPORT_SELECTED) ? true : false;

/*/
	cm->SetCoordSystem(IGameConversionManager::IGAME_USER);
	UserCoord    uc;
	uc.rotation=0;
	uc.uAxis=1;
	uc.vAxis=1;

	uc.xAxis=1;
	uc.yAxis=4;
	uc.zAxis=2;
	cm->SetUserCoordSystem(uc);
/**/
//	pIgame->SetPropertyFile(_T("hello world"));


	ShowConsole(m_dlg.m_hWnd);
	pRootExport=new RootExport;
	pRootExport->Init(pIgame);
	string backup_name = name;
	backup_name += ".bak";
	CopyFile(name,backup_name.c_str(),false);
	pRootExport->LoadData(name);

	int id=IDOK;

	if(showPrompts) 
	{
		//CMainDlg dlg;
		id=m_dlg.DoModal();
		if(id == IDCANCEL)
		{
			delete pRootExport;
			pRootExport = NULL;
			return 1;
		}
	}

	try
	{
		if(id==ID_EXPORT_LOD1 || id==ID_EXPORT_LOD2)
		{
			pRootExport->ExportLOD(name,id==ID_EXPORT_LOD1?1:2);
		}else
		{
			pRootExport->Export(name);
		}
	}
	catch(GameExporterError error)
	{
	}

	ShowConsole(NULL);

	delete pRootExport;
	pRootExport=NULL;
	DeleteFile(backup_name.c_str());
	if(hr_coinit==S_OK)
		CoUninitialize();
	return TRUE;
}
