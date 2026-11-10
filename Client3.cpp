#include <windows.h>
#include <winsock.h>
#include <stdio.h>

#pragma comment(lib, "WS2_32.lib")

#define SERV_PORT 5000
#define WSA_NETEVENT (WM_USER + 1)
#define ID_BTN_SEND 1003

HINSTANCE hInst;
HWND hwndInput, btnSend;
SOCKET cln_socket = INVALID_SOCKET;
SOCKADDR_IN dest_sin;
char szHostName[128] = "localhost";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"Client3Class";

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"Client3Class", L"Client 3", WS_OVERLAPPEDWINDOW,
        920, 100, 420, 320, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

BOOL SetConnection(HWND hWnd) {
    cln_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cln_socket == INVALID_SOCKET) return FALSE;

    PHOSTENT phe = gethostbyname(szHostName);
    if (!phe) return FALSE;

    dest_sin.sin_family = AF_INET;
    dest_sin.sin_port = htons(SERV_PORT);
    memcpy(&(dest_sin.sin_addr), phe->h_addr, phe->h_length);

    if (connect(cln_socket, (PSOCKADDR)&dest_sin, sizeof(dest_sin)) == SOCKET_ERROR) {
        closesocket(cln_socket);
        cln_socket = INVALID_SOCKET;
        return FALSE;
    }

    WSAAsyncSelect(cln_socket, hWnd, WSA_NETEVENT, FD_READ | FD_CLOSE);
    return TRUE;
}

void SendCustomText(HWND hWnd) {
    if (cln_socket == INVALID_SOCKET) {
        if (!SetConnection(hWnd)) {
            MessageBoxA(hWnd, "Failed to connect to server!", "Error", MB_OK | MB_ICONERROR);
            return;
        }
    }

    char szBuf[1024];
    GetWindowTextA(hwndInput, szBuf, sizeof(szBuf));

    if (strlen(szBuf) == 0) {
        MessageBoxA(hWnd, "Please enter text before sending!", "Warning", MB_OK | MB_ICONWARNING);
        return;
    }

    if (send(cln_socket, szBuf, (int)strlen(szBuf), 0) != SOCKET_ERROR) {
        MessageBoxA(hWnd, "Text successfully sent to server!", "Success", MB_OK | MB_ICONINFORMATION);
    }
    else {
        MessageBoxA(hWnd, "Send error!", "Error", MB_OK | MB_ICONERROR);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        hwndInput = CreateWindowA("EDIT", "Enter your text here: ",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_BORDER,
            10, 10, 380, 180, hWnd, NULL, hInst, NULL);

        btnSend = CreateWindowA("BUTTON", "Send Custom Text",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 205, 220, 35, hWnd, (HMENU)ID_BTN_SEND, hInst, NULL);

        WSADATA wsaData;
        WSAStartup(MAKEWORD(1, 1), &wsaData);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_SEND) {
            SendCustomText(hWnd);
        }
        break;

    case WM_DESTROY:
        if (cln_socket != INVALID_SOCKET) closesocket(cln_socket);
        WSACleanup();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}