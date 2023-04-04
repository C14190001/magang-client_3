#pragma once
#include "resource.h"
#include "framework.h"
#include <shellapi.h>
#include <windows.h>

extern HINSTANCE hInst;
void TrayDrawIcon(HWND hWnd);
void TrayDeleteIcon(HWND hWnd);

//
//  FUNCTION: TrayDrawIcon(HWND)
//
//  PURPOSE:  Draws application icon in a system tray
//
//
void TrayDrawIcon(HWND hWnd) {
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = TRAY_ICONUID;
	nid.uVersion = NOTIFYICON_VERSION;
	nid.uCallbackMessage = WM_TRAYMESSAGE;
	nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));
	LoadString(hInst, IDS_APP_TITLE, nid.szTip, 128);
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	Shell_NotifyIcon(NIM_ADD, &nid);
}

//
//  FUNCTION: TrayDeleteIcon(HWND)
//
//  PURPOSE:  Deletes application icon from system tray
//
//
void TrayDeleteIcon(HWND hWnd) {
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = TRAY_ICONUID;
	Shell_NotifyIcon(NIM_DELETE, &nid);
}