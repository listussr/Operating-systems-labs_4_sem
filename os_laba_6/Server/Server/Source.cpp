#include<WinSock2.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996) // для inet_addr

#define INT_SIZE sizeof(int)
#define ULL_SIZE sizeof(size_t)
#define SECOND 1000
#define MILLISECOND 1

/// <summary>
/// пространство имён с переменными для работы с графическим интерфейсом
/// </summary>
namespace fglv
{
    static HWND hChat;
}

typedef std::pair<std::wstring, const wchar_t*> msg_info;

HINSTANCE hInst;
static TCHAR szTitle[] = _T("Чат");

/// <summary>
/// пространство имён с переменными для клиент-серверного взаимодействия
/// </summary>
namespace glv
{
    HANDLE hSemaphore;
    std::vector<SOCKET> Sockets;
    const DWORD max_users = 2;
    std::vector<msg_info> History;
    int actual_clients[2];
    bool process_flag = true;
    bool server_loaded = false;
    int closed_client;
    int current_users = 0;
}

void randomise()
{
    srand(time(NULL));
}

///////////////////////////////////////////////////////////////////////
//                  taskkill /F /IM app.exe
///////////////////////////////////////////////////////////////////////

/// <summary>
/// Функция перевода ASCII строки в расширенную строку
/// </summary>
/// <param name="str"></param>
/// <returns>str in UTF_8</returns>
std::wstring StrToWstr(std::string str)
{
    wchar_t* wszTo = new wchar_t[str.length() + 1];
    wszTo[str.size()] = L'\0';
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wszTo, (int)str.length());
    std::wstring wstr(wszTo);
    delete[] wszTo;
    return wstr;
}

/// <summary>
/// Функция перевода расширенной строки в ASCII строку
/// </summary>
/// <param name="wstr"></param>
/// <returns>wstr in ASCII</returns>
std::string WstrToStr(std::wstring wstr)
{
    char* szTo = new char[wstr.length() + 1];
    szTo[wstr.size()] = '\0';
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, szTo, (int)wstr.length(), NULL, NULL);
    std::string str(szTo);
    delete[] szTo;
    return str;
}

/// <summary>
/// Функция очистки динамической памяти в истории сообщений
/// </summary>
void CleanHistoryMemory()
{
    for (msg_info message : glv::History)
    {
        delete[] message.second;
    }
}

/// <summary>
/// Функция обработки ошибок
/// </summary>
/// <param name="message"></param>
void Mistake(const wchar_t* message)
{
    MessageBox(NULL, message, L"Ошибка", NULL);
    for (SOCKET socket : glv::Sockets)
    {
        closesocket(socket);
    }
    CleanHistoryMemory();
    WSACleanup();
    //exit(1);
}

/// <summary>
/// Функция фиксации нового клиента
/// </summary>
/// <param name="client_number"></param>
void update_users(int client_number)
{
    if (glv::actual_clients[0] == glv::closed_client)
    {
        glv::actual_clients[0] = glv::actual_clients[1];
        glv::actual_clients[1] = client_number;

    }
    else
    {
        glv::actual_clients[1] = client_number;
    }
}

/// <summary>
/// Функция добавления сообщения на экран
/// </summary>
/// <param name="eWnd"></param>
/// <param name="text"></param>
void AddText(HWND eWnd, const TCHAR* text)
{
    SendMessage(eWnd, EM_SETSEL, -1, -1);
    SendMessage(eWnd, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text));
}

