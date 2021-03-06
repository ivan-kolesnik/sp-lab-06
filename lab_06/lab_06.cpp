#include "stdafx.h"
#include "lab_06.h"

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <shobjidl.h>
#include "atlstr.h"

#define START_BUTTON 3001
#define FOLDER_BUTTON 3002

#define ID_EDIT 51
#define ID_FOLDER_EDIT 52

#define ID_RICH_TEXT 1001
#define ID_SEARH_TEXT 1002

#define WM_MY_DATA WM_USER + 1

#define SEARCH_DELAY 50

typedef struct Data
{
    HWND WindowHwnd;
    LPWSTR filename;
    WCHAR directoryPath[MAX_PATH];
} MYDATA, *PMYDATA;

TCHAR szWindowClass[] = _T("DesktopApp");
TCHAR szTitle[] = _T("lab_06");

HINSTANCE hInst;

LPWSTR **sharedArr;
int sharedArrayIndex;

HANDLE ghMutex;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI ThreadRoutineFunction(LPVOID lpParam);
void FindFile(PMYDATA pData);
void FileDiag();
void appendTextToEdit(HWND hEdit, LPCWSTR newText);

int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAB06));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(
            NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL
        );

        return 1;
    }

    hInst = hInstance;

    HWND hWnd = CreateWindow(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        600, 350,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd)
    {
        MessageBox(
            NULL,
            _T("Call to CreateWindow failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL
        );

        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hWndEdit;
    static HWND hWndTextBottom;
    static HWND hWndFolder;
    static HWND hWndRichTextBox;
    static HWND hWndButton;

    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
        case WM_CREATE:
        {
            HWND hWndTextTop = CreateWindow(
                L"Static",
                L"Enter file name and folder:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 20,
                240, 20,
                hWnd,
                (HMENU)1,
                NULL,
                NULL
            );

            HWND hWndTextTop1 = CreateWindow(
                L"Static",
                L"Found files:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                300, 20,
                260, 20,
                hWnd,
                (HMENU)1,
                NULL,
                NULL
            );

            hWndEdit = CreateWindow(
                L"Edit",
                NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT | ES_LOWERCASE,
                20, 60,
                240, 20,
                hWnd,
                (HMENU)ID_EDIT,
                NULL,
                NULL
            );

            hWndFolder = CreateWindow(
                L"Edit",
                NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT | ES_LOWERCASE | ES_READONLY,
                20, 90,
                215, 20,
                hWnd,
                (HMENU)ID_FOLDER_EDIT,
                NULL,
                NULL
            );

            HWND hWndButtonFolder = CreateWindow(
                L"Button",
                L"...",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_FLAT | BS_NOTIFY,
                240, 90,
                20, 20,
                hWnd,
                (HMENU)FOLDER_BUTTON,
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL
            );

            hWndButton = CreateWindow(
                L"Button",
                L"Search",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_FLAT | BS_NOTIFY,
                140, 140,
                120, 30,
                hWnd,
                (HMENU)START_BUTTON,
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL
            );

            hWndTextBottom = CreateWindow(
                L"Static",
                L"",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 260,
                540, 40,
                hWnd,
                (HMENU)ID_SEARH_TEXT,
                NULL,
                NULL
            );

            hWndRichTextBox = CreateWindow(
                L"Edit",
                NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
                300, 60,
                260, 180,
                hWnd,
                (HMENU)ID_RICH_TEXT,
                NULL,
                NULL
            );

            break;
        }
        case WM_PAINT:
        {
            hdc = BeginPaint(hWnd, &ps);

            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW));

            EndPaint(hWnd, &ps);

            break;
        }
        case WM_COMMAND:
        {
            switch (wParam)
            {
                case START_BUTTON:
                {
                    int text_length = GetWindowTextLengthW(hWndEdit) + 1;

                    LPWSTR filename = new WCHAR[text_length];
                    GetWindowTextW(hWndEdit, filename, text_length);

                    SetDlgItemText(hWnd, ID_SEARH_TEXT, L"");
                    SetWindowText(hWndRichTextBox, L"");

                    PMYDATA pData = (PMYDATA)HeapAlloc(
                        GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        sizeof(MYDATA)
                    );

                    pData->WindowHwnd = hWnd;
                    pData->filename = filename;
                    GetCurrentDirectory(sizeof(pData->directoryPath), pData->directoryPath);

                    sharedArrayIndex = 0;
                    sharedArr = new LPWSTR*[SHRT_MAX];
                    ghMutex = CreateMutex(NULL, FALSE, NULL);

                    FindFile(pData);

                    break;
                }
                case FOLDER_BUTTON:
                {
                    FileDiag();
                    WCHAR dir[MAX_PATH];

                    GetCurrentDirectory(sizeof(dir), dir);
                    SetWindowText(hWndFolder, dir);

                    break;
                }
                default:
                {
                    break;
                }
            }

            break;
        }
        case WM_MY_DATA:
        {
            int foundFileIndex = (int)lParam;

            SetDlgItemText(hWnd, ID_SEARH_TEXT, sharedArr[foundFileIndex][0]);

            if (StrStrIW(sharedArr[foundFileIndex][0], L"?") != NULL)
            {
                int outLength = GetWindowTextLength(hWndRichTextBox) + lstrlen(sharedArr[foundFileIndex][0]) + 4;
                WCHAR * buf = (WCHAR *)GlobalAlloc(GPTR, outLength * sizeof(WCHAR));

                _tcscat_s(buf, outLength, sharedArr[foundFileIndex][0]);
                _tcscat_s(buf, outLength, L"\r\n");
                appendTextToEdit(hWndRichTextBox, buf);

                GlobalFree(buf);
            }

            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            delete[] sharedArr;

            break;
        }
        default:
        {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }

    return 0;
}

