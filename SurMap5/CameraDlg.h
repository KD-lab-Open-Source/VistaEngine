#ifndef __CAMERA_DLG_H_INCLUDED__
#define __CAMERA_DLG_H_INCLUDED__


// CCameraDlg dialog

class CCameraDlg : public CDialog
{
	DECLARE_DYNAMIC(CCameraDlg)
	
public:
	CCameraDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCameraDlg();

// Dialog Data
	enum { IDD = IDD_DLG_CAMERA };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:

	enum {
		NONE,
		CREATE_POINTS,
		SELECT_POINTS,
		EDIT_POINTS,
	} mouseMode;

	CButton createCameraButton;
	CButton deleteCameraButton;
	CButton playCameraButton;
	CButton deletePointButton;
	CButton setCPButton;
	CButton setPCButton;
	CListBox CameraList;

	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();
	afx_msg void OnLbnSelchangeList1();


	void OnMapLBClick(const Vect3f& coord);
	void OnMapRBClick(const Vect3f& coord);

	void RefreshList();
	void EnablePointsButtons(); // Группа кнопок для работы с точками.
	void DisablePointsButtons();
	void EnablePropGroup(); // Группа свойств камеры.
	void DisablePropGroup();
	void setSplineProp(); // Вывести на контролы св-ва сплайна.
	void getSplineProp(); // Получить с контролов св-ва сплайна.

	virtual BOOL OnInitDialog();

	int time;
	CString name;
	BOOL cycling;

	afx_msg void OnEnChangeEdit1();
	afx_msg void getSpl();
	afx_msg void OnBnClickedCheck1();
	CButton addPoint;
	afx_msg void OnBnClickedButton7();
};

#endif
