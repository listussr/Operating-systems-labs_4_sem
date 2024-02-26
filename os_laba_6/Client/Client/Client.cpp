#pragma comment(lib, "ws2_32.lib")
#include <Winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <string>
#pragma warning(disable: 4996)


#define ULL_SIZE sizeof(size_t)
#define INT_SIZE sizeof(int)
#define SECOND 1000
#define ON_BUTTON_CLICKED 1

static TCHAR szTitle[] = _T("���");

HINSTANCE hInst;

/// <summary>
/// ������� �������� ASCII ������ � ����������� ������
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
/// ������� �������� ����������� ������ � ASCII ������
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

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

/// <summary>
/// ���������� ���������� ������������ winapi
/// </summary>
namespace fglv
{
    static HWND hChat;  // ���������� ���� ����� ���������
    static HWND hEntry; // ���������� ���� ����������� ���������
    static HWND hBtn;   // ���������� ������ �������� ���������
}

/// <summary>
/// ���������� ���������� ��� ������ � �����
/// </summary>
namespace glv
{
    SOCKET Connection;
    // �� ��������� true
    bool connected_flag = true;
    // �� ��������� false
    bool written = false;
    wchar_t* wmessage;
}

/// <summary>
/// ������� ���������� ��������� �� �����
/// </summary>
/// <param name="eWnd">[���������� ����]</param>
/// <param name="text">[������������ �����]</param>
void AddText(HWND eWnd, const TCHAR* text)
{
    SendMessage(eWnd, EM_SETSEL, -1, -1);
    SendMessage(eWnd, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text));
}

/// <summary>
/// ������� ��������� ����������� � ���� ����� ������
/// </summary>
void ProcessEntry()
{
    int length = GetWindowTextLength(fglv::hEntry);
    glv::wmessage = new wchar_t[length + 1];
    GetWindowText(fglv::hEntry, glv::wmessage, length + 1);
    SendMessage(fglv::hEntry, WM_SETTEXT, WPARAM(0), WPARAM(L""));
    if (length > 0)
    {
        AddText(fglv::hChat, L"��: ");
        AddText(fglv::hChat, glv::wmessage);
        AddText(fglv::hChat, L"\n");
        glv::written = true;
    }
}

/// <summary>
/// ����������� �� ������� � ��������� ���������
/// </summary>
void Leave()
{
    closesocket(glv::Connection);
    WSACleanup();
    exit(0);
}

/// <summary>
/// ������� ��������� ��������������� �������� ���� ������� ����� �������
/// </summary>
void CloseClient()
{
    const char message[] = {"--leave"};
    int size_of_message = 7;
    // ������������� ���������� �� ������ ��������� � ������
    send(glv::Connection, (char*)&size_of_message, INT_SIZE, NULL);
    //AddText(fglv::hChat, L"������ ���������");
    //AddText(fglv::hChat, L"\n");
    Sleep(2 * SECOND); // �������
    send(glv::Connection, message, size_of_message, NULL);
    //AddText(fglv::hChat, L"��������� ����������");
    //AddText(fglv::hChat, L"\n");
    Sleep(2 * SECOND); // �������
    // �������
    Leave();
}

/// <summary>
/// ������� ��������� ������
/// </summary>
/// <param name="message"></param>
void Mistake(const wchar_t* message)
{
    MessageBox(NULL, L"������", message, NULL);
    closesocket(glv::Connection);
    WSACleanup();
    exit(1);
}