/// <summary>
/// Функция пересылки сообщения
/// </summary>
/// <param name="message"></param>
/// <param name="size_of_message"></param>
/// <param name="client_number"></param>
void ResendMessage(const wchar_t* wmessage, int size_of_message, int client_number)
{
    SOCKET current_connection = glv::Sockets[glv::actual_clients[(glv::actual_clients[0] != client_number) ? 0 : 1]];

    AddText(fglv::hChat, L"---------------\n");
    AddText(fglv::hChat, L"Клиент, который получит сообщение: ");
    AddText(fglv::hChat, std::to_wstring(client_number).c_str());
    AddText(fglv::hChat, L" | подключение ");
    AddText(fglv::hChat, std::to_wstring(current_connection).c_str());

    for (int i = 0; i < 2; ++i)
    {
        AddText(fglv::hChat, L"\n");
        AddText(fglv::hChat, L"Users[");
        AddText(fglv::hChat, std::to_wstring(i).c_str());
        AddText(fglv::hChat, L"]: ");
        AddText(fglv::hChat, std::to_wstring(glv::actual_clients[i]).c_str());
        AddText(fglv::hChat, L"\n");
        AddText(fglv::hChat, L"Connections[");
        AddText(fglv::hChat, std::to_wstring(i).c_str());
        AddText(fglv::hChat, L"]: ");
        AddText(fglv::hChat, std::to_wstring(glv::Sockets[glv::actual_clients[i]]).c_str());
    }

    AddText(fglv::hChat, L"\n");
    AddText(fglv::hChat, L"Size: ");
    AddText(fglv::hChat, std::to_wstring(size_of_message).c_str());

    send(current_connection, (char*)&client_number, INT_SIZE, NULL);
    send(current_connection, (char*)&size_of_message, INT_SIZE, NULL);

    AddText(fglv::hChat, L"\n");
    AddText(fglv::hChat, L"Message: ");
    AddText(fglv::hChat, wmessage);
    
    std::wstring wstr(wmessage);
    std::string str = WstrToStr(wstr);
    const char* message = str.c_str();

    send(current_connection, message, size_of_message, NULL);

    AddText(fglv::hChat, L"---------------\n");
}

/// <summary>
/// Функция обработки сообщения
/// </summary>
/// <param name="message"></param>
/// <param name="message_length"></param>
/// <param name="client_number"></param>
/// <param name="connection_flag"></param>
void MessageHandler(const wchar_t* message, int message_length, int client_number, bool& connection_flag)
{
    // проверяем на соответствие команде о выходе
    if (!wcscmp(message, L"--leave\n"))
    {
        --glv::current_users;
        connection_flag = false;
    }
    // записываем сообщение в историю
    std::wstring name = L"Пользователь #";
    name += std::to_wstring(client_number);
    AddText(fglv::hChat, name.c_str());
    AddText(fglv::hChat, L"\n");

    AddText(fglv::hChat, L"Размер сообщения: ");
    AddText(fglv::hChat, std::to_wstring(message_length).c_str());
    AddText(fglv::hChat, L"\n");

    AddText(fglv::hChat, L"Message: ");
    AddText(fglv::hChat, message);
    AddText(fglv::hChat, L"\n");
    wchar_t* history_msg = new wchar_t[message_length + 1];
    wcscpy(history_msg, message);
    history_msg[message_length] = L'\0';
    msg_info msg(name, history_msg);
    glv::History.push_back(msg);
    AddText(fglv::hChat, L"Последнее сообщение в истории: ");
    AddText(fglv::hChat, (glv::History[glv::History.size() - 1]).first.c_str());
    AddText(fglv::hChat, L" ");
    AddText(fglv::hChat, (glv::History[glv::History.size() - 1]).second);
    AddText(fglv::hChat, L"\n");
    // отправляем сообщение другому пользователю
    if(glv::current_users)
        ResendMessage(message, message_length, client_number);
}

