/*
 *
 *  Copyright (C) 2013  Anwar Mohamed <anwarelmakrahy[at]gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to Anwar Mohamed
 *  anwarelmakrahy[at]gmail.com
 *
 */

#include <Windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <tchar.h>
#include <gdiplus.h>
#include "Serial.h"

using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

using namespace std;

DWORD	gPidToFind=0;
HWND    gTargetWindowHwnd = NULL;
HWND	NextHwndParent = NULL;
INT		PosArray[4][3];
HWND	NextWidgetsHwnd[4][3];
CSerial serial;

//BOOL ListProcessThreads( DWORD dwOwnerPID );
//void printError( TCHAR* msg );
BOOL CALLBACK myWNDENUMPROC(HWND hwCurHwnd, LPARAM lpMylp);
VOID ProcessColours();
//PBITMAPINFO CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp);
//void CreateBMPFile(HWND hwnd, LPTSTR pszFile, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC);

int main()
{
	cout << "**************************************************" << endl;
	cout << "*                                                *" << endl;
	cout << "*         Emotiv EPOC Brain Activity Map         *" << endl;
	cout << "*           Serial Port Control Server           *" << endl;
	cout << "*                                                *" << endl;
	cout << "**************************************************" << endl;

	STARTUPINFO startup_info;
	PROCESS_INFORMATION process_info;

	ZeroMemory( &startup_info, sizeof(startup_info) );
	startup_info.cb = sizeof(startup_info);
	ZeroMemory( &process_info, sizeof(process_info) );

	startup_info.dwFlags = STARTF_USESHOWWINDOW;
	startup_info.wShowWindow = SW_MAXIMIZE;

	if (! CreateProcess	(	TEXT("C:\\Program Files (x86)\\Emotiv EPOC Brain Activity Map\\EmoBrainMap.exe"),NULL,
							NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,NULL,&startup_info,&process_info))
	{
		cout << "Unable to execute." << endl;
		system("PAUSE");
		return 0;
	}

	serial.Open(_T("COM1"));
	serial.Setup(CSerial::EBaud9600,CSerial::EData8,CSerial::EParNone,CSerial::EStop1);
	serial.SetupHandshaking(CSerial::EHandshakeHardware);

	//serial.Write("1");
	

	Sleep(3000);

	gPidToFind = process_info.dwProcessId;
	EnumWindows(myWNDENUMPROC, process_info.dwProcessId);
	//ListProcessThreads(gPidToFind);
	
	//_tprintf( TEXT("\n     parent hWnd = %08x\n\n"), gTargetWindowHwnd );

	NextHwndParent = FindWindowEx(gTargetWindowHwnd, NULL, NULL, NULL);
	NextHwndParent = FindWindowEx(gTargetWindowHwnd, NextHwndParent, NULL, NULL);
	HWND WidgetsHwnd [4];
	
	Sleep(100);

	/* getting pictures hwnd */
	for (int i=0; i<4; i++)
	{
		if (i==0)
		{
			WidgetsHwnd[i] = FindWindowEx(NextHwndParent, NULL, NULL, NULL);

			Sleep(100);
			for (int j=0; j<3; j++)
			{
				if (j==0)
					NextWidgetsHwnd[i][j] = FindWindowEx(WidgetsHwnd[i], NULL, NULL, NULL);
				else
					NextWidgetsHwnd[i][j] = FindWindowEx(WidgetsHwnd[i], NextWidgetsHwnd[i][j-1], NULL, NULL);
			}
		}
		else
		{
			WidgetsHwnd[i] = FindWindowEx(NextHwndParent, WidgetsHwnd[i-1], NULL, NULL);

			Sleep(100);
			for (int j=0; j<3; j++)
			{
				if (j==0)
					NextWidgetsHwnd[i][j] = FindWindowEx(WidgetsHwnd[i], NULL, NULL, NULL);
				else
					NextWidgetsHwnd[i][j] = FindWindowEx(WidgetsHwnd[i], NextWidgetsHwnd[i][j-1], NULL, NULL);
			}
		}
	}

	for (int m=0; m<4; m++)
	{
		Sleep(500);
		NextWidgetsHwnd[m][2] = FindWindowEx(NextWidgetsHwnd[m][2], NULL, NULL, NULL);
		//cout << (PDWORD)NextWidgetsHwnd[m][2] << endl;
	}

	/*
	1-	theta
	2-	alpha
	3-	beta
	4-	delta
	*/

	RECT rectangle;
	for (int k=0; k<4; k++)
	{
		//cout << k+1 << endl;

		GetWindowRect(NextWidgetsHwnd[k][2], &rectangle);
		if (GetDeviceCaps( GetDC(NextWidgetsHwnd[k][2]), BITSPIXEL ) == CM_NONE)
		{
			cout << "Unable to GetPixel()." << endl;
			system("PAUSE");
			serial.Close();
			TerminateProcess(process_info.hProcess, 0);
			return 0;
		}

		PosArray[k][0] = rectangle.top;
		//cout << PosArray[k][0] << endl;
		PosArray[k][1] = rectangle.bottom;
		//cout << PosArray[k][1] << endl;
		PosArray[k][2] = rectangle.left;
		//cout << PosArray[k][2] << endl;
		PosArray[k][3] = rectangle.right;
		//cout << PosArray[k][3] << endl << endl;

		/*save capture*/
		/*HDC hDC       = GetDC(NextWidgetsHwnd[k][2]);
		HDC hTargetDC = CreateCompatibleDC( hDC );

		HBITMAP hBitmap = CreateCompatibleBitmap( hDC,	rectangle.right - rectangle.left, 
														rectangle.bottom - rectangle.top );
		SelectObject( hTargetDC, hBitmap );
		
		Sleep(100);
		PrintWindow( NextWidgetsHwnd[k][2], hTargetDC, PW_CLIENTONLY );

		Sleep(100);
		CreateBMPFile( NextWidgetsHwnd[k][2], 
			TEXT("D:\\test.bmp") ,
			CreateBitmapInfoStruct(NextWidgetsHwnd[k][2], hBitmap), 
			hBitmap, hTargetDC);

		DeleteObject( hBitmap );
		ReleaseDC( NextWidgetsHwnd[k][2], hDC );
		DeleteDC( hTargetDC );


		GetClientRect(NextWidgetsHwnd[k][2], &rectangle);
		cout << rectangle.right << endl;
		cout << rectangle.bottom << endl;*/
	}

	if (!serial.IsOpen())
	{
		cout << "Cannot connect to serial port. exiting..." << endl;
		system("PAUSE");

		serial.Close();
		TerminateProcess(process_info.hProcess, 0);
		return 0;
	}

	cout << endl << "Press enter anytime to stop analyzing data and exit" << endl << endl;

	while (!(GetAsyncKeyState(VK_RETURN) & 0x8000))
	//for(int i=0; i<2; i++)
	{
		Sleep(1000);
		ProcessColours();
	}

	system("PAUSE");

	serial.Close();
	TerminateProcess(process_info.hProcess, 0);
	return 0;
}


