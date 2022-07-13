#ifndef __OUTPUT_PROGRESS_DLG_H_INCLUDED__
#define __OUTPUT_PROGRESS_DLG_H_INCLUDED__

class COutputProgressDlg : public CDialog
{
	DECLARE_DYNAMIC(COutputProgressDlg)

public:
	COutputProgressDlg(const char* title, CWnd* pParent = 0);   // standard constructor
	virtual ~COutputProgressDlg();

	BOOL Create(CWnd* parent);

	void pumpMessages();
	void progress(int totalPercent, int filePercent, const char* fileName);

// Dialog Data
	enum { IDD = IDD_OUTPUT_PROGRESS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	std::string title_;
	CProgressCtrl totalProgress_;
	CProgressCtrl fileProgress_;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

namespace ExportOutputCallback{
	static COutputProgressDlg progressDlg("Packing...");

	static void callback(int totalPercent, int filePercent, const char* currentFileName){
		progressDlg.progress(totalPercent, filePercent, currentFileName);
	}

	static void hide(CWnd* parent){
		progressDlg.DestroyWindow();
		if(parent)
			parent->EnableWindow(TRUE);
	}

	static void show(CWnd* parent){
		progressDlg.Create(parent);
		progressDlg.ShowWindow(SW_SHOW);
		if(parent)
			parent->EnableWindow(FALSE);
		progressDlg.Invalidate(TRUE);
	}
};


#endif