// IdsMOCADlg.h	: header file
//

#if	!defined(AFX_IDSMOCADLG_H__00EFA87C_7A28_4501_8760_D28D4F306A00__INCLUDED_)
#define	AFX_IDSMOCADLG_H__00EFA87C_7A28_4501_8760_D28D4F306A00__INCLUDED_

#if	_MSC_VER > 1000
#pragma	once
#endif // _MSC_VER > 1000

#include "opencv2/uEye.h"
#include <vector>
#include "afxcmn.h"

/////////////////////////////////////////////////////////////////////////////
// CIdsMOCADlg dialog

//memorys used to store the different images
struct Memory
{
	Memory(void)
		: pcImageMemory(NULL)
		, lMemoryId(0)
		, lSequenceId(0)
	{
	}

	char* pcImageMemory;
	INT lMemoryId;
	INT lSequenceId;
};

class CIdsMOCADlg : public CDialog
{
public:

	CIdsMOCADlg(CWnd* pParent = NULL);	// standard	constructor

	enum { IDD = IDD_IDSMOCA_DIALOG };

	//Scalar avg;

private:
	virtual	void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	HICON	m_hIcon;

	// uEye	varibles
	HIDS	m_hCam;			// handle to camera
	INT		m_nColorMode;	// Y8/RGB24
	INT		m_nBitsPerPixel;// number of bits needed store one pixel
	//INT		m_nSizeX;		// width of image
	//INT		m_nSizeY;		// height of image

	SENSORINFO m_sInfo;		// sensor information struct
	INT		m_nRenderMode;	// render mode

	//Definitions used to rotate the original image
	HWND	m_hWndDisplay;	// handle to diplay	window

	typedef std::vector<Memory> MemoryVector;
	MemoryVector m_vecMemory;
	const INT m_cnNumberOfSeqMemory;

	INT		m_lMemoryId;	// camera vertical memory -	buffer ID

	// Path to save the snapshots
	BOOL    m_bFirstTime;
	CString m_strPath;

	//check if the thread is running
	BOOL m_bThreadIsRunning;

	//last mode used IDS_LIVE_MODE/IDS_ANALZYE_MODE
	INT m_nLastMode;

	//used exposure time
	double m_ExposureTime;

	//control variables for the sliders
	CSliderCtrl m_ctrlSliderExposure;
	CSliderCtrl m_ctrlSliderGain;
	CSliderCtrl m_ctrlSliderFPS;

	//functions used to control the camera
	INT InitCamera(HIDS* hCam, HWND hWnd);
	bool OpenCamera();
	bool OpenCameraAOI();
	void ExitCamera();
	INT	 InitImageMemorys();
	void StartThread();
	void StartOFThread();
	void StopThread(BOOL bWait);
	void StopLiveVideo(INT nWait);
	void Console();
	int  roundUp(int numToRound, int multiple);

	//building the dialog
	void RepositionDialogItems();

	//declare the thread function as friend so that it can access all protected/private members of the dialog
	friend void ImageProcessing(void* pParam);
	friend void OFProcessing(void* pParam);

	//function to rotate the given image memory
	INT CopyuEyeMemory(char* pcSource, char* pcDestination, INT nWidth, INT nHeight, INT nBitsPerPixel);

	//used to display the image memory
	void DisplayuEyeImage(INT nMemoryID, INT nRenderMode);

	INT AllocImageMems(void);
	INT FreeImageMems(void);
	INT InitSequence(void);
	INT ClearSequence(void);
	INT GetLastMem(char** ppLastMem, INT& lMemoryId, INT& lSequenceId);

	//message function used to update the framerate from the thread
	afx_msg LRESULT OnUpdateFramerate(WPARAM wParam, LPARAM lParam);

	// Generated message map functions
	virtual	BOOL OnInitDialog();
	afx_msg	void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg	void OnPaint();
	afx_msg	HCURSOR	OnQueryDragIcon();
	afx_msg	void OnButtonStart();
	afx_msg	void OnButtonStop();
	afx_msg	void OnButtonExit();
	afx_msg	void OnButtonAnalyze();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnClose();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnStnClickedStaticWarning();
	afx_msg void OnStnClickedStaticLogo();
	afx_msg void OnStnClickedStaticTemp();
	afx_msg void OnStnClickedStaticFps();
	afx_msg void OnStnClickedStaticExposure();
};

#endif // !defined(AFX_IDSMOCADLG_H__00EFA87C_7A28_4501_8760_D28D4F306A00__INCLUDED_)
