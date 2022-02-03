/*

V 4,11
TODO:
Code Cleanup

prerequisites:
build opencv with cuda tbb mkl support
iDS camera and drivers
NI cDAQ device and drivers

*/                                                                         

#include "stdafx.h"
#include "IdsMOCA.h"
#include "IdsMOCADlg.h"
#include ".\idsMOCAdlg.h"
#include "opencv2/version.h"

#include <process.h>
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/video.hpp>
#include <opencv2/cudaoptflow.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudaimgproc.hpp>

#include <cmath>

#include "tbb/task_group.h"
#include <opencv2/NIDAQmx.h>

using namespace tbb;
using namespace cv;
using namespace cv::cuda;

////////////////////////////////////////////////////////////////
//Global variables AOI, OpenCV, NI, TBB
////////////////////////////////////////////////////////////////

INT m_nSizeX;
INT m_nSizeY;
IS_SIZE_2D imageSize;
IS_RECT rectAOI;
Rect2d r;

Mat img0, roi0, roi1;
UMat mag, angle, flowx, flowy;
GpuMat gpuimg0, gpuimg1, gflow, upsampledFlowXY;
GpuMat planes[2];
Stream stream;

Scalar avg;
float64     data0[1] = { 0.0 };
TaskHandle	taskHandle = 0;

task_group a;

////////////////////////////////////////////////////////////////
#define  UPDATE_FRAMERATE WM_USER+1
extern CIdsMOCAApp theApp;
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//global list of the dialog items
//used for easy dynamic positioning
INT g_nDlgItemsID[] = {
IDC_BUTTON_START,
IDC_BUTTON_STOP,
IDC_BUTTON_Analyze,
IDC_BUTTON_EXIT,
IDC_STATIC_EXPOSURE,
IDC_SLIDER_EXPOSURE,
IDC_SLIDER_FPS,
IDC_STATIC_FPS,
IDC_STATIC_GAIN,
IDC_SLIDER_GAIN,
IDC_STATIC_Temp,
};

INT g_nDlgItemsCount = 8;


void ImageProcessing(void* pParam);

#define IDS_LIVE_MODE 1
#define IDS_ANALZYE_MODE 2
HANDLE g_hFrameEvent;
HANDLE g_hCloseThread;
HANDLE g_hExitThread;

CIdsMOCADlg::CIdsMOCADlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIdsMOCADlg::IDD, pParent)
	, m_cnNumberOfSeqMemory(3)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CIdsMOCADlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER_EXPOSURE, m_ctrlSliderExposure);
	DDX_Control(pDX, IDC_SLIDER_GAIN, m_ctrlSliderGain);
	DDX_Control(pDX, IDC_SLIDER_FPS, m_ctrlSliderFPS);

	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CIdsMOCADlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START, OnButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_STOP, OnButtonStop)
	ON_BN_CLICKED(IDC_BUTTON_EXIT, OnButtonExit)
	ON_MESSAGE(UPDATE_FRAMERATE, OnUpdateFramerate)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_Analyze, OnButtonAnalyze)
	ON_WM_HSCROLL()
	ON_STN_CLICKED(IDC_STATIC_FPS, &CIdsMOCADlg::OnStnClickedStaticFps)
	ON_STN_CLICKED(IDC_STATIC_EXPOSURE, &CIdsMOCADlg::OnStnClickedStaticExposure)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIdsMOCADlg message handlers
/////////////////////////////////////////////////////////////////////////////


BOOL CIdsMOCADlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetIcon(m_hIcon, TRUE);
	m_vecMemory.reserve(m_cnNumberOfSeqMemory);
	m_vecMemory.resize(m_cnNumberOfSeqMemory);
	m_hCam = 0;
	m_nRenderMode = IS_RENDER_FIT_TO_WINDOW;
	m_lMemoryId = 0;
	m_bThreadIsRunning = FALSE;
	m_nLastMode = 0;
	g_hCloseThread = CreateEvent(NULL, FALSE, FALSE, L"CloseThread");
	g_hExitThread = CreateEvent(NULL, FALSE, FALSE, L"ExitThread");
	m_hWndDisplay = GetDlgItem(IDS_DISPLAY)->m_hWnd; 
	SetWindowText(L"MOCA GUI 4,11");
	RepositionDialogItems();
	m_bFirstTime = TRUE;
	m_strPath = "-1";
	return true;
}

void CIdsMOCADlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialog::OnSysCommand(nID, lParam);
}

///////////////////////////////////////////////////////////////////////////////
// METHOD: roundup()
// DESCRIPTION: round up height and width selected using mousetracker api from opencv to adapt it to be used for uEye API (devisable by factor of 8)
///////////////////////////////////////////////////////////////////////////////

int roundup(int num, int factor)
{
	return num + factor - 1 - (num - 1) % factor;
}

void CIdsMOCADlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 1;
		int y = (rect.Height() - cyIcon + 1) / 1;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}

	if (m_nLastMode == IDS_LIVE_MODE)
		DisplayuEyeImage(m_lMemoryId, IS_RENDER_NORMAL);

	if (m_nLastMode == IDS_ANALZYE_MODE)
		DisplayuEyeImage(m_lMemoryId, IS_RENDER_FIT_TO_WINDOW);
}
HCURSOR CIdsMOCADlg::OnQueryDragIcon()
{
	return (HCURSOR)m_hIcon;
}

///////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::OnButtonStart()
// DESCRIPTION: start live display and open extra window for 1x1 scale feed
///////////////////////////////////////////////////////////////////////////////
void CIdsMOCADlg::OnButtonStart()
{
	INT nRet = IS_SUCCESS;
	//TODOs if its open, reset , reinit and start from here again
	if (m_hCam == 0)
		OpenCamera();
	if (m_hCam != 0)
	{
		ClearSequence();
		InitSequence();
		nRet = is_CaptureVideo(m_hCam, IS_WAIT);
		if (!m_bThreadIsRunning)
			StartThread();
		GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_Analyze)->EnableWindow(FALSE);
		m_nLastMode = IDS_LIVE_MODE;
	}
}

///////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::OnButtonStop()
// DESCRIPTION: stop live display and return immediately
///////////////////////////////////////////////////////////////////////////////
void CIdsMOCADlg::OnButtonStop()
{
	StopLiveVideo(IS_FORCE_VIDEO_STOP);
	cv::destroyAllWindows();
	DAQmxWriteAnalogF64(taskHandle, 1, 1, 0, DAQmx_Val_GroupByChannel, data0, NULL, NULL);
	DAQmxStopTask(taskHandle);
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_Analyze)->EnableWindow(TRUE);
	
}