BOOL CALLBACK myWNDENUMPROC(HWND hwCurHwnd, LPARAM lpMylp)
{
	DWORD pID;
	char szBuffer[MAX_PATH];

	GetWindowText(hwCurHwnd,szBuffer,200);
	DWORD TpID = GetWindowThreadProcessId(hwCurHwnd, &pID);
	if (strcmp(szBuffer, "Emotiv EPOC Brain Activity Map") == 0)
	{
		gTargetWindowHwnd = hwCurHwnd;
		return FALSE;
	}
    return TRUE;
}

VOID ProcessColours()
{
	RECT dimentions;

	memset(&dimentions, 0 , sizeof(RECT));
	GetClientRect(NextWidgetsHwnd[0][2], &dimentions);
	COLORREF colorTheta = GetPixel(	GetDC(NextWidgetsHwnd[0][2]), 
									(dimentions.right/2), 
									(dimentions.bottom/2));

	memset(&dimentions, 0 , sizeof(RECT));
	GetClientRect(NextWidgetsHwnd[1][2], &dimentions);
	COLORREF colorAlpha = GetPixel(	GetDC(NextWidgetsHwnd[1][2]), 
									(dimentions.right/2), 
									(dimentions.bottom/2));

	memset(&dimentions, 0 , sizeof(RECT));
	GetClientRect(NextWidgetsHwnd[2][2], &dimentions);
	COLORREF colorBeta = GetPixel(	GetDC(NextWidgetsHwnd[2][2]), 
									(dimentions.right/2), 
									(dimentions.bottom/2));

	memset(&dimentions, 0 , sizeof(RECT));
	GetClientRect(NextWidgetsHwnd[3][2], &dimentions);
	COLORREF colorDelta = GetPixel(	GetDC(NextWidgetsHwnd[3][2]), 
									(dimentions.right/2), 
									(dimentions.bottom/2));

#define WHITE	0x00ffffff
#define BLACK	0x00000000
#define RED		0x00ff0000
#define BLUE	0x000000ff
#define YELLOW	0x00ffff00
#define GREEN	0x0000ff00

	cout << "\r" << (PDWORD)colorTheta << " " << (PDWORD)colorAlpha << " " << (PDWORD)colorBeta << " " << (PDWORD)colorDelta << flush;

	if (colorBeta == RED)
		serial.Write("1");
	else if (colorBeta == GREEN)
		serial.Write("3");
	else if (colorBeta == YELLOW)
		serial.Write("5");
	else
		serial.Write("0");
}












