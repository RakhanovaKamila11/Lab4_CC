#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <string>

#pragma comment(lib, "WS2_32.lib")

#define SERV_PORT 5000
#define WSA_ACCEPT (WM_USER + 0)
#define WSA_NETEVENT (WM_USER + 1)
#define MAX_CLIENTS 3

HINSTANCE hInst;
HWND hWindow;
HWND hwndEdit;

WSADATA wsaData;
WORD wVersionRequested = MAKEWORD(1, 1);
SOCKET srv_socket = INVALID_SOCKET;

int ClientNum = -1;
SOCKET sock[MAX_CLIENTS];
SOCKADDR_IN sockaddr[MAX_CLIENTS];

char mess[4096] = "";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"ServerClass";

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"ServerClass", L"Server", WS_OVERLAPPEDWINDOW,
        400, 100, 500, 400, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;

    hWindow = hWnd;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

void ServerStart(HWND hWnd) {
    if (srv_socket != INVALID_SOCKET) {
        MessageBoxA(hWnd, "Server is already running!", "Info", MB_OK | MB_ICONINFORMATION);
        return;
    }

    srv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_socket == INVALID_SOCKET) {
        MessageBoxA(hWnd, "Socket creation error!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    SOCKADDR_IN srv_address;
    srv_address.sin_family = AF_INET;
    srv_address.sin_port = htons(SERV_PORT);
    srv_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(srv_socket, (LPSOCKADDR)&srv_address, sizeof(srv_address)) == SOCKET_ERROR) {
        closesocket(srv_socket);
        srv_socket = INVALID_SOCKET;
        MessageBoxA(hWnd, "Port binding error!", "Error", MB_OK | MB_ICONSTOP);
        return;
    }

    if (listen(srv_socket, 5) == SOCKET_ERROR) {
        closesocket(srv_socket);
        srv_socket = INVALID_SOCKET;
        MessageBoxA(hWnd, "Listen error!", "Error", MB_OK);
        return;
    }

    WSAAsyncSelect(srv_socket, hWnd, WSA_ACCEPT, FD_ACCEPT);
    strcat_s(mess, "Server started. Waiting for clients...\r\n");
    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
}

void ServerStop(HWND hWnd) {
    WSAAsyncSelect(srv_socket, hWnd, 0, 0);
    if (srv_socket != INVALID_SOCKET) {
        closesocket(srv_socket);
        srv_socket = INVALID_SOCKET;
    }
    for (int i = 0; i <= ClientNum; i++) {
        if (sock[i] != INVALID_SOCKET) {
            closesocket(sock[i]);
            sock[i] = INVALID_SOCKET;
        }
    }
    ClientNum = -1;
    strcat_s(mess, "Server stopped.\r\n");
    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
}

BOOL AcceptClient(int j) {
    int sockaddr_len = sizeof(sockaddr[j]);
    sock[j] = accept(srv_socket, (LPSOCKADDR)&sockaddr[j], &sockaddr_len);
    if (sock[j] != INVALID_SOCKET) {
        if (!WSAAsyncSelect(sock[j], hWindow, WSA_NETEVENT, FD_READ | FD_CLOSE))
            return TRUE;
    }
    return FALSE;
}

void WndProc_OnWSAAccept(HWND hWnd, LPARAM lParam) {
    if (WSAGETSELECTERROR(lParam)) return;

    if (ClientNum >= MAX_CLIENTS - 1) {
        MessageBoxA(hWnd, "Client limit reached (max 3)!", "Error", MB_OK);
        return;
    }

    ClientNum++;
    if (!AcceptClient(ClientNum)) {
        MessageBoxA(hWnd, "Client connection error!", "Error", MB_OK);
        ClientNum--;
        return;
    }

    char szTemp[256];
    sprintf_s(szTemp, "Client %d added (IP=%s, Port=%u)\r\n", ClientNum + 1,
        inet_ntoa(sockaddr[ClientNum].sin_addr), htons(sockaddr[ClientNum].sin_port));

    strcat_s(mess, szTemp);
    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
}

void WndProc_OnWSANetEvent(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    char szTemp[512];
    int Number = -1;

    for (int i = 0; i <= ClientNum; i++) {
        if (sock[i] == (SOCKET)wParam) {
            Number = i;
            break;
        }
    }
    if (Number == -1) return;

    if (WSAGETSELECTEVENT(lParam) == FD_READ) {
        int rc = recv((SOCKET)wParam, szTemp, sizeof(szTemp) - 1, 0);
        if (rc > 0) {
            szTemp[rc] = '\0';
            char buf[1024];
            sprintf_s(buf, "\r\nData from Client %d:\r\n%s\r\n", Number + 1, szTemp);
            strcat_s(mess, buf);
            SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
        }
    }

    if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) {
        WSAAsyncSelect(sock[Number], hWindow, 0, 0);
        closesocket(sock[Number]);
        sock[Number] = INVALID_SOCKET;

        char szTempClose[256];
        sprintf_s(szTempClose, "\r\nClient %d disconnected.\r\n", Number + 1);
        strcat_s(mess, szTempClose);
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        hwndEdit = CreateWindowA("EDIT", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 480, 340, hWnd, NULL, hInst, NULL);

        if (WSAStartup(wVersionRequested, &wsaData) != 0) {
            MessageBoxA(hWnd, "WSAStartup Error", "ERROR", MB_OK);
            return FALSE;
        }

        sprintf_s(mess, "Server GUI Window.\r\nWinSock Status: %s\r\n\r\n", wsaData.szSystemStatus);
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);

        ServerStart(hWnd);
        break;

    case WSA_ACCEPT:
        WndProc_OnWSAAccept(hWnd, lParam);
        break;

    case WSA_NETEVENT:
        WndProc_OnWSANetEvent(hWnd, wParam, lParam);
        break;

    case WM_DESTROY:
        ServerStop(hWnd);
        WSACleanup();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}