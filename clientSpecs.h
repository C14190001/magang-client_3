#define _WIN32_DCOM
#include <iostream>
#include <string>  
#include <iostream> 
#include <sstream>
#include <comdef.h>
#include <Wbemidl.h>
#include <fstream>
#include <stdlib.h>
#pragma comment(lib, "wbemuuid.lib")
using namespace std;

//Konversi BSTR ke STRING (https://stackoverflow.com/questions/6284524/bstr-to-stdstring-stdwstring-and-vice-versa)
string ConvertWCSToMBS(const wchar_t* pstr, long wslen) {
	int len = ::WideCharToMultiByte(CP_ACP, 0, pstr, wslen, NULL, 0, NULL, NULL);
	string dblstr(len, '\0');
	len = ::WideCharToMultiByte(CP_ACP, 0, pstr, wslen, &dblstr[0], len, NULL, NULL);

	return dblstr;
}
std::string ConvertBSTRToMBS(BSTR bstr) {
	int wslen = ::SysStringLen(bstr);
	return ConvertWCSToMBS((wchar_t*)bstr, wslen);
}

//https://learn.microsoft.com/en-us/windows/win32/wmisdk/example--getting-wmi-data-from-the-local-computer
//Variable inputQuery + inputProperty. Output: String dengan '/' Tiap element.
//
//OS Name: "SELECT * FROM Win32_OperatingSystem" + "Name"
//All GPU Name: "SELECT * FROM Win32_VideoController" + "Name"
//CPU Name: "SELECT * FROM Win32_Processor" + "Name"
//HDD / SSD Capacity: "SELECT * FROM Win32_DiskDrive" + "Size"
//RAM Capacity: "SELECT * FROM Win32_PhysicalMemory" + "Capacity"
//PC Name: "SELECT * FROM Win32_ComputerSystem" + "Name"
//MAC Address: "SELECT * FROM Win32_NetworkAdapterConfiguration WHERE IPEnabled = True" + "MACAddress"
string getWmiData(string inputQuery, string inputProperty) {
	string outputString = "";
	HRESULT hres;
	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres)) {
		return "-1";
	}

	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if (FAILED(hres)) {
		CoUninitialize();
		return "-1";
	}

	IWbemLocator* pLoc = NULL;
	hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
	if (FAILED(hres)) {
		CoUninitialize();
		return "-1";
	}

	IWbemServices* pSvc = NULL;
	hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
	if (FAILED(hres)) {
		pLoc->Release();
		CoUninitialize();
		return "-1";
	}

	hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return "-1";
	}

	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(inputQuery.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return "-1";
	}

	IWbemClassObject* pclsObj = NULL; ULONG uReturn = 0;
	while (pEnumerator) {
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
		if (0 == uReturn) { break; }

		VARIANT vtProp;
		VariantInit(&vtProp);
		//String to LPCWSTR
		//https://www.geeksforgeeks.org/convert-stdstring-to-lpcwstr-in-c/
		wstring temp = wstring(inputProperty.begin(), inputProperty.end());
		LPCWSTR wideString = temp.c_str();

		hr = pclsObj->Get(wideString, 0, &vtProp, 0, 0);
		outputString += ConvertBSTRToMBS(vtProp.bstrVal) + "/";

		VariantClear(&vtProp);
		pclsObj->Release();
	}

	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();
	CoUninitialize();
	return outputString;
}

string getAllIpAddress() {
	string line = "";
	string line2 = "";
	ifstream ipFile;
	int offset;
	system("ipconfig /all > ip.txt");
	ipFile.open("ip.txt");
	if (ipFile.is_open()) {
		while (!ipFile.eof()) {
			getline(ipFile, line);
			if ((offset = line.find("IPv4 Address. . . . . . . . . . . :")) != string::npos) {
				line.erase(0, 39);
				int pos = line.find("(Preferred)");
				line2 += line.substr(0, pos) + "/";
			}
		}
	}
	ipFile.close();
	remove("ip.txt");

	return line2;
}
string getAllMacAddress() {
	string line = "";
	string line2 = "";
	ifstream ipFile;
	int offset;
	system("ipconfig /all > ip.txt");
	ipFile.open("ip.txt");
	if (ipFile.is_open()) {
		while (!ipFile.eof()) {
			getline(ipFile, line);
			if ((offset = line.find("Physical Address. . . . . . . . . :")) != string::npos) {
				line.erase(0, 39);
				line2 += line + "/";
			}
		}
	}
	ipFile.close();
	remove("ip.txt");
	return line2;
}