/*BOOL ListProcessThreads( DWORD dwOwnerPID ) 
{ 
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE; 
	THREADENTRY32 te32; 
  
	hThreadSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 ); 
	if( hThreadSnap == INVALID_HANDLE_VALUE ) 
	return( FALSE ); 
  
	te32.dwSize = sizeof(THREADENTRY32 ); 
 
	if( !Thread32First( hThreadSnap, &te32 ) ) 
	{
		printError( TEXT("Thread32First") );
		CloseHandle( hThreadSnap );
		return( FALSE );
	}

	do 
	{ 
		if( te32.th32OwnerProcessID == dwOwnerPID )
		{
			_tprintf( TEXT("\n     THREAD ID      = 0x%08X"), te32.th32ThreadID ); 
			_tprintf( TEXT("\n     base priority  = %d"), te32.tpBasePri ); 
			_tprintf( TEXT("\n     delta priority = %d"), te32.tpDeltaPri ); 
		}
	} while( Thread32Next(hThreadSnap, &te32 ) );

	_tprintf( TEXT("\n\n"));

	CloseHandle( hThreadSnap );
	return( TRUE );
}*/

/*void printError( TCHAR* msg )
{
	DWORD eNum;
	TCHAR sysMsg[256];
	TCHAR* p;

	eNum = GetLastError( );
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, eNum,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			sysMsg, 256, NULL );

	p = sysMsg;
	while( ( *p > 31 ) || ( *p == 9 ) )
	++p;
	do { *p-- = 0; } while( ( p >= sysMsg ) &&
							( ( *p == '.' ) || ( *p < 33 ) ) );

	_tprintf( TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg );
}*/