/// <summary>
/// Функция отправки истории новому пользователю
/// </summary>
/// <param name="client_number"></param>
void SendHistory(int client_number)
{
    // выбираем какому из 2 активных пользователей отправить сообщение
    SOCKET current_connection = glv::Sockets[glv::actual_clients[(glv::actual_clients[0] != client_number) ? 1 : 0]];
    // отправляем кол-во сообщений из истории
    size_t history_size = glv::History.size();
    send(current_connection, (char*)&history_size, ULL_SIZE, NULL);
    for (msg_info msg : glv::History)
    {
        // отправляем все сообщения из истории
        std::string namestr = WstrToStr(msg.first);
        const char* name = namestr.c_str();
        int size_of_name = msg.first.size();
        send(current_connection, (char*)&size_of_name, INT_SIZE, NULL);
        send(current_connection, name, size_of_name, NULL);

        std::wstring messagewstr(msg.second);
        std::string messagestr = WstrToStr(messagewstr);
        const char* message = messagestr.c_str();
        int size_of_message = messagestr.size();
        send(current_connection, (char*)&size_of_message, INT_SIZE, NULL);
        send(current_connection, message, size_of_message, NULL);
    }
}

/// <summary>
/// Функция обработки клиента чата
/// </summary>
/// <param name="number"></param>
void ClientHandler(int number)
{
    AddText(fglv::hChat, L"Клиент подключен");
    AddText(fglv::hChat, L"\n");
    int client_number = number - 1;
    int size_of_message;
    bool connected = true;
    const wchar_t* wmessage;
    char* message;
    DWORD dwResult = 1;
    while (dwResult != WAIT_OBJECT_0)
    {
        dwResult = WaitForSingleObject(glv::hSemaphore, 1);
        Sleep(300 * MILLISECOND);
    }
    SendHistory(client_number);
    while (connected)
    {
        if (recv(glv::Sockets[client_number], (char*)&size_of_message, INT_SIZE, NULL) < 0)
        {
            ReleaseSemaphore(glv::hSemaphore, glv::current_users - 1, NULL);
            Mistake(L"Ошибка получения размера сообщения строка 268");
        }
        AddText(fglv::hChat, L"Размер сообщения: ");
        AddText(fglv::hChat, std::to_wstring(size_of_message).c_str());
        AddText(fglv::hChat, L"\n");

        message = new char[size_of_message + 2];
        message[size_of_message] = '\n';
        message[size_of_message + 1] = '\0';
        if (recv(glv::Sockets[client_number], message, size_of_message, NULL) < 0)
        {
            ReleaseSemaphore(glv::hSemaphore, glv::current_users - 1, NULL);
            Mistake(L"Ошибка получения сообщения строка 280");
        }

        std::string str(message);
        std::wstring wstr = StrToWstr(str);
        wmessage = wstr.c_str();

        AddText(fglv::hChat, L"Message: ");
        AddText(fglv::hChat, wmessage);
        AddText(fglv::hChat, L"\n");
        //Sleep(30 * SECOND);
        MessageHandler(wmessage, size_of_message, client_number, connected);
    }
    glv::closed_client = client_number;
    glv::server_loaded = false;
    AddText(fglv::hChat, L"\n");
    AddText(fglv::hChat, L"Клиент #");
    AddText(fglv::hChat, std::to_wstring(client_number).c_str());
    AddText(fglv::hChat, L" отключился\n");
    ReleaseSemaphore(glv::hSemaphore, 1, NULL);
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

/// <summary>
/// 
/// </summary>
/// <param name="hInstance"></param>
/// <param name="hPrevInstance"></param>
/// <param name="lpCmdLine"></param>
/// <param name="nCmdShow"></param>
/// <returns>WNDCLASSEX</returns>
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
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"Server";
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    return wcex;
}

/// <summary>
/// Инициализация параматра MSG
/// </summary>
/// <returns>MSG</returns>
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

/// <summary>
/// Функция инициализации версии библиотеки Winsock
/// </summary>
void InitializeWSA()
{
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0)
    {
        MessageBox(NULL, L"Ошибка инициализации библиотеки", L"Ошибка инициализации библиотеки", NULL);
        exit(1);
    }
}

/// <summary>
/// Функция инициализации семафора
/// </summary>
/// <returns>Semaphore</returns>
HANDLE InitializeSemaphore()
{
    HANDLE semaphore = CreateSemaphore(NULL, 2, 2, L"semaphore");
    if (!semaphore)
    {
        MessageBox(NULL, L"Ошибка инициализации Семафора", L"Ошибка инициализации Семафора", NULL);
        exit(1);
    }
    return semaphore;
}

