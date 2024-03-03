#include<string>
#include<iostream>

namespace constants
{
	const char* info_message = "Hello. This program counts min and max values in array";
	const char* info_message_int = "Enter size of array (positive integer): ";
	const char* info_message_arr = "Enter elements in array (integers): ";
	const char* incorrect_input = "Incorrect input! Retry: ";
	const char* extra_info = "You can't enter 0 because it is used as a flag in assembler input";
	const char* minimal_value = "Min value: ";
	const char* maximal_value = "Max value: ";
	const char* continue_info = "Enter 1 to continue working with program. Enter 2 to quit program.";
}

/// <summary>
/// костыльная функция получения int значения из консоли, к которой обращаетс ассемблер
/// </summary>
/// <param name="value"></param>
void scanint(int &value)
{
	std::string str;
	std::cin >> str;
	try 
	{
		value = std::stoi(str);
	}
	catch (...)
	{
		value = 0;
	}
	std::cin.ignore(-10000, '\n');
}

/// <summary>
/// костыльная функция вывода массива char, к которой обращается ассемблер
/// </summary>
/// <param name="message"></param>
void print_char(const char* message)
{
	std::cout << message << '\n';
}

/// <summary>
/// костыльная функция вывода массива int, к которой обращается ассемблер
/// </summary>
/// <param name="message"></param>
void print_int(int& message)
{
	std::cout << message << '\n';
}

/// <summary>
/// Функция показа вводной информации
/// </summary>
void show_info()
{
	__asm
	{
		mov eax, constants::info_message
		push eax
		call print_char
		add esp, 4 * 1

		mov eax, constants::extra_info
		push eax
		call print_char
		add esp, 4 * 1
	}
}

/// <summary>
/// функция получения размера массива
/// </summary>
/// <returns>size -> int</returns>
int get_int()
{
	int value;
	__asm
	{
		; выводим сообщение о вводе
		mov eax, constants::info_message_int
		push eax
		call print_char
		add esp, 4 * 1
		; делаем ввод intового значения
		mov eax, 1
		jmp Enter_value

		Enter_value:
			lea eax, value
			push eax
			call scanint
			add esp, 4 * 1
			mov ebx, value
			cmp value, 0
			jle incorrect_input
			jmp ex

		incorrect_input:
			mov ebx, constants::incorrect_input
			push ebx
			call print_char
			add esp, 4 * 1
			jmp Enter_value

		ex:
			xor eax, eax
			xor ebx, ebx
	}
	return value;
}

/// <summary>
/// Функция получения массива чисел, в котором будем искать минимум и максимум
/// </summary>
/// <param name="value"></param>
/// <returns>array -> int*</returns>
int* get_array(int value)
{
	int size_ = value;
	int ebx_res;
	int ecx_res;
	int* arr = new int[value];
	int val;
	__asm
	{
		; выводим сообщение о вводе
		mov eax, constants::info_message_arr
		push eax
		call print_char
		add esp, 4 * 1

		; вводим элементы массива
		mov ecx, size_
		mov esi, arr
		mov ebx, 0
		jmp For

		For:
			cmp ecx, 0
			jz Exit_
			jmp Enter_val
			
		Enter_val:
			lea eax, val
			mov ecx_res, ecx
			push eax
			call scanint
			add esp, 4 * 1
			cmp val, 0
			jz Incorrect_input
			mov edx, val
			push eax
			lea eax, [esi + 4 * ebx]
			mov [eax], edx
			pop eax
			mov ecx, ecx_res
			inc ebx
			dec ecx
			jmp For
			
		Incorrect_input:
			mov ebx_res, ebx
			mov ebx, constants::incorrect_input
			push ebx
			call print_char
			add esp, 4 * 1
			mov ebx, ebx_res
			mov ecx, ecx_res
			jmp Enter_val
			
		Exit_:
			xor eax, eax
			xor ebx, ebx
			xor esi, esi
			xor ecx, ecx
			xor edx, edx
	}
	return arr;
}

/// <summary>
/// Функция нахождения минимального и максимального значений функции
/// </summary>
/// <param name="Array_of_numbers"></param>
/// <param name="Length_of_array"></param>
/// <returns>result -> int* (min, max)</returns>
int* find_extremums(int* Array_of_numbers, int Length_of_array)
{
	int* res = new int[2];
	int min, max;
	__asm
	{
		mov ecx, 1
		mov esi, Array_of_numbers
		mov ebx, [esi]
		mov min, ebx
		mov max, ebx
		jmp For

		For:
			cmp ecx, Length_of_array
			je Enter_res
			mov eax, [esi + 4 * ecx]
			mov edx, min
			cmp eax, edx
			jl Less
			mov edx, max
			cmp eax, edx
			jg More
			inc ecx
			jmp For

		More:
			mov max, eax
			inc ecx
			jmp For

		Less:
			mov min, eax
			inc ecx
			jmp For

		Enter_res:
			mov esi, res
			lea eax, [esi]
			mov ebx, min
			mov [eax], ebx
			lea eax, [esi + 4]
			mov ebx, max
			mov [eax], ebx
			jmp Exit_asm

		Exit_asm:
			xor eax, eax
			xor bl, bl
			xor dl, dl
			xor esi, esi
			xor ecx, ecx
	}
	return res;
}

/// <summary>
/// Функция вывода в консоль результата
/// </summary>
/// <param name="*res"></param>
void show_result(int* res)
{
	__asm
	{
		mov eax, constants::minimal_value
		push eax
		call print_char
		add esp, 4 * 1

		lea esi, res
		mov eax, [esi]
		push eax
		call print_int
		add esp, 4 * 1

		mov eax, constants::maximal_value
		push eax
		call print_char
		add esp, 4 * 1

		add res, 4
		lea esi, res
		mov eax, [esi]
		push eax
		call print_int
		add esp, 4 * 1
	}
}

/// <summary>
/// Функция проверки зацикливания программы
/// </summary>
/// <returns>flag -> bool</returns>
bool check_continue()
{
	int res;
	__asm
	{
		mov eax, constants::continue_info
		push eax
		call print_char
		add esp, 4 * 1

		For:
			lea eax, res
			push eax
			call scanint
			add esp, 4 * 1
			mov ebx, res
			cmp ebx, 1
			jne Is_two
			jmp Exit_asm
		Is_two:
			cmp ebx, 2
			jne Incorrect_input
			jmp Exit_asm

		Incorrect_input:
			mov eax, constants::incorrect_input
			push eax
			call print_char
			add esp, 4 * 1
			jmp For

		Exit_asm:
			xor eax, eax
			xor ebx, ebx
	}
	return res - 2;
}

int main()
{
	bool continue_flag = true;
	while (continue_flag)
	{
		show_info();
		int size = get_int();
		int* arr = get_array(size);
		int* res = find_extremums(arr, size);
		show_result(res);
		continue_flag = check_continue();
	}
	return 0;
}