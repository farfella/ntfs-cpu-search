/*
Copyright (c) Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#include <stdio.h>
#include <windows.h>
#include <commctrl.h>

#include "bootsector.h"
#include "volumereader.h"
#include "ntfs.h"
#include "masterfiletable.h"


#include "bmhapu.h"
#include "threadpool.h"
#include "mempool.h"

#include "bmhwide.h"
#include "cs.h"

#include <vector>
#include <string>

#include "resource.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


struct RESULT
{
	wchar_t FileName[256];
	wchar_t FilePath[1024];
	wchar_t Date[256];
};
std::vector<RESULT> results;
CRITICAL_SECTION resultCs;


struct SearchStruct
{
	wchar_t FileName[256];
	wchar_t Content[256];
	wchar_t Drive[256];
	bool	useCPU;
};


HWND hWndMain;
HWND hwndStatusBar;
HINSTANCE hInstance;

INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int init();
void deinit();

int WINAPI WinMain(__in HINSTANCE hInstance,
				   __in_opt HINSTANCE hPrevInstance,
				   __in LPSTR lpCmdLine,
				   __in int nShowCmd)
{
	
	InitializeCriticalSection(&resultCs);
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof icex;
	icex.dwICC = ICC_PROGRESS_CLASS | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;

	::hInstance = hInstance;

	InitCommonControlsEx(&icex);


	hWndMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOGMAIN), NULL, DialogProc);

	hwndStatusBar = CreateStatusWindow( WS_CHILD | WS_VISIBLE, TEXT("Initializing..."), hWndMain, 50000);

	init();

	MSG msg = {0};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!IsDialogMessage(hWndMain, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}


	CloseWindow(hwndStatusBar);
	EndDialog(hWndMain, 0);

	deinit();

	DeleteCriticalSection(&resultCs);

	return 0;
}


void error(const char * str, ...);

std::vector<std::wstring> columns;
bool initcolumns(HWND hWndListView);


DWORD WINAPI SearchThread(LPVOID lpv);
volatile long StopRequested = 0;


void ShowHelpAbout(HWND hWnd);

INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICONAS));

			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

			initcolumns(GetDlgItem(hWnd, IDC_LISTFILES));

			SendMessage(GetDlgItem(hWnd, IDC_COMBODRIVES), CB_SETCURSEL, 0, 0);
			

		}
		return TRUE;
	case WM_SYSCOMMAND:
		if (wParam == SC_CLOSE)
		{
			PostQuitMessage(0);
			return TRUE;
		}
		else if (wParam == SC_CONTEXTHELP)
		{
			ShowHelpAbout(hWnd);
			return TRUE;
		}
		break;
	case WM_COMMAND:
		if (wParam == IDSEARCH)
		{
			wchar_t Search[200] = {0};
			if (GetDlgItemText(hWnd, IDSEARCH, Search, 200))
			{
				if (lstrcmp(Search, L"Search") == 0)
				{
					{
						cs _(resultCs);
						results.clear();
						ListView_DeleteAllItems(GetDlgItem(hWnd, IDC_LISTFILES));
						InterlockedExchange(&StopRequested, 0);
					}

					SetDlgItemText(hWnd, IDSEARCH, L"Stop");
					
					SearchStruct * s = new SearchStruct;
				
					GetDlgItemText(hWnd, IDC_EDITFILENAME, s->FileName, 256);
					GetDlgItemText(hWnd, IDC_EDITCONTENT, s->Content, 256);
					GetDlgItemText(hWnd, IDC_COMBODRIVES, s->Drive, 256);

					UINT isGPUsChecked = IsDlgButtonChecked(hWnd, IDC_RADIOGPUS);
					s->useCPU = !isGPUsChecked;

					HANDLE h = CreateThread(NULL, 0, SearchThread, s, 0, 0);
					if (NULL != h)
					{
						CloseHandle(h);
					}
					
					wchar_t Searching[1024] = {0};
					LoadString(GetModuleHandle(NULL), IDS_SEARCHING, Searching, 1024);
					SendMessage(hwndStatusBar, SB_SETTEXT, 0, (LPARAM)Searching);

				}
				else if (lstrcmp(Search, L"Stop") == 0)
				{
					wchar_t Stopping[1024] = {0};
					LoadString(GetModuleHandle(NULL), IDS_STOPPING, Stopping, 1024);

					SetDlgItemText(hWnd, IDSEARCH, Stopping);
					InterlockedIncrement(&StopRequested);
				}
			}


			return TRUE;
		}
		break;
	case WM_NOTIFY:
		{
			NMLVDISPINFO* plvdi;
			NMITEMACTIVATE * plact;

			switch (((LPNMHDR) lParam)->code)
			{
			case NM_DBLCLK:
				plact = (NMITEMACTIVATE*)lParam;
				switch (plact->iSubItem)
				{
				case 0:
					{
						std::wstring path = results[plact->iItem].FilePath;
						path = path.substr(2);
						wchar_t drive[256] = {0};
						GetDlgItemText(hWnd, IDC_COMBODRIVES, drive, 256);
						path = drive + path;
						path = L"explorer /select," + path;
						//_wsystem(path.c_str());
						STARTUPINFO si = {0};
						PROCESS_INFORMATION pi = {0};
						si.cb = sizeof(si);
						if (CreateProcess(NULL, &path[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
						{
							CloseHandle(pi.hProcess);
							CloseHandle(pi.hThread);
						}
					}
					break;
				}
				break;
			case LVN_GETDISPINFO:
				plvdi = (NMLVDISPINFO*)lParam;
				switch (plvdi->item.iSubItem)
				{
					case 0:
						plvdi->item.pszText = results[plvdi->item.iItem].FileName;
						break;
					case 1:
						plvdi->item.pszText = results[plvdi->item.iItem].FilePath;
						break;
					case 2:
						plvdi->item.pszText = results[plvdi->item.iItem].Date;
						break;
					default:
						break;
				}
			}
			break;
		}
		break;
	}

	return FALSE;
}

bool initcolumns(HWND hWndListView) 
{ 

	columns.push_back(L"Name");
	columns.push_back(L"Path");
	columns.push_back(L"Last Accessed");

	std::vector<int> widths;
	widths.push_back(100);
	widths.push_back(300);
	widths.push_back(100);


	for (size_t i = 0; i < columns.size(); i++)
    {
		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

        lvc.iSubItem = i;
        lvc.pszText = const_cast<wchar_t*>(columns[i].c_str());
        lvc.cx = widths[i];

        //if ( i < 2 )
            lvc.fmt = LVCFMT_LEFT; 
        //else
        //   lvc.fmt = LVCFMT_RIGHT;

        if (ListView_InsertColumn(hWndListView, i, &lvc) == -1)
            return false;
    }
 
	return true;
}

void ShowHelpAbout(HWND hWnd)
{
	wchar_t Title[256] = {0};
	LoadString(hInstance, IDS_STRING_TITLE, Title, 256);
	MessageBox(hWnd, L"Copyright (c) Ateeq Sharfuddin, 2011.\nAll Rights Reserved.", Title, MB_OK | MB_ICONINFORMATION);
}

void search(std::wstring const filename, std::wstring const content, std::wstring const drive, bool useCPU);

DWORD WINAPI SearchThread(LPVOID lpv)
{
	SearchStruct * s = (SearchStruct*)(lpv);

	LARGE_INTEGER freq = {0}, start = {0}, end = {0};
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);
	search(s->FileName, s->Content, s->Drive, s->useCPU);
	QueryPerformanceCounter(&end);

	end.QuadPart -= start.QuadPart;
	double e = (double)end.QuadPart;
	double f = (double)freq.QuadPart;
	e /= f;

	wchar_t text[256] = {0};

	DWORD const d= results.size();

	//swprintf(text, L"Done %f seconds. %d Found", e, d);
	wchar_t DoneString[1024] = {0};
	if (LoadString(GetModuleHandle(NULL), IDS_SEARCH_DONE, DoneString, 1024))
		swprintf(text, ARRAYSIZE(text), DoneString, e, d);

	wchar_t Search[1024] = {0};
	if (LoadString(GetModuleHandle(NULL), IDS_SEARCH, Search, 1024))
		SetDlgItemText(hWndMain, IDSEARCH, Search);

	SendMessage(hwndStatusBar, SB_SETTEXT, 0, (LPARAM)text);

	delete s;
	return 0;
}



std::map<std::wstring, std::pair<reader *, NTFS *> > getNTFSDrives();
char * readSource();


threadpool tp;
threadpool tp2;


bool	canUseCPU = true;


std::map<std::wstring, std::pair<reader *, NTFS *> > mapper;
std::map<std::wstring, masterfiletable *> mftsCPU;


int init()
{

	bmh::wide::ci::init();
	bmh::binary::init();

	const unsigned int readsize = 1048576;

	memory::pool::init(readsize, 20, 40);

	SYSTEM_INFO sysInfo = {0};
	GetSystemInfo(&sysInfo);

	if (sysInfo.dwNumberOfProcessors == 0)	// unlikely
		sysInfo.dwNumberOfProcessors = 1;

	tp.init(sysInfo.dwNumberOfProcessors * 2); // 2 threads per CPU...
	tp2.init(2);

	//tp.init(1); // 2 threads per CPU...

	mapper = getNTFSDrives();
	
	std::map<std::wstring, std::pair<reader *, NTFS *> >::iterator i;
	for (i = mapper.begin(); i != mapper.end(); ++i)
	{
		masterfiletable * mftCPU = new masterfiletable(i->second.second, i->second.first);
		mftCPU->init(readsize );
		mftsCPU[i->first] = mftCPU;
	}
	

	SendMessage(GetDlgItem(hWndMain, IDC_COMBODRIVES), CB_SETCURSEL, 0, 0);

	EnableWindow(GetDlgItem(hWndMain, IDC_RADIOGPUS), FALSE);
	CheckRadioButton(hWndMain, IDC_RADIOGPUS, IDC_RADIOCPUS, IDC_RADIOCPUS);

	SendMessage(hwndStatusBar, SB_SETTEXT, 0, (LPARAM)L"Enter your query...");


	return 0;
}


void error(const char * str, ...)
{
	va_list p;
	char b[1024] = {0};
	
	va_start(p, str);
	_vsprintf_p(b, 1024, str, p);
	va_end(p);

	b[1023] = 0;


#ifdef _CONSOLE
	printf("%s", b);
#else
	MessageBoxA(hWndMain, b, "Desktop Search", MB_OK|MB_ICONERROR);
#endif
}



std::map<std::wstring, std::pair<reader *, NTFS *> > getNTFSDrives()
{
	wchar_t DriveStrings[1024];
	DWORD len = GetLogicalDriveStrings(1024, DriveStrings);

	std::map<std::wstring, std::pair<reader *, NTFS *>> m;
			
	wchar_t * pDrive = DriveStrings;
	while (*pDrive)
	{
		if (GetDriveType(pDrive) == DRIVE_FIXED)
		{
			std::wstring drive = pDrive;
			if (drive [ drive.length() - 1] == L'\\')
				drive.pop_back();


			reader * r = createvolumereader(drive);
			if (r)
			{
				BOOT_SECTOR * b = bootsector::read(r);
				NTFS * filesystem = init(b);

				if (filesystem)
				{
					SendMessage(GetDlgItem(hWndMain, IDC_COMBODRIVES), CB_ADDSTRING, NULL, (LPARAM)pDrive);

					m[pDrive] = std::pair<reader *, NTFS *>(r, filesystem);

				}
				else
					freevolumereader(r);
			}
			
		}

		pDrive += wcslen(pDrive) + 1;
	}

	return m;
}

bool stop();

void setrange(int Min, int Max);
void update(int Val);


void result(std::wstring filename, std::wstring path, long long date);




void search(std::wstring const filename, std::wstring const content, std::wstring const drive, bool useCPU)
{
	std::string c;
	char * contentANSI = 0;

	if (content.length() > 0)
	{
		contentANSI = (char *)malloc(content.length() + 1);
		if (NULL != contentANSI)
		{
			WideCharToMultiByte(CP_ACP, 0, content.c_str(), content.length() + 1, contentANSI, content.length() + 1, 0, 0);
			c = contentANSI;
		}
	}

	if (useCPU)
	{
		if (canUseCPU)
		{
			if (mftsCPU.find(drive) != mftsCPU.end())
			{
				mftsCPU[drive]->findfiles(filename, c, setrange, update, result, stop);
				mftsCPU[drive]->waitToFinish();
			}
			else
			{
				char InvalidDrive[1024] = {0};
				if (LoadStringA(GetModuleHandle(NULL), IDS_SELECT_NTFS_DRIVE, InvalidDrive, 1024))
					error(InvalidDrive);
			}
		}
	}

	wchar_t Search[1024] ={0};
	LoadString(GetModuleHandle(NULL), IDS_SEARCH, Search, 1024);
	SetDlgItemText(hWndMain, IDSEARCH, Search);

	free(contentANSI);
}




void setrange(int Min, int Max)
{
	SendMessage( GetDlgItem(hWndMain, IDC_PROGRESSBAR), PBM_SETRANGE32, Min, Max);
	SendMessage(GetDlgItem(hWndMain, IDC_PROGRESSBAR), PBM_SETPOS, Min, 0);
}

void update(int Val)
{
	PostMessage(GetDlgItem(hWndMain, IDC_PROGRESSBAR), PBM_DELTAPOS, Val, 0);
}

bool stop()
{
	return StopRequested != 0;
}

void result(std::wstring filename, std::wstring path, long long date)
{

	cs _(resultCs);
	RESULT r = {0};

	wcscpy_s(r.FileName, filename.c_str());
	wcscpy_s(r.FilePath, path.c_str());

	FILETIME ft = *(FILETIME*)&date;
	FILETIME lft;
	FileTimeToLocalFileTime(&ft, &lft);
	SYSTEMTIME st;
	FileTimeToSystemTime(&lft, &st);
	wchar_t Date[256];
	wchar_t Time[256];
	GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, Date, 256);
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, Time, 256);

	wsprintf(r.Date, L"%s %s", Date, Time);

	results.push_back(r);

	LVITEM lvI = {0};
    lvI.pszText   = LPSTR_TEXTCALLBACK;
    lvI.mask      = LVIF_TEXT ;
	lvI.iItem  = results.size() - 1;
	ListView_InsertItem(GetDlgItem(hWndMain, IDC_LISTFILES), &lvI);


}

void deinit()
{
	bmh::binary::deinit();
}
