#include "stdafx.h"
#include "IdsMOCA.h"
#include "IdsMOCADlg.h"
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CIdsMOCAApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

CIdsMOCAApp::CIdsMOCAApp()
{
}

CIdsMOCAApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CIdsMOCAApp and console initialization
/////////////////////////////////////////////////////////////////////////////

void Console()
{
	AllocConsole();
	FILE* pFileCon = NULL;
	pFileCon = freopen("CONOUT$", "w", stdout);
	COORD coordInfo;
	coordInfo.X = 50;
	coordInfo.Y = 50;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coordInfo);
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS);
}

BOOL CIdsMOCAApp::InitInstance()
{
	InitCommonControls();
	Console(); //console for debugging
	std::cout << "MOCA iDS, OpenCV CUDA 4,11 debugging output:" << "\n";
	std::cout << "change log:" << "\n";
	std::cout << "Reverted to version 4.7, and set default max FPS to 90 fps" << "\n";
	std::cout << "WinSize set to 30, FASTWIN False" << "\n";
	std::cout << "Past" << "\n";
	std::cout << "Added FPS slider, WinSize set to 15" << "\n";
	std::cout << "Set default max FPS to 203 fps" << "\n";
	std::cout << "Gain default value set to 50, Enabled edge enhancment, Enabled hotpixel correction" << "\n";

	AfxEnableControlContainer();
	SetRegistryKey(_T("MOCA\\MOCA_GUI"));
	WriteProfileString(_T("MRU"), _T("Test"), theApp.m_pszAppName);

	CIdsMOCADlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}

	return FALSE;
}

//===========================================================================//
//                                                                           //
//  Copyright (C) 2004 - 2017                                                //
//  IDS Imaging GmbH                                                         //
//  Dimbacherstr. 6-8                                                        //
//  D-74182 Obersulm-Willsbach                                               //
//                                                                           //
//  The information in this document is subject to change without            //
//  notice and should not be construed as a commitment by IDS Imaging GmbH.  //
//  IDS Imaging GmbH does not assume any responsibility for any errors       //
//  that may appear in this document.                                        //
//                                                                           //
//  This document, or source code, is provided solely as an example          //
//  of how to utilize IDS software libraries in a sample application.        //
//  IDS Imaging GmbH does not assume any responsibility for the use or       //
//  reliability of any portion of this document or the described software.   //
//                                                                           //
//  General permission to copy or modify, but not for profit, is hereby      //
//  granted,  provided that the above copyright notice is included and       //
//  reference made to the fact that reproduction privileges were granted	 //
//	by IDS Imaging GmbH.				                                     //
//                                                                           //
//  IDS cannot assume any responsibility for the use or misuse of any        //
//  portion of this software for other than its intended diagnostic purpose	 //
//  in calibrating and testing IDS manufactured cameras and software.		 //
//                                                                           //
//===========================================================================//

/*
License Agreement
For Open Source Computer Vision Library
(3 - clause BSD License)

Copyright(C) 2000 - 2019, Intel Corporation, all rights reserved.
Copyright(C) 2009 - 2011, Willow Garage Inc., all rights reserved.
Copyright(C) 2009 - 2016, NVIDIA Corporation, all rights reserved.
Copyright(C) 2010 - 2013, Advanced Micro Devices, Inc., all rights reserved.
Copyright(C) 2015 - 2016, OpenCV Foundation, all rights reserved.
Copyright(C) 2015 - 2016, Itseez Inc., all rights reserved.

Third party copyrights are property of their respective owners.

Redistribution and use in sourceand binary forms, with or without modification, are permitted provided that the following conditions are met :

Redistributions of source code must retain the above copyright notice, this list of conditionsand the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditionsand the following disclaimer in the documentationand /or other materials provided with the distribution.
Neither the names of the copyright holders nor the names of the contributors may be used to endorse or promote products derived from this software without specific prior written permission.

This software is provided by the copyright holders and contributors “as is” and
any express or implied warranties, including, but not limited to, the implied warranties of merchantabilityand fitness for
a particular purpose are disclaimed.In no event shall copyright holders or contributors be liable for any direct, indirect,
incidental, special, exemplary, or consequential damages(including, but not limited to, procurement of substitute goods or
services; loss of use, data, or profits; or business interruption) however caused and on any theory of liability, whether
in contract, strict liability, or tort(including negligence or otherwise) arising in any way out of the use of this software,
even if advised of the possibility of such damage.

NI - DAQ\Examples\DAQmx ANSI C code was used for AO

*/