///////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::OnButtonExit()
// DESCRIPTION: - stop live display
///////////////////////////////////////////////////////////////////////////////
void CIdsMOCADlg::OnButtonExit()
{
	ExitCamera();
	cv::destroyAllWindows();
	PostQuitMessage(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::OpenCamera()  for live feed
// DESCRIPTION: - Opens a handle to a connected camera, pixel clock, FPS, and exposure
///////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CIdsMOCADlg::OpenCamera()
{
	INT nRet = IS_NO_SUCCESS;
	CString str;

	ExitCamera();

	m_hCam = (HIDS)0;
	nRet = InitCamera(&m_hCam, NULL);
	if (nRet == IS_SUCCESS)
	{
		//IS_SIZE_2D imageSize; Set max width & height
		nRet = is_AOI(m_hCam, IS_AOI_IMAGE_GET_SIZE, (void*)&imageSize, sizeof(imageSize));
		m_nSizeX = imageSize.s32Width;
		m_nSizeY = imageSize.s32Height;
		//set the maximum pixel clock
		UINT nRange[3];
		ZeroMemory(nRange, sizeof(nRange));
		// Get pixel clock range
		nRet = is_PixelClock(m_hCam, IS_PIXELCLOCK_CMD_GET_RANGE, (void*)nRange, sizeof(nRange));
		if (nRet == IS_SUCCESS)
		{
			//UINT nMin = nRange[0];
			UINT nMax = nRange[1];
			//UINT nInc = nRange[2];
			nRet = is_PixelClock(m_hCam, IS_PIXELCLOCK_CMD_SET, (void*)&nMax, sizeof(nMax));
			if (nRet == !IS_SUCCESS)
			{
				AfxMessageBox(TEXT("Setting Pixel clock to Max failed!"), rectAOI.s32Width);
			}
		}
		double dMin, dMax, dInt, dDummy;
		nRet = is_GetFrameTimeRange(m_hCam, &dMin, &dMax, &dInt);
		if (nRet == IS_SUCCESS)
		{
			nRet = is_SetFrameRate(m_hCam, 1 / dMin, &dDummy);
			//initialize the fps slider
			m_ctrlSliderFPS.SetRange((1 / dMax), (1 / dMin));
			m_ctrlSliderFPS.SetPos((1 / dMin));
			//str.Format(L"FPS: ", (1 / dMin)); //SetDlgItemText(IDC_STATIC_FPS, str);
			if (nRet == !IS_SUCCESS)
			{
				AfxMessageBox(TEXT("Setting FPS to Max failed!"), rectAOI.s32Width);
			}
		}
		//set the maximum exposure time for the first time
		double dblRange[3];
		nRet = is_Exposure(m_hCam, IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE, (void*)dblRange, sizeof(dblRange));
		dMin = dblRange[0];
		dMax = dblRange[1];
		dInt = dblRange[2];
		nRet = is_Exposure(m_hCam, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&dMax, sizeof(dMax));
		if (nRet == !IS_SUCCESS)
		{
			AfxMessageBox(TEXT("Setting Exposure to Max"), rectAOI.s32Width);
		}
		//and remember the value for later use
		nRet = is_Exposure(m_hCam, IS_EXPOSURE_CMD_GET_EXPOSURE, (void*)&m_ExposureTime, sizeof(m_ExposureTime));
		//initialize the exposure slider
		m_ctrlSliderExposure.SetRange((INT)(dMin * 1000), (INT)(dMax * 1000));
		m_ctrlSliderExposure.SetPos((INT)(dMax * 1000));
		str.Format(L"Exposure time: %.2fms", m_ExposureTime);
		SetDlgItemText(IDC_STATIC_EXPOSURE, str);
		//initialize the gain slider
		nRet = is_SetHardwareGain(m_hCam, 50, -1, -1, -1);
		m_ctrlSliderGain.SetRange(0, 100);
		m_ctrlSliderGain.SetPos(50);
		str.Format(L"Gain: %d", 50);
		SetDlgItemText(IDC_STATIC_GAIN, str);
		//retrieve the sensor information
		nRet = is_GetSensorInfo(m_hCam, &m_sInfo);
		UpdateData(TRUE);
		//Initialize the memory
		nRet = InitImageMemorys();
		//position all the dialog items dynamically
		RepositionDialogItems();
		if (nRet == IS_SUCCESS)
		{
			//Enable the frame event for the imaging thread
			g_hFrameEvent = CreateEvent(NULL, FALSE, TRUE, L"FrameEvent");
			nRet = is_InitEvent(m_hCam, g_hFrameEvent, IS_SET_EVENT_FRAME);
			nRet = is_EnableEvent(m_hCam, IS_SET_EVENT_FRAME);
		}
		else
			AfxMessageBox(TEXT("Initializing the memory failed!"), MB_ICONWARNING);
		return true;
	}
	else
	{
		AfxMessageBox(L"No uEye camera could be opened!", MB_ICONWARNING);
		return false;
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::OpenCameraAOI()  for analysis feed
// DESCRIPTION: - Opens a handle to a connected camera and set AOI (rounded), pixel clock, FPS, and exposure
///////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CIdsMOCADlg::OpenCameraAOI()
{
	INT nRet = IS_NO_SUCCESS;
	CString str;
	ExitCamera();
	m_hCam = (HIDS)0;
	nRet = InitCamera(&m_hCam, NULL);
	if (nRet == IS_SUCCESS)
	{
		rectAOI.s32X = r.x;  
		rectAOI.s32Y = r.y; 
		rectAOI.s32Width = m_nSizeX = r.width;
		rectAOI.s32Height = m_nSizeY = r.height;
		std::cout << "Rounded AOI  : " << r.x << ", " << r.y << ", " << m_nSizeY << ", " << m_nSizeX << "\n";
		nRet = is_AOI(m_hCam, IS_AOI_IMAGE_SET_AOI, (void*)&rectAOI, sizeof(rectAOI));
		if (nRet == !IS_SUCCESS)
		{
			AfxMessageBox(TEXT("Setting AOI failed!"));
		}
		//maximum pixel clock for the AOI
		UINT nRange[3];
		ZeroMemory(nRange, sizeof(nRange));
		nRet = is_PixelClock(m_hCam, IS_PIXELCLOCK_CMD_GET_RANGE, (void*)nRange, sizeof(nRange));
		if (nRet == IS_SUCCESS)
		{
			// UINT nMin = nRange[0];
			UINT nMax = nRange[1];
			// UINT nInc = nRange[2];
			nRet = is_PixelClock(m_hCam, IS_PIXELCLOCK_CMD_SET, (void*)&nMax, sizeof(nMax));
			if (nRet == !IS_SUCCESS)
			{
				AfxMessageBox(TEXT("Setting Pixel clock to Max failed!"));
			}
		}
		double dMin, dMax, dInt, dDummy;
		nRet = is_GetFrameTimeRange(m_hCam, &dMin, &dMax, &dInt);
		if (nRet == IS_SUCCESS)
		{
			nRet = is_SetFrameRate(m_hCam, 90, &dDummy);
			//re-initialize fps slider for the AOI
			m_ctrlSliderFPS.SetRange((1 / dMax), (1 / dMin));
			m_ctrlSliderFPS.SetPos(90);
			if (nRet == !IS_SUCCESS)
			{
				AfxMessageBox(TEXT("Setting FPS to Max failed!"));
			}
		}
		//set the maximum exposure for the AOI
		double dblRange[3];
		nRet = is_Exposure(m_hCam, IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE, (void*)dblRange, sizeof(dblRange));
		dMin = dblRange[0];
		dMax = dblRange[1];
		dInt = dblRange[2];
		nRet = is_Exposure(m_hCam, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&dMax, sizeof(dMax));
		if (nRet == !IS_SUCCESS)
		{
			AfxMessageBox(TEXT("Setting Exposure to Max"));
		}
		nRet = is_HotPixel(m_hCam, IS_HOTPIXEL_ENABLE_SENSOR_CORRECTION, NULL, NULL);
		UINT nEdgeEnhancement = 7;
		nRet = is_EdgeEnhancement(m_hCam, IS_EDGE_ENHANCEMENT_CMD_SET, (void*)&nEdgeEnhancement, sizeof(nEdgeEnhancement));
		if (nRet == !IS_SUCCESS)
		{
			AfxMessageBox(TEXT("Setting Edge Enhancement failed"));
		}
		nRet = is_GetSensorInfo(m_hCam, &m_sInfo);
		UpdateData(TRUE);
		nRet = InitImageMemorys();
		RepositionDialogItems();
		DAQmxCreateTask("ROI1_AO", &taskHandle);
		DAQmxCreateAOVoltageChan(taskHandle, "cDAQ1Mod1/ao0", "ROI1", -10.0, 10.0, DAQmx_Val_Volts, "");
		DAQmxStartTask(taskHandle);
		if (nRet == IS_SUCCESS)
		{
			g_hFrameEvent = CreateEvent(NULL, FALSE, TRUE, L"FrameEvent");
			nRet = is_InitEvent(m_hCam, g_hFrameEvent, IS_SET_EVENT_FRAME);
			nRet = is_EnableEvent(m_hCam, IS_SET_EVENT_FRAME);
		}
		else
			AfxMessageBox(TEXT("Initializing the memory failed!"), MB_ICONWARNING);
		return true;
	}
	else
	{
		AfxMessageBox(L"No uEye camera could be opened!", MB_ICONWARNING);
		return false;
	}
}
///////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::ExitCamera()
// DESCRIPTION: - exits the instance of the camera
///////////////////////////////////////////////////////////////////////////////
void CIdsMOCADlg::ExitCamera()
{
	if (m_hCam != 0)
	{
		StopLiveVideo(IS_WAIT);
		ClearSequence();
		FreeImageMems();
		is_ExitCamera(m_hCam);
		m_hCam = NULL;
	}
}
///////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::InitImageMemorys()
// DESCRIPTION: - initializes the memory and set the color mode to MONO8
///////////////////////////////////////////////////////////////////////////////
INT CIdsMOCADlg::InitImageMemorys()
{
	INT nRet = IS_NO_SUCCESS;
	if (m_hCam == NULL)
		return IS_NO_SUCCESS;
	ClearSequence();
	FreeImageMems();
	m_nColorMode = IS_CM_MONO8;
	m_nBitsPerPixel = 8;
	nRet = is_SetDisplayMode(m_hCam, IS_SET_DM_OPENGL);  
	nRet = AllocImageMems();
	if (nRet == IS_SUCCESS)
	{
		InitSequence();
		is_SetColorMode(m_hCam, m_nColorMode);
	}
	else
	{
		AfxMessageBox(TEXT("Memory allocation failed!"), MB_ICONWARNING);
	}
	return nRet;
}
///////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::OnClose()
// DESCRIPTION: - exits the application
///////////////////////////////////////////////////////////////////////////////
void CIdsMOCADlg::OnClose()
{
	//exit the camera
	ExitCamera();
	CDialog::OnClose();
}
///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION: - sets the MOCA video dynamically on the right side of the dialog
///////////////////////////////////////////////////////////////////////////////
void CIdsMOCADlg::RepositionDialogItems()
{
	CRect rect, rect2;
	INT nOffsetY = 0;
	INT nOffsetX = 0;
	GetDlgItem(IDS_DISPLAY)->GetWindowRect(&rect);
	ScreenToClient(&rect);
	rect.top = 10;
	rect.left = 10;
	rect.right = rect.left + 720; 
	rect.bottom = rect.top + 620; 
	GetDlgItem(IDS_DISPLAY)->MoveWindow(rect);

}

///////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::DisplayuAOIImage()
///////////////////////////////////////////////////////////////////////////////
void CIdsMOCADlg::DisplayuEyeImage(INT nMemoryID, INT nRenderMode)
{
	INT nRet = IS_SUCCESS;
	nRet = is_RenderBitmap(m_hCam, nMemoryID, m_hWndDisplay, nRenderMode);
}
///////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::OnButtonAnalyze()
// DESCRIPTION: - Run the selected AOI through the analyze function
///////////////////////////////////////////////////////////////////////////////
void CIdsMOCADlg::OnButtonAnalyze()
{
	INT nRet = IS_SUCCESS;
	if (m_hCam == 0)
		OpenCameraAOI();
	if (m_hCam != 0)
	{
		OpenCameraAOI();
		InitSequence();
		nRet = is_CaptureVideo(m_hCam, IS_WAIT);
		if (!m_bThreadIsRunning)
			StartOFThread();
		GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_Analyze)->EnableWindow(FALSE);
		m_nLastMode = IDS_ANALZYE_MODE;
	}
}
///////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::OnUpdateFramerate()
///////////////////////////////////////////////////////////////////////////////
afx_msg LRESULT CIdsMOCADlg::OnUpdateFramerate(WPARAM wParam, LPARAM lParam)
{
	CString strFramerate;
	strFramerate.Format(L"Current frame rate: %.2f fps", (double)(wParam) / 1000.0);
	SetDlgItemText(IDC_STATIC_FPS, strFramerate);
	return 1;
}
///////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::OnHScroll()
// DESCRIPTION: - function for sliders control
///////////////////////////////////////////////////////////////////////////////
void CIdsMOCADlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	INT nValue = 0;
	CString str;
	double dDummy;
	if (m_hCam)
	{
		if (((CSliderCtrl*)pScrollBar == &m_ctrlSliderExposure) || ((CSliderCtrl*)pScrollBar == &m_ctrlSliderFPS))
		{

			nValue = m_ctrlSliderExposure.GetPos();
			m_ExposureTime = nValue / 1000.0;
			is_Exposure(m_hCam, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&m_ExposureTime, sizeof(m_ExposureTime));
			str.Format(L"Exposure time: %.2fms", m_ExposureTime);
			SetDlgItemText(IDC_STATIC_EXPOSURE, str);
			nValue = m_ctrlSliderFPS.GetPos();  //FPS slider
			is_SetFrameRate(m_hCam, double(nValue), &dDummy);
			str.Format(L"Frame rate set to: %d", nValue);
			SetDlgItemText(IDC_STATIC_FPS, str);
		}
		else if ((CSliderCtrl*)pScrollBar == &m_ctrlSliderGain)
		{
			//get the position and set the hardware gain
			nValue = m_ctrlSliderGain.GetPos();
			is_SetHardwareGain(m_hCam, nValue, -1, -1, -1);
			str.Format(L"Gain: %d", nValue);
			SetDlgItemText(IDC_STATIC_GAIN, str);
		}
	}
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

///////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::StartThread()
// DESCRIPTION: - used to start the thread for image processing
///////////////////////////////////////////////////////////////////////////////
void CIdsMOCADlg::StartThread()
{
	_beginthread(ImageProcessing, 0, (void*)this);
}

void CIdsMOCADlg::StartOFThread()
{
	_beginthread(OFProcessing, 0, (void*)this);
}

///////////////////////////////////////////////////////////////////////////////
// METHODChangeProcessPriority()
// DESCRIPTION: - sets the priority of the process to high, expect it to improve performance
///////////////////////////////////////////////////////////////////////////////
void ChangeProcessPriority()
{
	HANDLE hProcess = GetCurrentProcess();
	SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);
}

///////////////////////////////////////////////////////////////////////////////
// METHOD CIdsMOCADlg::StopThread()
// DESCRIPTION: - sets the event for stopping the thread and waits for his exit
///////////////////////////////////////////////////////////////////////////////
void CIdsMOCADlg::StopThread(BOOL bWait)
{
	SetEvent(g_hCloseThread);
	if (bWait)
	{
		WaitForSingleObject(g_hExitThread, 1000);
	}
}
///////////////////////////////////////////////////////////////////////////////
// METHOD ImageProcessing()
// DESCRIPTION: - thread for inital live feed to select single AOI and round it
///////////////////////////////////////////////////////////////////////////////
void ImageProcessing(void* pParam)
{
	CIdsMOCADlg* dlg = (CIdsMOCADlg*)pParam;
	dlg->m_bThreadIsRunning = TRUE;
	HANDLE Handles[2];
	Handles[0] = g_hFrameEvent;
	Handles[1] = g_hCloseThread;
	DWORD nRet = 0;
	INT nuEyeRet = IS_SUCCESS;
	INT nFrameCounter = 0;
	INT nFrameRate = 0;
	LARGE_INTEGER liCounterStart, liCounterStop, liFrequency;
	double dFrequency, dTime, dTimePerFrame;
	QueryPerformanceFrequency(&liFrequency);
	dFrequency = (double)liFrequency.QuadPart;
	QueryPerformanceCounter(&liCounterStart);
	do
	{
		nRet = WaitForMultipleObjects(2, Handles, FALSE, INFINITE);
		if (nRet == WAIT_OBJECT_0)
		{
			char* pLast = NULL;
			INT lMemoryId = 0;
			INT lSequenceId = 0;
			if (IS_SUCCESS == dlg->GetLastMem(&pLast, lMemoryId, lSequenceId))
			{
				nuEyeRet = is_LockSeqBuf(dlg->m_hCam, IS_IGNORE_PARAMETER, pLast);
				if (IS_SUCCESS == nuEyeRet)
				{
					img0 = cv::Mat(m_nSizeY, m_nSizeX, CV_8UC1, pLast); //   CV_8UC1 
					cv::imshow("Live Video. Press ESC to select ROI", img0);
					char k = (char)waitKey(1);
					if (k == 27)
					{
						bool fromCenter = false;
						r = selectROI(img0, fromCenter);
						
						std::cout << "Selected AOI : " << r.x << ", " << r.y << ", " << r.height << ", " << r.width << "\n"; 
						m_nSizeX = r.width = roundup(r.width, 8);
						m_nSizeY = r.height = roundup(r.height, 8);
						roi0 = img0(r);
						gpuimg0.upload(roi0);
						dlg->OnButtonStop();
					}
					nFrameCounter++;
					if (nFrameCounter == 1600) 
					{
						QueryPerformanceCounter(&liCounterStop);
						dTime = (liCounterStop.QuadPart - liCounterStart.QuadPart) / dFrequency;
						dTimePerFrame = dTime / 1800;
						nFrameRate = (INT)(1000.0 / dTimePerFrame);
						PostMessage(dlg->m_hWnd, UPDATE_FRAMERATE, nFrameRate, 0);
						nFrameCounter = 0;
						QueryPerformanceCounter(&liCounterStart);
					}
					is_UnlockSeqBuf(dlg->m_hCam, IS_IGNORE_PARAMETER, pLast);
				}
			}
		}
	} while (nRet != WAIT_OBJECT_0 + 1);
	dlg->m_bThreadIsRunning = FALSE;
	SetEvent(g_hExitThread);
	return;
}

///////////////////////////////////////////////////////////////////////////////
// METHOD Optical flow Processing()
// DESCRIPTION: - thread for AOI optical flow calc.
///////////////////////////////////////////////////////////////////////////////

void OFProcessing(void* pParam)
{
	ChangeProcessPriority(); 
	CIdsMOCADlg* dlg = (CIdsMOCADlg*)pParam;
	dlg->m_bThreadIsRunning = TRUE;
	HANDLE Handles[2];
	Handles[0] = g_hFrameEvent;
	Handles[1] = g_hCloseThread;

	DWORD nRet = 0;
	INT nuEyeRet = IS_SUCCESS;
	INT nFrameCounter = 0;
	INT nFrameRate = 0;
	Ptr<cuda::FarnebackOpticalFlow> fg_frn = cv::cuda::FarnebackOpticalFlow::create((int)1, (double)0.5, (bool)false, (int)30, (int)3, (int)5, (double)1.1, (int)0);

	LARGE_INTEGER liCounterStart, liCounterStop, liFrequency;
	double dFrequency, dTime, dTimePerFrame;
	QueryPerformanceFrequency(&liFrequency);
	dFrequency = (double)liFrequency.QuadPart;
	QueryPerformanceCounter(&liCounterStart);
	do
	{
		nRet = WaitForMultipleObjects(2, Handles, FALSE, INFINITE);
		if (nRet == WAIT_OBJECT_0)
		{
			char* pLast = NULL;
			INT lMemoryId = 0;
			INT lSequenceId = 0;
			if (IS_SUCCESS == dlg->GetLastMem(&pLast, lMemoryId, lSequenceId))
			{
				nuEyeRet = is_LockSeqBuf(dlg->m_hCam, IS_IGNORE_PARAMETER, pLast);
				roi1 = cv::Mat(m_nSizeY, m_nSizeX, CV_8UC1, pLast); //   CV_8UC1  
				gpuimg1.upload(roi1);
				fg_frn->calc(gpuimg0, gpuimg1, gflow, stream); 
				cuda::split(gflow, planes, stream);
				planes[0].download(flowx);  
				planes[1].download(flowy); 
				cv::cartToPolar(flowx, flowy, mag, angle, false);
				a.run([&] {avg = cv::mean(mag) * 7; });
				a.run([&] {DAQmxWriteAnalogScalarF64(taskHandle, 1, 0, avg[0], NULL); }); 
				a.wait(); 
				cuda::swap(gpuimg0, gpuimg1);                
				if (IS_SUCCESS == nuEyeRet)
				{
					dlg->DisplayuEyeImage(lMemoryId, IS_RENDER_NORMAL);
					nFrameCounter++;
					if (nFrameCounter == 5000)
					{
						QueryPerformanceCounter(&liCounterStop);
						dTime = (liCounterStop.QuadPart - liCounterStart.QuadPart) / dFrequency;
						dTimePerFrame = dTime / nFrameCounter;
						nFrameRate = (INT)(1000.0 / dTimePerFrame);
						PostMessage(dlg->m_hWnd, UPDATE_FRAMERATE, nFrameRate, 0);
						nFrameCounter = 0;
						QueryPerformanceCounter(&liCounterStart);
					}
					is_UnlockSeqBuf(dlg->m_hCam, IS_IGNORE_PARAMETER, pLast);
				}
			}
		}
	}
	while (nRet != WAIT_OBJECT_0 + 1);
	dlg->m_bThreadIsRunning = FALSE;
	SetEvent(g_hExitThread);
	return;
}

INT CIdsMOCADlg::InitCamera(HIDS* hCam, HWND hWnd)
{
	INT nRet = is_InitCamera(hCam, hWnd);
	if (nRet == IS_STARTER_FW_UPLOAD_NEEDED)
	{
		INT nUploadTime = 25000;
		is_GetDuration(*hCam, IS_STARTER_FW_UPLOAD, &nUploadTime);
		CString Str1, Str2, Str3;
		Str1 = "This camera requires a new firmware. The upload will take about";
		Str2 = "seconds. Please wait ...";
		Str3.Format(L"%s %d %s", Str1, nUploadTime / 1000, Str2);
		AfxMessageBox(Str3, MB_ICONWARNING);
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));
		*hCam = (HIDS)(((INT)*hCam) | IS_ALLOW_STARTER_FW_UPLOAD);
		nRet = is_InitCamera(hCam, hWnd);
	}
	return nRet;
}