void viewAllSpecs() {
	string substring = "";
	bool igpu = false; int b = 0; double c = 0;
	//PC Name
	substring = getWmiData("SELECT * FROM Win32_ComputerSystem", "Name");
	for (int i = 0; i < substring.length(); i++) {
		if (substring[i] == '/') {
			cout << "PC Name: " << substring.substr(0, i) << endl;
			break;
		}
	}
	//CPU
	substring = getWmiData("SELECT * FROM Win32_Processor", "Name");
	for (int i = 0; i < substring.length(); i++) {
		if (substring[i] == '/') {
			cout << "CPU: " << substring.substr(0, i) << endl;
			break;
		}
	}
	//GPU
	substring = getWmiData("SELECT * FROM Win32_VideoController", "Name");
	for (int i = 0; i < substring.length(); i++) {
		if (!igpu) {
			if (substring[i] == '/') {
				if (i == substring.length() - 1) {
					cout << "Intergrated GPU: " << substring.substr(b, i - b) << endl;
					break;
				}
				else {
					cout << "Dedicated GPU: " << substring.substr(0, i) << endl;
					b = i;
					igpu = true;
				}
			}
		}
		else {
			if (substring[i] == '/') {
				cout << "Intergrated GPU: " << substring.substr(b + 1, i - 1 - b) << endl;
				break;
			}
		}
	}
	//RAM
	substring = getWmiData("SELECT * FROM Win32_PhysicalMemory", "Capacity");
	b = 0;
	for (int i = 0; i < substring.length(); i++) {
		if (substring[i] == '/') {
			c += stod(substring.substr(b, i));
			b = i + 1;
		}
	}
	cout << "RAM: " << int(c / 1073741824) << " GB\n";
	//HDD
	substring = getWmiData("SELECT * FROM Win32_DiskDrive", "Size");
	b = 0; c = 0;
	for (int i = 0; i < substring.length(); i++) {
		if (substring[i] == '/') {
			c += stod(substring.substr(b, i));
			b = i + 1;
		}
	}
	cout << "HDD: " << int(c / 1073741824) << " GB\n";

	//IP Address
	cout << "IP Address: ";
	substring = getAllIpAddress();
	b = 0;
	for (int i = 0; i < substring.length(); i++) {
		if (substring[i] == '/') {
			cout << substring.substr(b, i - b);
			b = i + 1;
			if (i < substring.length() - 1) {
				cout << ", ";
			}
		}
	}
	cout << "\n";

	//MAC Address
	cout << "MAC Address: ";
	substring = getAllMacAddress();
	b = 0;
	for (int i = 0; i < substring.length(); i++) {
		if (substring[i] == '/') {
			cout << substring.substr(b, i - b);
			b = i + 1;
			if (i < substring.length() - 1) {
				cout << ", ";
			}
		}
	}
	cout << "\n";
}
void getAllSpecs(string& name, string& cpu, string& igpu, string& egpu, int& ram, int& hdd, string& ip, string& mac) {
	string substring = "";
	bool twoGpu = false; int b = 0; double c = 0;

	//PC Name
	substring = getWmiData("SELECT * FROM Win32_ComputerSystem", "Name");
	for (int i = 0; i < substring.length(); i++) {
		if (substring[i] == '/') {
			name = substring.substr(0, i);
			break;
		}
	}
	//CPU
	substring = getWmiData("SELECT * FROM Win32_Processor", "Name");
	for (int i = 0; i < substring.length(); i++) {
		if (substring[i] == '/') {
			cpu = substring.substr(0, i);
			break;
		}
	}
	//GPU
	substring = getWmiData("SELECT * FROM Win32_VideoController", "Name");
	for (int i = 0; i < substring.length(); i++) {
		if (!twoGpu) {
			if (substring[i] == '/') {
				if (i == substring.length() - 1) {
					igpu = substring.substr(b, i - b);
					break;
				}
				else {
					egpu = substring.substr(0, i);
					b = i;
					twoGpu = true;
				}
			}
		}
		else {
			if (substring[i] == '/') {
				igpu = substring.substr(b + 1, i - 1 - b);
				break;
			}
		}
	}
	//RAM
	substring = getWmiData("SELECT * FROM Win32_PhysicalMemory", "Capacity");
	b = 0;
	for (int i = 0; i < substring.length(); i++) {
		if (substring[i] == '/') {
			c += stod(substring.substr(b, i));
			b = i + 1;
		}
	}
	ram = int(c / 1073741824);
	//HDD
	substring = getWmiData("SELECT * FROM Win32_DiskDrive", "Size");
	b = 0; c = 0;
	for (int i = 0; i < substring.length(); i++) {
		if (substring[i] == '/') {
			c += stod(substring.substr(b, i));
			b = i + 1;
		}
	}
	hdd = int(c / 1073741824);
	//IP Address
	ip = getAllIpAddress();

	//MAC Address
	mac = getAllMacAddress();
}