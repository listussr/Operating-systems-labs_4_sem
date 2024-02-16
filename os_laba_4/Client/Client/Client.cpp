#pragma comment(lib, "ws2_32.lib")
#include<WinSock2.h>
#include<iostream>
#include<Windows.h>
#include<math.h>
#pragma warning(disable:4996)

/*
Вариант 23-24. (24 вариант с помощью сокетов)
С процесса-сервера запускается n процессов клиентов. Для каждого из созданных клиентов указывается время жизни (в секундах).
Клиент запускается, существует заданное время и завершает работу. Также следует предусмотреть значение для бесконечного времени.
Требуется не менее трех одновременно запускаемых процессов-клиентов.
*/

void client_life(int lifetime) {
	// выводим время в консоль и ставим время жизни программы
	if (lifetime > 0) {
		std::cout << "Lifetime: " << lifetime << " seconds" << '\n';
		Sleep((DWORD)lifetime * 1000);
	}
	else {
		std::cout << "Lifetime: " << "eternity" << '\n';
		Sleep((DWORD)(pow(2, sizeof(DWORD) * 8) - 1) * 1000);
	}
	std::cout << "Time is up! You are going to be disconected in 10 seconds!" << '\n';
	Sleep(10 * 1000);
}

int get_lifetime(SOCKET Connection) {
	// получаем время жизни клиента от сервера
	int lifetime;
	recv(Connection, (char*)&lifetime, sizeof(int), NULL);
	return lifetime;
}

void initialise_WSA() {

	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0) {
		std::cout << "Error" << std::endl;
		exit(1);
	}
}

int main(int argc, char* argv[]) {
	initialise_WSA();
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
	if (connect(Connection, (SOCKADDR*)&addr, sizeof(addr))) {
		std::cout << "Failure in connection to server!" << '\n';
		return 1;
	}
	std::cout << "Connected!" << '\n';
	int lifetime = get_lifetime(Connection);
	client_life(lifetime);
	return 0;
}