/*PBITMAPINFO CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp)
{ 
    BITMAP bmp; 
    PBITMAPINFO pbmi; 
    WORD    cClrBits; 

    // Retrieve the bitmap color format, width, and height.  
    if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp)) 
		cout << "Error: GetObject" << endl; 

    // Convert the color format to a count of bits.  
    cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
    if (cClrBits == 1) 
        cClrBits = 1; 
    else if (cClrBits <= 4) 
        cClrBits = 4; 
    else if (cClrBits <= 8) 
        cClrBits = 8; 
    else if (cClrBits <= 16) 
        cClrBits = 16; 
    else if (cClrBits <= 24) 
        cClrBits = 24; 
    else cClrBits = 32; 

    // Allocate memory for the BITMAPINFO structure. (This structure  
    // contains a BITMAPINFOHEADER structure and an array of RGBQUAD  
    // data structures.)  

     if (cClrBits < 24) 
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
                    sizeof(BITMAPINFOHEADER) + 
                    sizeof(RGBQUAD) * (1<< cClrBits)); 

     // There is no RGBQUAD array for these formats: 24-bit-per-pixel or 32-bit-per-pixel 

     else 
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
                    sizeof(BITMAPINFOHEADER)); 

    // Initialize the fields in the BITMAPINFO structure.  

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
    pbmi->bmiHeader.biWidth = bmp.bmWidth; 
    pbmi->bmiHeader.biHeight = bmp.bmHeight; 
    pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
    pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
    if (cClrBits < 24) 
        pbmi->bmiHeader.biClrUsed = (1<<cClrBits); 

    // If the bitmap is not compressed, set the BI_RGB flag.  
    pbmi->bmiHeader.biCompression = BI_RGB; 

    // Compute the number of bytes in the array of color  
    // indices and store the result in biSizeImage.  
    // The width must be DWORD aligned unless the bitmap is RLE 
    // compressed. 
    pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits +31) & ~31) /8
                                  * pbmi->bmiHeader.biHeight; 
    // Set biClrImportant to 0, indicating that all of the  
    // device colors are important.  
     pbmi->bmiHeader.biClrImportant = 0; 
     return pbmi; 
 } */

/*void CreateBMPFile(HWND hwnd, LPTSTR pszFile, PBITMAPINFO pbi, 
                  HBITMAP hBMP, HDC hDC) 
 { 
     HANDLE hf;                 // file handle  
    BITMAPFILEHEADER hdr;       // bitmap file-header  
    PBITMAPINFOHEADER pbih;     // bitmap info-header  
    LPBYTE lpBits;              // memory pointer  
    DWORD dwTotal;              // total count of bytes  
    DWORD cb;                   // incremental count of bytes  
    BYTE *hp;                   // byte pointer  
    DWORD dwTmp; 

    pbih = (PBITMAPINFOHEADER) pbi; 
    lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

    if (!lpBits) 
		cout << "Error: GlobalAlloc" << endl;

    // Retrieve the color table (RGBQUAD array) and the bits  
    // (array of palette indices) from the DIB.  
    if (!GetDIBits(hDC, hBMP, 0, (WORD) pbih->biHeight, lpBits, pbi, 
        DIB_RGB_COLORS)) 
    {
		cout << "Error: GetDIBits" << endl; 
    }

    // Create the .BMP file.  
    hf = CreateFile(pszFile, 
                   GENERIC_READ | GENERIC_WRITE, 
                   (DWORD) 0, 
                    NULL, 
                   CREATE_ALWAYS, 
                   FILE_ATTRIBUTE_NORMAL, 
                   (HANDLE) NULL); 
    if (hf == INVALID_HANDLE_VALUE) 
        cout << "Error: WriteFile" << endl; 
    hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M"  
    // Compute the size of the entire file.  
    hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + 
                 pbih->biSize + pbih->biClrUsed 
                 * sizeof(RGBQUAD) + pbih->biSizeImage); 
    hdr.bfReserved1 = 0; 
    hdr.bfReserved2 = 0; 

    // Compute the offset to the array of color indices.  
    hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
                    pbih->biSize + pbih->biClrUsed 
                    * sizeof (RGBQUAD); 

    // Copy the BITMAPFILEHEADER into the .BMP file.  
    if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), 
        (LPDWORD) &dwTmp,  NULL)) 
    {
       cout << "Error: WriteFile" << endl; 
    }

    // Copy the BITMAPINFOHEADER and RGBQUAD array into the file.  
    if (!WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER) 
                  + pbih->biClrUsed * sizeof (RGBQUAD), 
                  (LPDWORD) &dwTmp, ( NULL)))
        cout << "Error: WriteFile" << endl;

    // Copy the array of color indices into the .BMP file.  
    dwTotal = cb = pbih->biSizeImage; 
    hp = lpBits; 
    if (!WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp,NULL)) 
           cout << "Error: WriteFile" << endl; 

    // Close the .BMP file.  
     if (!CloseHandle(hf)) 
           cout << "Error: CloseHandle" << endl; 

    // Free memory.  
    GlobalFree((HGLOBAL)lpBits);
}*/