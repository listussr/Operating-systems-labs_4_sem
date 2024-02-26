#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <string>
#pragma warning(disable: 4996)

/*
Вариант 23-24. (24 вариант с помощью сокетов)
С процесса-сервера запускается n процессов клиентов. Для каждого из созданных клиентов указывается время жизни (в секундах).
Клиент запускается, существует заданное время и завершает работу. Также следует предусмотреть значение для бесконечного времени.
Требуется не менее трех одновременно запускаемых процессов-клиентов.
*/

SOCKET Connections[100];
int counter = 0;

/// <summary>
/// генерируем флаг бесконечного времени с вероятностью 1/4
/// </summary>
/// <returns>is_infinity -> bool</returns>
bool is_infinity()
{
	int min = 0;
	int max = 3;
	int result = min + rand() % (max - min + 1);
	return !result;
}

/// <summary>
/// генерируем время жизни пользователя на сервере
/// </summary>
/// <returns>time -> int</returns>
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

/// <summary>
/// добавляем рандом
/// </summary>
void randomise() 
{
	srand(time(NULL));
}

/// <summary>
/// инициализируем версию библиотеки
/// </summary>
void initialise_WSA() 
{
	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0)
	{
		std::cout << "Error" << std::endl;
		exit(1);
	}
}

/// <summary>
/// функция запуска процесса клиента
/// </summary>
void start_client_session() 
{
	const char* navigation_command = "cd C:\\University\\kurs_2\\semestr_2\\systems\\os_laba_4\\Client\\x64\\Debug";
	const char* run_command = "start C:\\University\\kurs_2\\semestr_2\\systems\\os_laba_4\\Client\\x64\\Debug\\Client.exe";
	// проверка на случай непредвиденных обстоятельств
	int navigation_failure = system(navigation_command);
	if (navigation_failure)
	{
		std::cout << "Failure in system navigation!" << '\n';
		exit(1);
	}
	// запускаем собранную программу у клиента
	system(run_command);
	
	/*STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, si.cb);
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	for (int i = 0; i < amount_of_processes; ++i) {
		if (!CreateProcess(
			L"C:\\University\\kurs_2\\semestr_2\\systems\\os_laba_4\\Client\\x64\\Debug\\Client.exe",
			NULL,
			NULL,
			NULL,
			FALSE,
			CREATE_NEW_CONSOLE,
			NULL,
			NULL,
			&si,
			&pi)
			) {
			std::cout << "Failure in creating process!" << '\n';
			exit(1);
		}
		Sleep(1 * 1000);
	}*/
}

/// <summary>
/// запуск процессов клиентов и генерирование для них врменени жизни на сервере
/// </summary>
/// <param name="sListen"></param>
/// <param name="addr"></param>
/// <param name="sizeofaddr"></param>
/// <param name="amount_of_clients"></param>
void connection(SOCKET sListen, SOCKADDR_IN addr, int sizeofaddr, int amount_of_clients) 
{
	SOCKET newConnection;
	for (int i = 0; i < amount_of_clients; ++i) 
	{
		start_client_session();
		newConnection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);
		if (!newConnection) 
		{
			std::cout << "Error #2\n";
		}
		else 
		{
			std::cout << "Client Connected!\n";
			int lifetime = generate_lifetime();
			send(newConnection, (char*)&lifetime, sizeof(int), NULL);
		}
		Connections[i] = newConnection;
		++counter;
	}
}

bool is_in_border(int left, int right, int value) 
{
	return left <= value && value < right;
}

/// <summary>
/// получаем количество клиентов сервера из консоли
/// </summary>
/// <returns></returns>
int get_int() 
{
	std::cout << "Enter amount of clients that will be opened (value between 3 and 100): ";
	std::string str;
	bool flag = true;
	int value, left{3}, right{100};
	// пока входные данные не являются коректными повторяем ввод
	while (flag)
	{
		std::cin >> str;
		try 
		{
			value = std::stoi(str);
			if (!is_in_border(3, 100, value))
			{
				throw std::invalid_argument("");
			}
			flag = false;
		}
		catch (std::invalid_argument) 
		{
			std::cout << "Incorrect argument or number is outside the borders! Retry input: ";
		}
	}
	return value;
}

int main(int argc, char* argv[]) 
{
	int amount_of_clients = get_int();
	randomise();
	initialise_WSA();
	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
	listen(sListen, SOMAXCONN);

	connection(sListen, addr, sizeofaddr, amount_of_clients);
	system("pause");
	return 0;
}
