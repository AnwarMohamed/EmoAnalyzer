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
#include "Serial.h"


using namespace std;

DWORD	gPidToFind=0;
HWND    gTargetWindowHwnd = NULL;
HWND	NextHwndParent = NULL;
INT		PosArray[4][3];
HWND	NextWidgetsHwnd[4][3];
CSerial serial;

BOOL CALLBACK myWNDENUMPROC(HWND hwCurHwnd, LPARAM lpMylp);
VOID ProcessColours();
void RgbToHsb(struct RGB_t rgb, struct HSB_t* outHsb);

typedef struct RGB_t { unsigned char red, green, blue; } RGB;
typedef struct HSB_t { float hue, saturation, brightness; } HSB;

int main()
{
	cout << "**************************************************" << endl;
	cout << "*                                                *" << endl;
	cout << "*         Emotiv EPOC Brain Activity Map         *" << endl;
	cout << "*           Serial Port Control Server           *" << endl;
	cout << "*   By Anwar Mohamed <anwarelmakrahy@gmail.com>  *" << endl;
	cout << "*         Programmed for project name:           *" << endl;
	cout << "*      How to control objects using your mind    *" << endl;
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

	Sleep(3000);

	gPidToFind = process_info.dwProcessId;
	EnumWindows(myWNDENUMPROC, process_info.dwProcessId);
	
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
		PosArray[k][1] = rectangle.bottom;
		PosArray[k][2] = rectangle.left;
		PosArray[k][3] = rectangle.right;

	}

	int serial_port;
	cout << endl << "Please enter serial port number: ";
	cin >> serial_port;

	if (!serial.Open(serial_port, 9600))
	{
		cout << "Cannot connect to serial port. exiting..." << endl;
		system("PAUSE");

		serial.Close();
		TerminateProcess(process_info.hProcess, 0);
		return 0;
	}

	cout << endl << "Press enter anytime to stop analyzing data and exit" << endl << endl;

	while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000))
	//for(int i=0; i<2; i++)
	{
		Sleep(2000);
		ProcessColours();
	}

	cout << endl;
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

	RGB color;
	color.red = GetRValue(colorBeta);
	color.blue = GetBValue(colorBeta);
	color.green = GetGValue(colorBeta);

	//memset(&color, 155 , sizeof(RGB));
	//color = { 0, 0, 0 };
	
    struct HSB_t hsb;
    RgbToHsb(color, &hsb);

    //printf("RGB(%u,%u,%u) -> HSB(%f,%f,%f)\n", color.red, color.green, color.blue, hsb.hue, hsb.saturation, hsb.brightness * 100);

	if (hsb.hue  >= 0 && hsb.hue < 60 && hsb.saturation  > 0)
	{
		cout <<  "red region" << endl;
		serial.SendData("1", 1);
	}
	else if (hsb.hue  >= 60 && hsb.hue < 120 && hsb.saturation  > 0)
	{
		cout << "yellow region" << endl;
		serial.SendData("2", 1);
	} 
	else if (hsb.hue >= 120 && hsb.hue < 180 && hsb.saturation  > 0)
	{
		cout << "green region" << endl;
		serial.SendData("3", 1);
	}
	else
		serial.SendData("0", 1);

	cout<< hsb.hue << "%" << endl;
}

void RgbToHsb(struct RGB_t rgb, struct HSB_t* outHsb)
{
    // TODO check arguments

    float r = rgb.red / 255.0f;
    float g = rgb.green / 255.0f;
    float b = rgb.blue / 255.0f;
    float max = max(max(r, g), b);
    float min = min(min(r, g), b);
    float delta = max - min;
    if (delta != 0)
    {
        float hue;
        if (r == max)
        {
            hue = (g - b) / delta;
        }
        else
        {
            if (g == max)
            {
                hue = 2 + (b - r) / delta;
            }
            else
            {
                hue = 4 + (r - g) / delta;
            }
        }
        hue *= 60;
        if (hue < 0) hue += 360;
        outHsb->hue = hue;
    }
    else
    {
        outHsb->hue = 0;
    }
    outHsb->saturation = max == 0 ? 0 : (max - min) / max;
    outHsb->brightness = max;
}