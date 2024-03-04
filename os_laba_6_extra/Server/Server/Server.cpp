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
static TCHAR szTitle[] = _T("Сервер");

namespace fglv
{
    static HWND hChat;
    static HWND hEntry;
    static HWND hBtn;
}

namespace glv
{
    bool connected = true;
    std::vector<SOCKET> Sockets;
    int client_number = 0;
    std::ofstream out;
}

enum messagetype
{
    CONNECTING_FLAG = 1, // флаг подключения отключения 
    DISCONNECTING_FLAG,
    TIME_SEND, // само сообщение
};


/// <summary>
/// Функция логирования в файл всех событий, обрабатываемых на сервере
/// </summary>
void Logging(int client_number, int lifetime, messagetype operation_flag)
{
    auto time = std::chrono::system_clock::now();
    time_t cur_time = std::chrono::system_clock::to_time_t(time);
    glv::out << std::ctime(&cur_time) << '\n';
    switch (operation_flag)
    {
    case(CONNECTING_FLAG):
        glv::out << "{ Client #" << client_number << " was connected to the chat }" << '\n';
        break;
    case(DISCONNECTING_FLAG):
        glv::out << "{ Client #" << client_number << " was disconnected from the chat }" << '\n';
        break;
    case(TIME_SEND):
        if (lifetime > 0)
        {
            glv::out << "<" << '\n' << "Client #" << client_number << ". Lifetime: " << lifetime << " seconds. >" << '\n';
        }
        else
        {
            glv::out << "<" << '\n' << "Client #" << client_number << ". Lifetime: " << " eternity. >" << '\n';
        }
        break;
    }
}

/// <summary>
/// функция подключения рандома в функцию rand
/// </summary>
void randomise()
{
    srand(time(NULL));
}


bool is_infinity()
{
    int min = 0;
    int max = 3;
    int result = min + rand() % (max - min + 1);
    return !result;
}

int generate_lifetime()
{
    if (is_infinity())
    {
        return -1;
    }
    int min = 1;
    int max = 30;
    int time = min + rand() % (max - min + 1);
    return time;
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

void AddText(HWND eWnd, const TCHAR* text)
{
    SendMessage(eWnd, EM_SETSEL, -1, -1);
    SendMessage(eWnd, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text));
}

void ProcessEntry()
{
    int length = GetWindowTextLength(fglv::hEntry);
    wchar_t* wmessage = new wchar_t[length + 1];
    GetWindowText(fglv::hEntry, wmessage, length + 1);
    SendMessage(fglv::hEntry, WM_SETTEXT, WPARAM(0), WPARAM(L""));
    if (glv::client_number)
    {
        AddText(fglv::hChat, L"Подождите 10 секунд. Процесс завершается.");
        AddText(fglv::hChat, L"\n");
        return;
    }
    try 
    {
        for (int i = 0; i < length; ++i)
        {
            wchar_t t = wmessage[i];
            if (t != L'0' && t != L'1' && t != L'2' && t != L'3' && t != L'4' && t != L'5' && t != L'6' && t != L'7' && t != L'8' && t != L'9')
                throw std::invalid_argument("");
        }
        int number = _wtoi(wmessage);
        if (number < 3 || number > 15)
        {
            AddText(fglv::hChat, L"Недопустимое количество пользователей: ");
            AddText(fglv::hChat, wmessage);
            AddText(fglv::hChat, L"\n");
            AddText(fglv::hChat, L"Повторите ввод! ");
            AddText(fglv::hChat, L"\n");
        }
        else
        {
            AddText(fglv::hChat, L"Количество пользователей: ");
            AddText(fglv::hChat, wmessage);
            AddText(fglv::hChat, L"\n");
            glv::client_number = number;
        }
    }
    catch (...)
    {
        AddText(fglv::hChat, L"Недопустимый ввод! ");
        AddText(fglv::hChat, L"Повторите попытку! ");
        AddText(fglv::hChat, L"\n");
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

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
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 22);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"Server";
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
    CreateWindow(L"static", L"Сервер", WS_VISIBLE | WS_CHILD | ES_CENTER, 5, 5, 720, 20, hWnd, NULL, NULL, NULL);
    fglv::hChat = CreateWindow(L"EDIT", NULL, WS_VISIBLE | WS_CHILD | ES_READONLY | WS_VSCROLL | ES_MULTILINE, 5, 35, 710, 220, hWnd, NULL, NULL, NULL);
    fglv::hEntry = CreateWindow(L"EDIT", L"3", WS_VISIBLE | WS_CHILD | WS_BORDER, 5, 270, 600, 20, hWnd, NULL, NULL, NULL);
    fglv::hBtn = CreateWindow(L"button", L"Подтвердить", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 610, 270, 100, 20, hWnd, (HMENU)ON_BUTTON_CLICKED, NULL, NULL);
}

void start_client_session()
{
    //const char* navigation_command = "cd C:\\University\\kurs_2\\semestr_2\\systems\\os_laba_6_extra\\Client\\x64\\Debug\\Client.exe";
    //const char* run_command = "start C:\\University\\kurs_2\\semestr_2\\systems\\os_laba_6_extra\\Client\\x64\\Debug\\Client.exe";
    // проверка на случай непредвиденных обстоятельств
    //int navigation_failure = system(navigation_command);
    //if (navigation_failure)
    //{
    //    Mistake(L"Ошибка вызова клиента!");
    //}
    // запускаем собранную программу у клиента
    //system(run_command);
    //STARTUPINFO info = { sizeof(info) };
    //PROCESS_INFORMATION processInfo;
    //CreateProcess(L"C:\\University\\kurs_2\\semestr_2\\systems\\os_laba_6_extra\\Client\\x64\\Debug\\Client.exe", NULL, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    TCHAR szCmdLine[] = _T("C:\\University\\kurs_2\\semestr_2\\systems\\os_laba_6_extra\\Client\\x64\\Debug\\Client.exe");
    CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
}

void connection(SOCKET sListen, SOCKADDR_IN addr, int sizeofaddr, int amount_of_clients)
{
    SOCKET newConnection;
    for (int i = 0; i < amount_of_clients; ++i)
    {
        start_client_session();
        newConnection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);
        if (!newConnection)
        {
            Mistake(L"Ошибка подключения");
        }
        else
        {
            Logging(i + 1, NULL, CONNECTING_FLAG);
            int lifetime = generate_lifetime();
            Logging(i + 1, lifetime, TIME_SEND);
            send(newConnection, (char*)&lifetime, sizeof(int), NULL);
        }
        glv::Sockets.push_back(newConnection);
    }
    Sleep(10 * SECOND);
    glv::client_number = 0;
    for (SOCKET socket : glv::Sockets)
    {
        closesocket(socket);
    }
}

void ProcessServer()
{
    randomise();
    InitializeWSA();
    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;

    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);
    while (glv::connected)
    {
        while (!glv::client_number)
        {
            Sleep(1 * SECOND);
        }

        connection(sListen, addr, sizeofaddr, glv::client_number);
    }
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
    HWND hWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, L"Server", szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 740, 350, NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    glv::out.open("Logger.txt", std::ios::binary);
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
    case WM_COMMAND:
        switch (wParam)
        {
        case(ON_BUTTON_CLICKED):
        {
            ProcessEntry();
            break;
        }
        default: break;
        }
        break;
    case WM_DESTROY:
        glv::connected = false;
        glv::out.close();
        WSACleanup();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}