/// <summary>
/// ������� ��������� ������� ����
/// </summary>
void GetHistory()
{
    size_t history_size;
    int message_size, name_size;
    char* message, * name;
    if (recv(glv::Connection, (char*)&history_size, ULL_SIZE, NULL) < 0)
    {
        Mistake(L"��������� ������� �������");
    }
    for (size_t i = 0; i < history_size; ++i)
    {
        if (recv(glv::Connection, (char*)&name_size, INT_SIZE, NULL) < 0)
        {
            Mistake(L"��������� ������� ����� �� �������");
        }
        name = new char[name_size + 1];
        name[name_size] = '\0';
        if (recv(glv::Connection, name, name_size, NULL) < 0)
        {
            Mistake(L"��������� �����");
        }
        std::string namestr(name);
        std::wstring wnamestr = StrToWstr(namestr);
        const wchar_t* wname = wnamestr.c_str();
        // �������� ������ ���������
        if (recv(glv::Connection, (char*)&message_size, INT_SIZE, NULL) < 0)
        {
            Mistake(L"��������� ������� ���������");
        }
        message = new char[message_size + 2];
        message[message_size] = '\n';
        message[message_size + 1] = '\0';
        // �������� ���� ���������
        if (recv(glv::Connection, message, message_size, NULL) < 0)
        {
            Mistake(L"��������� ���������");
        }
        std::string str(message);
        std::wstring wstr = StrToWstr(str);
        const wchar_t* wmessage = wstr.c_str();
        AddText(fglv::hChat, wname);
        AddText(fglv::hChat, L" ");
        AddText(fglv::hChat, wmessage);
        // ������ ������
        delete[] message;
        delete[] name;
    }
}

/// <summary>
/// ������� �������� ��������� �� ������������ �� ������ ����� ����
/// </summary>
void GetChatMessage()
{
    while (glv::connected_flag)
    {
        int client_number;
        int message_size;
        const wchar_t* wmessage;
        char* message;
        if (recv(glv::Connection, (char*)&client_number, INT_SIZE, NULL) < 0)
        {
            Mistake(L"��������� ������ ������������");
        }

        AddText(fglv::hChat, L"����� ������������: ");
        AddText(fglv::hChat, std::to_wstring(client_number).c_str());
        AddText(fglv::hChat, L"\n");

        // �������� ������ ���������
        if (recv(glv::Connection, (char*)&message_size, INT_SIZE, NULL) < 0)
        {
            Mistake(L"��������� ������� ���������");
        }

        AddText(fglv::hChat, L"������ ���������: ");
        AddText(fglv::hChat, std::to_wstring(message_size).c_str());
        AddText(fglv::hChat, L"\n");

        message = new char[message_size + 2];
        message[message_size] = '\n';
        message[message_size + 1] = '\0';

        // �������� ���� ���������
        if (recv(glv::Connection, message, message_size, NULL) < 0)
        {
            Mistake(L"��������� ���������");
        }

        std::string str(message);
        std::wstring wstr = StrToWstr(str);
        wmessage = wstr.c_str();

        // ������� ��������� � ���
        AddText(fglv::hChat, L"������������ #");
        AddText(fglv::hChat, std::to_wstring(client_number).c_str());
        AddText(fglv::hChat, L": ");
        AddText(fglv::hChat, L"\n");
        AddText(fglv::hChat, L"Message: ");
        AddText(fglv::hChat, wstr.c_str());

        // ������ ������
        delete[] message;
    }
}

/// <summary>
/// ������� �������� ��������� ������� ������������
/// </summary>
void SendChatMessage()
{
    while (glv::connected_flag)
    {
        while (!glv::written)
        {
            Sleep(1 * SECOND);
        }
        if (!wcscmp(glv::wmessage, L"--leave"))
        {
            glv::connected_flag = false;
        }

        std::wstring wstr(glv::wmessage);
        std::string str = WstrToStr(wstr);
        const char* message = str.c_str();
        int size_of_message = str.size();

        AddText(fglv::hChat, L"������ ���������: ");
        AddText(fglv::hChat, std::to_wstring(size_of_message).c_str());
        AddText(fglv::hChat, L"\n");
        send(glv::Connection, (char*)&size_of_message, INT_SIZE, NULL);
        if (!glv::connected_flag)
        {
            AddText(fglv::hChat, L"������ ���������");
            AddText(fglv::hChat, L"\n");
            Sleep(2 * SECOND);
        }
        send(glv::Connection, message, size_of_message, NULL);
        if (!glv::connected_flag)
        {
            AddText(fglv::hChat, L"��������� ����������");
            AddText(fglv::hChat, L"\n");
            Sleep(2 * SECOND);
            MessageBox(NULL, L"��������� ��������� ����� 5 ������", L"�������� ���������", MB_OK);
        }
        glv::written = false;
    }
    Leave();
}