void CIdsMOCADlg::StopLiveVideo(INT nWait)
{
	if (0 != m_hCam)
	{
		is_StopLiveVideo(m_hCam, nWait);
	}

	if (m_bThreadIsRunning)
	{
		StopThread(TRUE);
	}

	if (0 != m_hCam)
	{
		ClearSequence();
	}
}

INT CIdsMOCADlg::AllocImageMems(void)
{
	INT nRet = IS_SUCCESS;
	do
	{
		for (MemoryVector::iterator iter = m_vecMemory.begin(); iter != m_vecMemory.end(); ++iter)
		{
			nRet = is_AllocImageMem(m_hCam,
				m_nSizeX,
				m_nSizeY,
				m_nBitsPerPixel,
				&(iter->pcImageMemory),
				&(iter->lMemoryId));

			if (IS_SUCCESS != nRet)
			{
				break;
			}
		}
		break;
	} while (true);
	if (IS_SUCCESS != nRet)
	{
		FreeImageMems();
	}
	return nRet;
}
INT CIdsMOCADlg::FreeImageMems(void)
{
	INT nRet = IS_SUCCESS;
	for (MemoryVector::iterator iter = m_vecMemory.begin(); iter != m_vecMemory.end(); ++iter)
	{
		if (NULL != iter->pcImageMemory)
		{
			do
			{
				nRet = is_FreeImageMem(m_hCam, iter->pcImageMemory, iter->lMemoryId);

				if (IS_SEQ_BUFFER_IS_LOCKED == nRet)
				{
					::Sleep(1);
					continue;
				}

				break;
			} while (true);
		}
		iter->pcImageMemory = NULL;
		iter->lMemoryId = 0;
		iter->lSequenceId = 0;
	}
	return nRet;
}

