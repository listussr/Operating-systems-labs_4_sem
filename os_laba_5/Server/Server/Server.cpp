#include <iostream>
#include <string>
#include <vector>
#include<winsock2.h>
#include<cassert>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996) // для inet_addr
using namespace std;

/*
Усовершенствованная версия чата. Разработать чат для обмена сообщениями.
Пусть на сервере есть чат, к которому могут одновременно присоединяться только 2 процесса-клиента. Остальные ждут своей очереди.
Чат общий для всех, у каждого клиента сообщения пишутся своим цветом.
Если клиент только подключился – ему отсылается вся текущая история и задается цвет фона консоли процесса-клиента на один из заранее предусмотренных.
*/

typedef pair<std::string, const char*> msg_info;

namespace glv {
	HANDLE hSemaphore;
	vector<SOCKET> Sockets;
	const DWORD max_users = 2;
	vector<msg_info> History;
	const char hello_message[] = { "Hi, you have been connected to the chat!" };
	int actual_clients[2];
	bool process_flag = true;
	const char* themes[] = { "color 1F", "color 2F", "color 3F", "color 4F", "color 5F", "color 6F", "color 7F", "color 8F", "color 9F" };
	bool server_loaded = false;
	int closed_client;
}

void mistake(const char* message)
{
	std::cout << message << '\n';
	for (int i = 0; i < glv::Sockets.size(); ++i)
	{
		closesocket(glv::Sockets[i]);
	}
	WSACleanup();
	exit(1);
}

void randomise()
{
	srand(time(NULL));
}

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

int get_random_integer_in_border(int min, int max, int except_int)
{
	int res;
	res = min + rand() % (max - min);
	std::cout << "Except int " << except_int << '\n';
	std::cout << res << '\n';
	if (res == except_int && res < max)
		return res + 1;
	else if(res == except_int)
		return res - 1;
	return res;
}

void send_color_theme(int client_number) 
{
	const char* theme = glv::themes[client_number % 8];
	std::cout << "Color for user #" << client_number << ": " << *theme << '\n';
	SOCKET current_connection = glv::Sockets[glv::actual_clients[(glv::actual_clients[0] != client_number) ? 1 : 0]];
	std::cout << "User who will recieve theme #" << client_number % 9 << " | with connection " << current_connection << '\n';
	send(current_connection, theme, 8, NULL);
}

// функция отправки сообщения другому пользователю
void resend_message(const char* message, int size_of_message, int client_number)
{
	// смотрим какому пользователю отправить сообщение
	SOCKET current_connection = glv::Sockets[glv::actual_clients[(glv::actual_clients[0] != client_number)? 0: 1]];
	std::cout << "-------------------" << '\n';
	std::cout << "current client to be received a message #" << client_number << " | with connection " << current_connection << '\n';
	for (int i = 0; i < 2; ++i)
	{
		std::cout << "Users[" << i << "]: " << glv::actual_clients[i] << '\n';
		std::cout << "Connections[" << i << "]: " << glv::Sockets[glv::actual_clients[i]] << '\n';
	}
	// отправляем размер сообщения и само сообщение
	send(current_connection, (char*)&client_number, sizeof(int), NULL);
	//std::cout << "Client number" << "<" << client_number << ">" << '\n';
	send(current_connection, (char*)&size_of_message, sizeof(int), NULL);
	//std::cout << "Size of message" << "<" << size_of_message << ">" << '\n';
	send(current_connection, message, size_of_message, NULL);
	//std::cout << "<Message>: " << message << '\n';
	std::cout << "-------------------" << '\n';
}

// функция обработки сообщения
void message_handler(const char* message, int message_length, int client_number, bool& connection_flag)
{
	// проверяем на соответствие команде о выходе
	if (!strcmp(message, "--leave"))
	{
		connection_flag = false;
	}
	//std::cout << "<Message>: " << message << '\n';
	// записываем сообщение в историю
	std::string name = "User #";
	name += std::to_string(client_number);
	std::cout << name << '\t' << message << '\n';
	msg_info msg(name, message);
	glv::History.push_back(msg);
	// отправляем сообщение другому пользователю
	resend_message(message, message_length, client_number);
}

// функция отправки истории чата новоподключившемуся пользователю
void send_history(int client_number)
{
	// выбираем какому из 2 активных пользователей отправить сообщение
	SOCKET current_connection = glv::Sockets[glv::actual_clients[(glv::actual_clients[0] != client_number) ?  1: 0]];
	// отправляем кол-во сообщений из истории
	size_t history_size = glv::History.size();
	send(current_connection, (char*)&history_size, sizeof(size_t), NULL);
	for (msg_info msg : glv::History)
	{
		// отправляем все сообщения из истории
		int size_of_message = sizeof(msg.second);
		int size_of_name = msg.first.size();
		send(current_connection, (char*)&size_of_name, sizeof(int), NULL);
		send(current_connection, msg.first.c_str(), size_of_name, NULL);
		send(current_connection, (char*)&size_of_message, sizeof(int), NULL);
		send(current_connection, msg.second, sizeof(msg.second), NULL);
	}
}

