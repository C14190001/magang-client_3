//Tutorial Background Tray:
//https://www.lotushints.com/2013/03/win32-hide-to-system-tray-part-1/
//https://www.lotushints.com/2013/03/win32-hide-to-system-tray-part-2/

#include <iostream>
#include <mysql.h>
#include <string>
#include <fstream>
#include <thread>
#include <Windows.h>
#include <stdio.h>
#include "clientSpecs.h"
#include "clientApps.h"
#include "framework.h"
#include "backgroundTray.h"
#include "3_ClientGUI.h"
using namespace std;

//Config
#define ServerIP "192.168.18.5"
#define DbName "magang-database"
#define DbUsername "client"
#define DbPassword ""
#define DbPort 3306

#define MAX_LOADSTRING 100

// Global Variables:
HWND hWnd;
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
MYSQL* conn; MYSQL_ROW row; MYSQL_RES* res;
string query, logText, logS; int clientID, qState;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void logTextFunction(string text) {
	logText += text + "\r\n";
	HWND hText = GetDlgItem(hWnd, IDC_LOGWINDOW);
	SetWindowText(hText, wstring(logText.begin(), logText.end()).c_str());

	SendMessageA(hText, EM_SETSEL, 0, -1);
	SendMessageA(hText, EM_SETSEL, -1, -1);
	SendMessageA(hText, EM_SCROLLCARET, 0, 0);
}
void insertRemainingStatus(string status) {
	fstream statusFile("statusUpdates.txt", fstream::out | fstream::app);
	struct tm timeinfo;
	char buffer[20];
	time_t t = time(0);
	localtime_s(&timeinfo, &t);

	strftime(buffer, 20, "%F %T", &timeinfo);
	string newStatus = status + "." + buffer + "\n";
	statusFile << newStatus;
	statusFile.close();
}
string deleteLastRemainingStatus() {
	//Returns last remaining Status
	fstream input("statusUpdates.txt", fstream::in);
	string a, b; int l = 0, n = 0, o = 0;

	if (input.is_open()) {
		while (!input.eof()) {
			getline(input, a);
			if (!a.empty()) {
				b += a + "/";
				l++;
			}
		}
	}
	else {
		logS = "statusUpdates.txt not found!";
		logTextFunction(logS);
	}
	input.close();

	input.open("statusUpdates_Temp.txt", fstream::out);
	for (int i = 0; i < b.length(); i++) {
		if (b[i] == '/') {
			if (o < l - 1) {
				a = b.substr(n, i - n);
				input << a << endl;
				n = i + 1;
				o++;
			}
			else {
				a = b.substr(n, i - n);
				break;
			}
		}
	}
	input.close();
	remove("statusUpdates.txt");
	rename("statusUpdates_Temp.txt", "statusUpdates.txt");
	return a;
}
void connectToDatabase() {
	conn = mysql_init(0);
	conn = mysql_real_connect(conn, ServerIP, DbUsername, DbPassword, DbName, DbPort, NULL, 0);
	while (!conn) {
		logS = "Error connecting to Database, Reconnecting...";
		logTextFunction(logS);

		conn = mysql_init(0);
		conn = mysql_real_connect(conn, ServerIP, DbUsername, DbPassword, DbName, DbPort, NULL, 0);
		Sleep(3000);
	}
	logS = "Connected to Database!";
	logTextFunction(logS);
}
string sqlQuery(string query) {
	string disconnect = "", qOutput = "";
	const char* q = query.c_str();
	qState = mysql_query(conn, q);

	if (!qState) {
		res = mysql_store_result(conn);
		if (res != NULL) {
			qOutput = "";
			while (row = mysql_fetch_row(res)) {
				int j = res->field_count;
				for (int i = 0; i < j; i++) {
					string x = row[i];
					qOutput += x + '/';
				}
				qOutput += '\n';
			}
		}
	}
	else {
		string sqlErr = mysql_error(conn);
		logS = "Database error: " + sqlErr;
		logTextFunction(logS);
		logS = "Trying to Reconnecting to the Database...";
		logTextFunction(logS);

		insertRemainingStatus("Disconnected"); //Simpan status Disconnected ke file (buat jaga2 kalau dimatikan saat Disconnected)

		do {
			conn = mysql_init(0);
			conn = mysql_real_connect(conn, ServerIP, DbUsername, DbPassword, DbName, DbPort, NULL, 0);
			while (!conn) {
				conn = mysql_init(0);
				conn = mysql_real_connect(conn, ServerIP, DbUsername, DbPassword, DbName, DbPort, NULL, 0);

				logS = "Error connecting to Database, Reconnecting...";
				logTextFunction(logS);

				Sleep(3000);
			}
			logS = "Connected to Database!";
			logTextFunction(logS);

			//Ambil status Disconnected yang disimpan sebelumnya di file
			disconnect = deleteLastRemainingStatus() + "/";

			//Kirim status Disconnected & Connected
			int b = 0;
			for (int i = 0; i < disconnect.length(); i++) {
				if (disconnect[i] == '/') {
					for (int j = b; b < i; j++) {
						if (disconnect[j] == '.') {
							string sq = "INSERT INTO `client_status` (`id`, `status`, `date_time`) VALUES ('" + to_string(clientID) + "', '" + disconnect.substr(b, j - b) + "', '" + disconnect.substr(j + 1, i - j - 1) + "')";
							q = sq.c_str();
							mysql_query(conn, q);
							break;
						}
					}
					b = i + 1;
				}
			}
			string sq = "INSERT INTO `client_status` (`id`, `status`) VALUES ('" + to_string(clientID) + "', 'Connected')";
			q = sq.c_str();
			mysql_query(conn, q);

			//Lakukan Query
			logS = "Retrying query: " + query;
			logTextFunction(logS);

			q = query.c_str();
			qState = mysql_query(conn, q);
			if (!qState) {
				res = mysql_store_result(conn);
				if (res != NULL) {
					qOutput = "";
					while (row = mysql_fetch_row(res)) {
						int j = res->field_count;
						for (int i = 0; i < j; i++) {
							string x = row[i];
							qOutput += x + '/';
						}
						qOutput += '\n';
					}
				}
			}
		} while (qState);
	}
	if (qOutput.empty()) {
		return "-1";
	}
	return qOutput;
}