INT CIdsMOCADlg::InitSequence(void)
{
	INT nRet = IS_SUCCESS;

	int i = 0;
	for (MemoryVector::iterator iter = m_vecMemory.begin(); iter != m_vecMemory.end(); ++iter, i++)
	{
		nRet = is_AddToSequence(m_hCam, iter->pcImageMemory, iter->lMemoryId);

		if (IS_SUCCESS != nRet)
		{
			ClearSequence();
			break;
		}

		iter->lSequenceId = i + 1;
	}

	return nRet;
}

INT CIdsMOCADlg::ClearSequence(void)
{
	return is_ClearSequence(m_hCam);
}

INT CIdsMOCADlg::GetLastMem(char** ppLastMem, INT& lMemoryId, INT& lSequenceId)
{
	INT nRet = IS_NO_SUCCESS;
	int dummy = 0;
	char* pLast = NULL;
	char* pMem = NULL;
	nRet = is_GetActSeqBuf(m_hCam, &dummy, &pMem, &pLast);
	if ((IS_SUCCESS == nRet) && (NULL != pLast))
	{
		nRet = IS_NO_SUCCESS;

		for (MemoryVector::iterator iter = m_vecMemory.begin(); iter != m_vecMemory.end(); ++iter)
		{
			if (pLast == iter->pcImageMemory)
			{
				*ppLastMem = iter->pcImageMemory;
				lMemoryId = iter->lMemoryId;
				lSequenceId = iter->lSequenceId;
				nRet = IS_SUCCESS;
				break;
			}
		}
	}
	return nRet;
}

void CIdsMOCADlg::OnStnClickedStaticTemp()
{
}

void CIdsMOCADlg::OnStnClickedStaticFps()
{
}

void CIdsMOCADlg::OnStnClickedStaticExposure()
{
}