DWORD WINAPI ThreadRoutineFunction(LPVOID lpParam)
{
    PMYDATA pData = (PMYDATA)lpParam;
    WIN32_FIND_DATA foundFileData;
    WCHAR directory[MAX_PATH];

    wcsncpy_s(directory, pData->directoryPath, MAX_PATH);
    wcsncat_s(directory, L"\\*", 3);

    HANDLE hFind = INVALID_HANDLE_VALUE;

    hFind = FindFirstFile(directory, &foundFileData);

    if (INVALID_HANDLE_VALUE != hFind)
    {
        do
        {
            if (foundFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (wcscmp(foundFileData.cFileName, L".") != 0 && wcscmp(foundFileData.cFileName, L"..") != 0)
                {
                    //Create new thread for recursion search
                    PMYDATA pDataCopy = (PMYDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MYDATA));

                    pDataCopy->filename = pData->filename;
                    pDataCopy->WindowHwnd = pData->WindowHwnd;
                    wcsncpy_s(pDataCopy->directoryPath, pData->directoryPath, MAX_PATH);
                    wcsncat_s(pDataCopy->directoryPath, L"\\", 2);
                    wcsncat_s(pDataCopy->directoryPath, foundFileData.cFileName, MAX_PATH);

                    FindFile(pDataCopy);
                }
            }

            DWORD dwWaitResult = WaitForSingleObject(ghMutex, INFINITE);

            switch (dwWaitResult)
            {
                // The thread got ownership of the mutex
                case WAIT_OBJECT_0:
                {
                    __try
                    {
                        sharedArr[sharedArrayIndex] = new LPWSTR[MAX_PATH];
                        sharedArr[sharedArrayIndex][0] = new WCHAR[260];

                        if (StrStrIW(foundFileData.cFileName, pData->filename) != NULL)
                        {
                            wcsncat_s(foundFileData.cFileName, L"?", 1);
                        }

                        if (
                            wcscmp(foundFileData.cFileName, L".") != 0
                            && wcscmp(foundFileData.cFileName, L"..") != 0
                            )
                        {
                            WCHAR buf[MAX_PATH];
                            wcsncpy_s(buf, pData->directoryPath, MAX_PATH);
                            wcsncat_s(buf, L"\\", 2);
                            wcsncat_s(buf, foundFileData.cFileName, MAX_PATH);
                            swprintf_s(sharedArr[sharedArrayIndex][0], 260, buf);

                            int index = sharedArrayIndex;
                            PostMessage(pData->WindowHwnd, WM_MY_DATA, 0, (LPARAM)index);
                            Sleep(SEARCH_DELAY);

                            sharedArrayIndex++;
                        }
                    }
                    __finally {
                        // Release ownership of the mutex object
                        if (!ReleaseMutex(ghMutex))
                        {
                            MessageBox(NULL, L"ReleaseMutex failed", L"Error", MB_OK);
                        }
                    }

                    break;
                }
                // The thread got ownership of an abandoned mutex
                case WAIT_ABANDONED:
                {
                    return FALSE;
                }
            }
        } while (FindNextFile(hFind, &foundFileData) != 0);

        FindClose(hFind);
    }
    else
    {
        MessageBox(NULL, L"FindFirstFile failed", L"Error", MB_OK);
        return 0;
    }

    return 0;
}

void FindFile(PMYDATA pData)
{
    DWORD IDThread;

    CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)ThreadRoutineFunction,
        pData,
        0,
        &IDThread
    );
}

void FileDiag()
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (SUCCEEDED(hr))
    {
        // Create the FileOpenDialog object.
        IFileDialog *file_dialog = NULL;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&file_dialog));

        if (SUCCEEDED(hr))
        {
            DWORD dwOptions;
            if (SUCCEEDED(file_dialog->GetOptions(&dwOptions)))
            {
                file_dialog->SetOptions(dwOptions | FOS_PICKFOLDERS);
            }
            // Show the Open dialog box.
            hr = file_dialog->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem *pItem;
                hr = file_dialog->GetResult(&pItem);

                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        if (!SetCurrentDirectory(pszFilePath))
                        {
                            MessageBox(NULL, L"SetCurrentDirectory failed", L"Error", MB_OK);
                        }

                        CoTaskMemFree(pszFilePath);
                    }

                    pItem->Release();
                }
            }

            file_dialog->Release();
        }

        CoUninitialize();
    }
}

void appendTextToEdit(HWND hEdit, LPCWSTR newText)
{
    int TextLen = SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0);

    SendMessage(hEdit, EM_SETSEL, (WPARAM)TextLen, (LPARAM)TextLen);
    SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)newText);
}
