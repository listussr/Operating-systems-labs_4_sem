#include <WinSock2.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996) // äëÿ inet_addr

#define INT_SIZE sizeof(int)
#define ULL_SIZE sizeof(size_t)
#define SECOND 1000
#define MILLISECOND 1
#define ON_BUTTON_CLICKED 1

HINSTANCE hInst;
static TCHAR szTitle[] = _T("Клиент");

namespace fglv
{
    static HWND hTimer;
    static HWND hMessage;
    static HWND hBtn;
}

namespace glv
{
    std::vector<SOCKET> Sockets;
    int client_number = 0;
    std::ofstream out;
}

void Mistake(const wchar_t* message)
{
    MessageBox(NULL, message, L"Ошибка", NULL);
    for (SOCKET socket : glv::Sockets)
    {
        closesocket(socket);
    }
    WSACleanup();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/// <summary>
/// Функция установки цвета окна в приложении
/// </summary>
/// <returns>color id -> int</returns>
int GetColor()
{
    srand(time(NULL));
    int left_border = 0, right_border = 7;
    int colorId[] = { 3, 6, 9, 12, 18, 20, 23, 24 };
    int id = rand() % right_border;
    return colorId[id];
}


WNDCLASSEX InitializeWNDCLASSEX(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + GetColor());
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"Client";
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    return wcex;
}

MSG HandleMSG()
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg;
}

void InitializeWSA()
{
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0)
    {
        MessageBox(NULL, L"Ошибка инициализации библиотеки", L"Ошибка", NULL);
        exit(1);
    }
}

void MainWndAddWidgets(HWND hWnd)
{
    CreateWindow(L"static", L"Клиент", WS_VISIBLE | WS_CHILD | ES_CENTER, 5, 5, 720, 20, hWnd, NULL, NULL, NULL);
    fglv::hTimer = CreateWindow(L"EDIT", NULL, WS_VISIBLE | WS_CHILD | ES_READONLY | WS_VSCROLL | ES_MULTILINE | WS_BORDER, 5, 35, 710, 220, hWnd, NULL, NULL, NULL);
    fglv::hMessage = CreateWindow(L"EDIT", L"Время жизни", WS_VISIBLE | WS_CHILD | ES_READONLY | WS_BORDER, 5, 270, 710, 20, hWnd, NULL, NULL, NULL);
}

int get_lifetime(SOCKET Connection)
{
    int lifetime;
    recv(Connection, (char*)&lifetime, sizeof(int), NULL);
    return lifetime;
}

void custom_output(int lifetime)
{
    std::wstring strtime(L"Время жизни: ");
    std::wstring remaining_time(L"Оставшееся время: ");
    strtime += std::to_wstring(lifetime) + L" секунд";
    const wchar_t* wstr = strtime.c_str();
    SendMessage(fglv::hMessage, WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(wstr));
    strtime.clear();
    while (lifetime--)
    {
        strtime = remaining_time + std::to_wstring(lifetime) + L" секунд";
        SendMessage(fglv::hTimer, WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(strtime.c_str()));
        strtime.clear();
        Sleep(1 * SECOND);
    }
    SendMessage(fglv::hTimer, WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(L"Вы будете отключены через 5 секунд"));
    Sleep(5 * SECOND);
}

void client_life(int lifetime)
{
    // выводим время в консоль и ставим время жизни программы
    if (lifetime > 0)
    {
        custom_output(lifetime);
    }
    else
    {
        // выставляем максимально возможное время жизни в секундах
        std::wstring strtime(L"Время жизни: ");
        std::wstring remaining_time(L"Оставшееся время: <eternity>");
        strtime += L"<eternity>";
        const wchar_t* wstr = strtime.c_str();
        SendMessage(fglv::hMessage, WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(wstr));
        SendMessage(fglv::hTimer, WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(remaining_time.c_str()));
        Sleep((DWORD)(pow(2, sizeof(DWORD) * 8) - 1) * SECOND);
    }
    exit(0);
}


void ProcessServer()
{
    InitializeWSA();
    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;

    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);

    SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL);
    // проверяем на подключение к серверу
    if (connect(Connection, (SOCKADDR*)&addr, sizeof(addr))) 
    {
        Mistake(L"Ошибка подключения");
    }
    std::cout << "Connected!" << '\n';
    int lifetime = get_lifetime(Connection);
    client_life(lifetime);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wcex = InitializeWNDCLASSEX(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL, _T("Call to RegisterClassEx failed!"), _T("Windows Desktop Guided Tour"), NULL);
        return 1;
    }
    hInst = hInstance;
    HWND hWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, L"Client", szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 740, 350, NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ProcessServer, NULL, NULL, NULL);
    MSG msg = HandleMSG();
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_CREATE:
    {
        MainWndAddWidgets(hWnd);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}