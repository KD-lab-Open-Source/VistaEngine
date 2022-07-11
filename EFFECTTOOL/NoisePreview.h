#pragma once


// CNoisePreview

class CNoisePreview : public CWnd
{
	DECLARE_DYNAMIC(CNoisePreview)

public:
	CNoisePreview();
	virtual ~CNoisePreview();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	vector<float> m_points;
};