/// <summary>
/// Функция добавления виджетов для окна
/// </summary>
/// <param name="hWnd"></param>
void MainWndAddWidgets(HWND hWnd)
{
    CreateWindow(L"static", L"Сервер", WS_VISIBLE | WS_CHILD | ES_CENTER, 5, 5, 720, 20, hWnd, NULL, NULL, NULL);
    fglv::hChat = CreateWindow(L"EDIT", NULL, WS_VISIBLE | WS_CHILD | ES_READONLY | WS_VSCROLL | ES_MULTILINE, 5, 35, 710, 220, hWnd, NULL, NULL, NULL);
}

void ProcessServer()
{
    InitializeWSA();
    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;
    AddText(fglv::hChat, L"Init library");
    AddText(fglv::hChat, L"\n");
    HANDLE hSemaphore = InitializeSemaphore();
    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);

    AddText(fglv::hChat, L"Listen");
    AddText(fglv::hChat, L"\n");

    int i = 0;
    glv::hSemaphore = CreateSemaphore(NULL, glv::max_users, glv::max_users, NULL);
    while (glv::process_flag)
    {
        if (!glv::process_flag)
            break;
        // записываем в вектор сокетов новые подключения
        glv::Sockets.push_back(accept(sListen, (SOCKADDR*)&addr, &sizeofaddr));
        ++glv::current_users;
        AddText(fglv::hChat, L"####\n");
        AddText(fglv::hChat, L"Socket.tail: ");
        AddText(fglv::hChat, std::to_wstring(glv::Sockets[glv::Sockets.size() - 1]).c_str());
        AddText(fglv::hChat, L"\n");
        AddText(fglv::hChat, L"####\n");

        if (i < 2)
        {
            // вносим в массив 
            glv::actual_clients[i] = i;
        }
        else
        {
            // перезаписываем подключения в зависимости от того кто подключился и кто отключился
            update_users(i);
            AddText(fglv::hChat, L"####\n");
            AddText(fglv::hChat, L"glv::actual_clients[0] ");
            AddText(fglv::hChat, std::to_wstring(glv::actual_clients[0]).c_str());
            AddText(fglv::hChat, L" | with socket: ");
            AddText(fglv::hChat, std::to_wstring(glv::Sockets[glv::actual_clients[0]]).c_str());
            AddText(fglv::hChat, L"\n");
            AddText(fglv::hChat, L"glv::actual_clients[1] ");
            AddText(fglv::hChat, std::to_wstring(glv::actual_clients[1]).c_str());
            AddText(fglv::hChat, L" | with socket: ");
            AddText(fglv::hChat, std::to_wstring(glv::Sockets[glv::actual_clients[0]]).c_str());
            AddText(fglv::hChat, L"\n");
            AddText(fglv::hChat, L"####");
            AddText(fglv::hChat, L"\n");
        }
        ++i;
        if (i > 1)
        {
            // если на сервере больше 2 человек, то ставим флаг приостановки обработки новых пользователей
            glv::server_loaded = true;
        }
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, (LPVOID)i, NULL, NULL);
        while (glv::server_loaded)
        {
            // пока стои флаг ожидаем
            Sleep(1 * SECOND);
        }
        Sleep(30 * MILLISECOND);
    }
    for (int i = 0; i < glv::Sockets.size(); ++i)
    {
        closesocket(glv::Sockets[i]);
    }
    // закрываем семафор
    CloseHandle(hSemaphore);
    WSACleanup();
    CleanHistoryMemory();
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

    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ProcessServer, NULL, NULL, NULL);
    MSG msg = HandleMSG();
    return (int)msg.wParam;
}

/// <summary>
/// Функция обработки событий оконного приложения
/// </summary>
/// <param name="hWnd"></param>
/// <param name="message"></param>
/// <param name="wParam"></param>
/// <param name="lParam"></param>
/// <returns>0</returns>
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