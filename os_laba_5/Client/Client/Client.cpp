#pragma comment(lib, "ws2_32.lib")
#include<iostream>
#include<Winsock2.h>
#include<Windows.h>
#include<string>
#pragma warning(disable: 4996)

/*
* Вариант 24
Усовершенствованная версия чата. Разработать чат для обмена сообщениями.
Пусть на сервере есть чат, к которому могут одновременно присоединяться только 2 процесса-клиента. Остальные ждут своей очереди.
Чат общий для всех, у каждого клиента сообщения пишутся своим цветом.
Если клиент только подключился – ему отсылается вся текущая история и задается цвет фона консоли процесса-клиента на один из заранее предусмотренных.
*/

namespace glv 
{
	SOCKET Connection;
	bool connected_flag = true;
}

/// <summary>
/// функция инициализации использования библиотеки (специализируем последнюю версию)
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
/// аварийное завершение программы
/// </summary>
/// <param name="message"></param>
void mistake(const char* message) 
{
	std::cout << message << '\n';
	closesocket(glv::Connection);
	WSACleanup();
	exit(1);
}

// получаем цвет консоли от сервера
void set_color_theme()
{
	char* theme = new char[9];
	theme[8] = '\0';
	if (recv(glv::Connection, theme, 8, NULL) < 0)
	{
		mistake("Failure in getting color theme!");
	}
	//std::cout << theme << '\n';
	system(theme);
}

/// <summary>
/// получаем приветствие от сервера
/// </summary>
void get_hello_message() 
{
	char hello_message[41];
	if (recv(glv::Connection, hello_message, 41, NULL) < 0)
	{
		mistake("Failure in connection!");
	}
	//std::cout << sizeof(hello_message) << '\n';
	std::cout << hello_message << '\n';
}

/// <summary>
/// получаем историю сообщений от сервера
/// </summary>
void get_chat_history() 
{
	size_t history_size;
	int message_size, name_size;
	char* message, * name;
	// получаем размер истории - кол-во сообщений в ней
	if (recv(glv::Connection, (char*)&history_size, sizeof(size_t), NULL) < 0) 
	{
		mistake("Failure in getting history size!");
	}
	// получаем каждое сообщение
	for (size_t i = 0; i < history_size; ++i)
	{
		// получаем имя отправителя сообщения
		if (recv(glv::Connection, (char*)&name_size, sizeof(int), NULL) < 0)
		{
			mistake("Failure in receiving name size!");
		}
		name = new char[name_size + 1];
		name[name_size] = '\0';
		if (recv(glv::Connection, name, name_size, NULL) < 0)
		{
			mistake("Failure in getting name!");
		}
		// получаем размер сообщения
		if (recv(glv::Connection, (char*)&message_size, sizeof(int), NULL) < 0)
		{
			mistake("Failure in getting message size!");
		}
		message = new char[message_size + 1];
		message[message_size] = '\0';
		// получаем само сообщение
		if (recv(glv::Connection, message, message_size, NULL) < 0)
		{
			mistake("Failure in getting message!");
		}
		std::cout << name << ": ";
		std::cout << message << '\n';
		// чистим память
		delete[] message;
		delete[] name;
	}
}

/// <summary>
/// функция получения сообщения от собеседника
/// </summary>
void get_message() 
{
	// пока мы подключены к серверу получаем сообщения
	while (glv::connected_flag) 
	{
		int client_number;
		int message_size;
		char* message;
		// получаем номер клиента, чтобы знать от кого сообщение
		if (recv(glv::Connection, (char*)&client_number, sizeof(int), NULL) < 0)
		{
			mistake("Failure in getting client number!");
		}
		// получаем размер сообщения
		if (recv(glv::Connection, (char*)&message_size, sizeof(int), NULL) < 0)
		{
			mistake("Failure in getting message size!");
		}
		message = new char[message_size + 1];
		//std::cout << "Receiving message size: " << message_size << '\n';
		message[message_size] = '\0';
		// получаем само сообщение
		if (recv(glv::Connection, message, message_size, NULL) < 0)
		{
			mistake("Failure in getting message!");
		}
		// выводим сообщение в чат
		std::cout << "User #" << client_number << ": ";
		std::cout << message << '\n';
		// чистим память
		delete[] message;
	}
}

/// <summary>
/// функция отправки сообщения
/// </summary>
void send_message()
{
	std::string message;
	getline(std::cin, message);
	size_t message_size = message.size();
	// проверка на сообщение о выходе из чата
	if (message == "--leave") 
	{
		glv::connected_flag = false;
	}
	// отсылаем сначала размер сообщения, затем сообщение
	//std::cout << "Sending message size " << message.size() << '\n';
	send(glv::Connection, (char*)&message_size, sizeof(int), NULL);
	send(glv::Connection, message.c_str(), message_size, NULL);
}

/// <summary>
/// функция, контролирующая работу чату на стороне пользователя
/// </summary>
void handle_chat() 
{
	set_color_theme();
	get_hello_message();
	get_chat_history();
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)get_message, NULL, NULL, NULL);
	while (glv::connected_flag)
	{
		send_message();
	}
}

int main(int argc, char* argv[]) 
{
	initialise_WSA();
	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
	listen(sListen, SOMAXCONN);

	glv::Connection = socket(AF_INET, SOCK_STREAM, NULL);
	// проверяем на подключение к серверу
	if (connect(glv::Connection, (SOCKADDR*)&addr, sizeof(addr))) 
	{
		std::cout << "Failure in connection to server!" << '\n';
		return 1;
	}
	handle_chat();
	return 0;
}
