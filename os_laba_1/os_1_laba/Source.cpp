#include<iostream>
#include<string>

using std::cin;
using std::cout;
typedef long double ld;
typedef std::pair<ld, ld> Pair;

void Programm() {
	cout << "Hello!" << '\n' << "This programm finds min and max values in array by the one pass.\n";
	cout << "--------------" << '\n';
}

ld Enter_ld(const char* str) {
	std::string str_val;
	ld val;
	bool correct{0};
	cout << str << ' ';
	while (!correct) {
		cin >> str_val;
		try {
			val = std::stold(str_val);
			correct = true;
		}
		catch (std::out_of_range) {
			cout << "Out of Long Double range!" << '\n';
		}
		catch (std::invalid_argument) {
			cout << "Invalid argument" << '\n';
		}
	}
	return val;
}

int Enter_Positive_Int(const char* str) {
	std::string str_val;
	int val;
	bool correct{ 0 };
	cout << str << ' ';
	while (!correct) {
		cin >> str_val;
		try {
			if (str_val.find(".") == std::string::npos) {
				val = std::stoi(str_val);
				if (val > 0)
					correct = true;
				else
					cout << "Size should be > 0. Try again" << '\n';
			}
			else
				throw  std::invalid_argument("");
		}
		catch (std::out_of_range) {
			cout << "Out of Integer range!" << '\n';
		}
		catch (std::invalid_argument) {
			cout << "Invalid argument!" << '\n';
		}
	}
	return val;
}

ld* GetArray(int size) {
	ld* arr = new ld[size];
	for (int i = 0; i < size; ++i) {
		arr[i] = Enter_ld("value");
	}
	return arr;
}

Pair GetBorderValues(ld* arr, int size) {
	Pair values;
	values.first = *arr;
	values.second = *arr;
	for (int i = 0; i < size; ++i) {
		if (arr[i] > values.second)
			values.second = arr[i];
		if (arr[i] < values.first)
			values.first = arr[i];
	}
	return values;
}

bool ContinueCheck() {
	std::string res;
	cout << "\n--------------" << '\n';
	cout << "If you want to close the program - press q" << '\n';
	cout << "If you want to continue - press c" << '\n';
	bool flag = true;
	bool res_flag = true;
	while (flag) {
		getline(cin, res);
		if (res.size() == 1 && res == "q") {
			flag = false;
			res_flag = false;
		}
		else if (res.size() == 1 && res == "c") {
			flag = false;
			res_flag = true;
		}
		else if(res.size() > 1) {
			cout << "Incorrect input!" << " Try again:" << '\t';
		}
	}
	cout << "\n--------------" << '\n';
	return res_flag;
}

int main() {
	Programm();
	bool flag = true;
	while (flag) {
		int size = Enter_Positive_Int("size of array");
		ld* arr = GetArray(size);
		Pair p = GetBorderValues(arr, size);
		cout << "Min value: " << p.first << '\n' << "Max value: " << p.second << '\n';
		flag = ContinueCheck();
	}
	return 0;
}