void setup() {
	int choice = -1;
	fstream in("ClientID.txt", fstream::in);
	if (in.is_open()) { in >> clientID; }
	else { 
		logS = "ClientID.txt not found";
		logTextFunction(logS);
	}
	in.close();

	if (clientID == NULL) {
		::MessageBox(hWnd, _T("Please set the Client ID first"), _T("Error"), MB_OK | MB_ICONHAND);
		exit(0);
	}

	if (stoi(sqlQuery("SELECT `id` FROM `clients` WHERE `id` = " + to_string(clientID))) != clientID) {
		remove("ClientID.txt");
		clientID = NULL;
		setup();
	}
}
void uploadRemainingStatus() {
	fstream statusFile("statusUpdates.txt", fstream::in);
	string statusQueue = "";
	if (statusFile.is_open()) {
		while (!statusFile.eof()) {
			string getL;
			getline(statusFile, getL);
			if (!getL.empty()) {
				statusQueue += getL + "/";
			}
		}
		statusFile.close();
	}
	else {
		logS = "statusUpdates.txt not found.";
		logTextFunction(logS);
	}

	if (!statusQueue.empty()) {
		int b = 0;
		for (int i = 0; i < statusQueue.length(); i++) {
			if (statusQueue[i] == '/') {
				for (int j = b; b < i; j++) {
					if (statusQueue[j] == '.') {
						//cout << "Updating status... (" + statusQueue.substr(b, j - b) + " at " + statusQueue.substr(j + 1, i - j - 1) + ")\n";
						sqlQuery("INSERT INTO `client_status` (`id`, `status`, `date_time`) VALUES ('" + to_string(clientID) + "', '" + statusQueue.substr(b, j - b) + "', '" + statusQueue.substr(j + 1, i - j - 1) + "')");
						break;
					}
				}
				cout << endl;
				b = i + 1;
			}
		}
	}

	remove("statusUpdates.txt");
	fstream newFile("statusUpdates.txt", fstream::out | fstream::app);
	newFile.close();
}
void waitForNewRequest() {
	//Starting up
	logS = "Connecting to the Database...";
	logTextFunction(logS);
	connectToDatabase();
	setup();
	uploadRemainingStatus();

	logS = "------------------------------";
	logTextFunction(logS);
	logS = "[ Client ID: " + to_string(clientID) + " ]";
	logTextFunction(logS);
	logS = "------------------------------";
	logTextFunction(logS);

	//Check for requests
	srand(time(0));
	int refreshTime = (rand() % (15 - 5 + 1)) + 5;

	logS = "Checking for updates every " + to_string(refreshTime) + " seconds.\n";
	logTextFunction(logS);
	while (1) {
		uploadRemainingStatus();
		//Upload / Insert Specs and Apps
		if (stoi(sqlQuery("SELECT `updated?` FROM `clients` WHERE `id` = " + to_string(clientID))) == 0) {
			//Apps
			if(stoi(sqlQuery("SELECT `client_apps`.`id` FROM `client_apps` WHERE `client_apps`.`id` = "+ to_string(clientID))) == clientID){
				logTextFunction("Updating Apps...");
				sqlQuery("UPDATE `client_apps` SET `apps` = '" + getAllInstalledPrograms() + "' WHERE `client_apps`.`id` = " + to_string(clientID));
			}
			else {
				logTextFunction("Inserting Apps...");
				sqlQuery("INSERT INTO `client_apps` (`id`, `apps`) VALUES ('" + to_string(clientID) + "', '" + getAllInstalledPrograms() + "')");
			}
			
			//Specs
			string name = "N/A", cpu = "N/A", igpu = "N/A", egpu = "N/A", ip = "N/A", mac = "N/A";
			int ram = 0, hdd = 0;
			getAllSpecs(name, cpu, igpu, egpu, ram, hdd, ip, mac); 

			if (stoi(sqlQuery("SELECT `client_specs`.`id` FROM `client_specs` WHERE `client_specs`.`id` = " + to_string(clientID))) == clientID) {
				logTextFunction("Updating Specs...");
				sqlQuery("UPDATE `client_specs` SET `name` = '" + name + "', `cpu` = '" + cpu + "', `i-gpu` = '" + igpu + "', `e-gpu` = '" + egpu + "', `ram` = '" + to_string(ram) + "', `memory` = '" + to_string(hdd) + "', `ip` = '" + ip + "', `mac` = '" + mac + "' WHERE `client_specs`.`id` = " + to_string(clientID));
				}
			else {
				logTextFunction("Inserting Specs...");
				sqlQuery("INSERT INTO `client_specs` (`id`, `name`, `cpu`, `i-gpu`, `e-gpu`, `ram`, `memory`, `ip`, `mac`) VALUES ('" + to_string(clientID) + "', '" + name + "', '" + cpu + "', '" + igpu + "', '" + egpu + "', '" + to_string(ram) + "', '" + to_string(hdd) + "', '" + ip + "', '" + mac + "')");
			}

			//Done
			sqlQuery("UPDATE `clients` SET `updated?` = '1' WHERE `clients`.`id` = " + to_string(clientID));
			logS = "Done updating.";
			logTextFunction(logS);
		}
		Sleep(refreshTime * 1000);
	}
}



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY3CLIENTGUI, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY3CLIENTGUI));

    MSG msg;

    // Main message loop:
	insertRemainingStatus("ON");
	thread clientProgram(waitForNewRequest);
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY3CLIENTGUI));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MY3CLIENTGUI);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_SYSMENU,
      0, 0, 300, 200, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   HWND logWindow = CreateWindow(L"Edit", L"", WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL, 0, 0, 280, 135, hWnd, (HMENU)IDC_LOGWINDOW, NULL, NULL);

   //ShowWindow(hWnd, nCmdShow);
   TrayDrawIcon(hWnd);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_ENDSESSION: {
		insertRemainingStatus("OFF");

		if (conn) {
			string sq = "INSERT INTO `client_status` (`id`, `status`) VALUES ('" + to_string(clientID) + "', 'OFF')";
			const char* q = sq.c_str();
			mysql_query(conn, q);
		}
		if (!qState) {
			deleteLastRemainingStatus();
		}
		Sleep(3000);
		return 0;
	}
	break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
				::MessageBox(hWnd, _T("Version 1.2"), _T("About"), MB_OK);
                //DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
			case IDM_EXIT: {
				int iMessage = ::MessageBox(hWnd, _T("Are you sure?"), _T("Exit"), MB_YESNO | MB_ICONQUESTION);
				switch (iMessage) {
				case IDYES: {
					//DestroyWindow(hWnd);
					exit(0);
					break;
				}
				default:
					break;
				}
				break;
			}
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
    break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;

    //( Sembunyikan ke Tray )
    case WM_CLOSE:
        TrayDrawIcon(hWnd);
        ShowWindow(hWnd, SW_HIDE);
        break;
    //( Munculkan ketika Tray di-klik )
    case WM_TRAYMESSAGE:
        switch (lParam) {
        case WM_LBUTTONDOWN:
            ShowWindow(hWnd, SW_SHOW);
            TrayDeleteIcon(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
