// IdsRotationDemo.h : main header file for the IDSRotationDemo application
//

#if !defined(AFX_IDSMOCA_H__3066A7A2_C7D9_4E0E_AF60_510BE2961B11__INCLUDED_)
#define AFX_IDSMOCA_H__3066A7A2_C7D9_4E0E_AF60_510BE2961B11__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CIdsMOCAApp:
// See IdsRotationDemo.cpp for the implementation of this class
//

class CIdsMOCAApp : public CWinApp
{
public:
	CIdsMOCAApp();

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CIdsMOCAApp)
public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CIdsMOCAApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IDSRotationDemo_H__3066A7A2_C7D9_4E0E_AF60_510BE2961B11__INCLUDED_)