void client_HANDLER(int i)
{
	int client_number = i - 1;
	std::cout << "Client " << client_number << " connected" << '\n';
	int size_of_msg;
	char* message;
	DWORD dwResult = 1;
	bool connection_flag = true;
	// ожидаем подключения
	while (dwResult != WAIT_OBJECT_0)
	{
		dwResult = WaitForSingleObject(glv::hSemaphore, 1);
		Sleep(300);
	}
	send_color_theme(client_number);
	// отсылаем приветствие
	send(glv::Sockets[client_number], glv::hello_message, sizeof(glv::hello_message), NULL);
	// отсылаем историю чата
	send_history(client_number);
	// пока клиент не вышел
	while (connection_flag) {
		// получаем размер сообщения
		if (recv(glv::Sockets[client_number], (char*)&size_of_msg, sizeof(int), NULL) < 0)
		{
			ReleaseSemaphore(glv::hSemaphore, 1, NULL);
			mistake("Failure in recieving message");
		}
		message = new char[size_of_msg + 1];
		message[size_of_msg] = '\0';
		// получаем сообщение
		if (recv(glv::Sockets[client_number], message, size_of_msg, NULL) < 0) 
		{
			ReleaseSemaphore(glv::hSemaphore, 1, NULL);
			mistake("Failure in recieving message!");
		}
		// обрабатываем сообщение
		message_handler(message, size_of_msg, client_number, connection_flag);
	}
	glv::server_loaded = false;
	glv::closed_client = client_number;
	ReleaseSemaphore(glv::hSemaphore, 1, NULL);
	std::cout << "User " << client_number << " has disconnected" << '\n';

}

// функция инииализации библиотеки
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

HANDLE initialise_semaphore()
{
	HANDLE semaphore =	CreateSemaphore(NULL, 2, 2, L"semaphore");
	if (!semaphore) 
	{
		std::cout << "Failure in semaphore creation!" << '\n';
		exit(1);
	}
	return semaphore;
}

/*
bool is_in_border(int left_border, int right_border, int value) {
	return left_border <= value <= right_border;
}

int get_integer() {
	int left_val{ 3 }, right_val{ 10 };
	std::string str_val;
	int value;
	bool incorrect = true;
	std::cout << "Enter amount of clients in the chat (3 <= value <= 10): ";
	while (incorrect) {
		std::cin >> str_val;
		try {
			if(is_in_border(left_val, right_val, value = std::stoi(str_val)))
				incorrect = false;
			else {
				throw std::invalid_argument("");
			}
		}
		catch (std::invalid_argument) {
			std::cout << "Incorrect input or value out of the border! Retry input: ";
		}
	}
	std::cout << '\n';
	return value;
}

void create_client_processes(int amount_of_processes) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, si.cb);
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	for (int i = 0; i < amount_of_processes; ++i) {
		if (!CreateProcess(
			L"C:\\University\\kurs_2\\semestr_2\\systems\\os_laba_5\\Client\\x64\Debug\\Client.exe",
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
	}
}
*/

int main(int argc, char* argv[])
{
	randomise();
	initialise_WSA();
	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	HANDLE hSemaphore = initialise_semaphore();
	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
	listen(sListen, SOMAXCONN);

	int i = 0;
	glv::hSemaphore = CreateSemaphore(NULL, glv::max_users, glv::max_users, NULL);
	while (glv::process_flag)
	{
		if (!glv::process_flag)
			break;
		glv::Sockets.push_back(accept(sListen, (SOCKADDR*)&addr, &sizeofaddr));
		std::cout << "####" << '\n';
		std::cout << "Socket.tail: " << glv::Sockets[glv::Sockets.size() - 1] << '\n';
		std::cout << "####" << '\n';
		
		if (i < 2)
		{
			glv::actual_clients[i] = i;
		}
		else
		{
			update_users(i);
			std::cout << "####" << '\n';
			std::cout << "glv::actual_clients[0]: " << glv::actual_clients[0] << " | with socket: " << glv::Sockets[glv::actual_clients[0]] << '\n';
			std::cout << "glv::actual_clients[1]: " << glv::actual_clients[1] << " | with socket: " << glv::Sockets[glv::actual_clients[1]] << '\n';
			std::cout << "####" << '\n';
		}
		++i;
		if (i > 1)
		{
			glv::server_loaded = true;
		}
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)client_HANDLER, (LPVOID)i, NULL, NULL);
		while (glv::server_loaded)
		{
			Sleep(1 * 1000);
		}
		Sleep(30);
	}
	std::cout << "Server has been closed" << '\n';
	for (int i = 0; i < glv::Sockets.size(); ++i)
	{
		closesocket(glv::Sockets[i]);
	}
	CloseHandle(hSemaphore);
	WSACleanup();
	system("pause");
	return 0;
}