/// <summary>
/// ������� ���������� ������� ����
/// </summary>
void HandleChat()
{
    GetHistory();
    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)GetChatMessage, NULL, NULL, NULL);
    SendChatMessage();
}

/// <summary>
/// ������� �������� ��������
/// </summary>
/// <param name="hWnd"></param>
void CreateWndWidgets(HWND hWnd)
{
    CreateWindow(L"static", L"���", WS_VISIBLE | WS_CHILD | ES_CENTER, 5, 5, 710, 20, hWnd, NULL, NULL, NULL);
    fglv::hChat = CreateWindow(L"EDIT", NULL, WS_VISIBLE | WS_CHILD | ES_READONLY | WS_VSCROLL | ES_MULTILINE, 5, 35, 710, 220, hWnd, NULL, NULL, NULL);
    fglv::hEntry = CreateWindow(L"EDIT", L"������", WS_VISIBLE | WS_CHILD | WS_BORDER, 5, 270, 600, 20, hWnd, NULL, NULL, NULL);
    fglv::hBtn = CreateWindow(L"button", L"���������", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 610, 270, 90, 20, hWnd, (HMENU)ON_BUTTON_CLICKED, NULL, NULL);
}

/// <summary>
/// 
/// </summary>
/// <param name="hInstance"></param>
/// <param name="hPrevInstance"></param>
/// <param name="lpCmdLine"></param>
/// <param name="nCmdShow"></param>
/// <returns>WNDCLASSEX wcex</returns>
WNDCLASSEX InitialiseWNDCLASSEX(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
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
    wcex.lpszClassName = L"Client";
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    return wcex;
}

/// <summary>
/// ������������� ��������� MSG
/// </summary>
/// <returns></returns>
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
/// ������� ������������� ������ ���������� Winsock
/// </summary>
void InitialiseWSA()
{
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0)
    {
        MessageBox(NULL, L"������ ������������� ����������", L"������ ������������� ����������", NULL);
        exit(1);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wcex = InitialiseWNDCLASSEX(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL, _T("Call to RegisterClassEx failed!"), _T("Windows Desktop Guided Tour"), NULL);
        return 1;
    }
    hInst = hInstance;
    HWND hWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, L"Client", szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 740, 350, NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    InitialiseWSA();
    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;
    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);

    glv::Connection = socket(AF_INET, SOCK_STREAM, NULL);
    if (connect(glv::Connection, (SOCKADDR*)&addr, sizeof(addr)))
    {
        Mistake(L"������ ����������� � �������!");
    }
    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)HandleChat, NULL, NULL, NULL);
    MSG msg = HandleMSG();
    return (int)msg.wParam;
}

/// <summary>
/// ������� ��������� ������� �������� ����������
/// </summary>
/// <param name="hWnd"></param>
/// <param name="message"></param>
/// <param name="wParam"></param>
/// <param name="lParam"></param>
/// <returns>0</returns>
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
        {
            CreateWndWidgets(hWnd);
            break;
        }
        case WM_COMMAND:
        {
            switch (wParam)
            {
                case ON_BUTTON_CLICKED:
                {
                    ProcessEntry();
                    break;
                }
                default: break;
            }
            break;
        }
        case WM_DESTROY:
        {
            glv::connected_flag = false;
            CloseClient();  
            break;
        }
        default:
        {
            return DefWindowProc(hWnd, message, wParam, lParam);
            break;
        }
    }

    return 0;
}