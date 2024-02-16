#include <iostream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <cmath>
#include<fstream>

using std::cout;
using std::cin;
using std::ifstream;
using std::ofstream;

/*          100   (sin(x) + cos(n))
    f(x) = sum(   ----------------- )
            n=0           x + n!
*/

typedef long double ld;

/*int factorial(int& n){
    int _factorial = 1;
    for(int i = 2; i <= n; ++i)
        _factorial *= i;
    return _factorial;
}*/

int fd_in[2];
int fd_out[2];
pid_t pid;

void show_help_message() {
    cout << "To count current function with data from console you should restart program without any keys" << '\n';
    cout << "If you want to compute function with file input and output restart programm with the keys --file [INPUT FILENAME] [OUTPUT FILENAME]" << '\n';
}

ld enter(const char* msg) {
    cout << msg << '\t';
    std::string str_input;
    ld input;
    bool error_flag = true;
    while (error_flag) {
        cin >> str_input;
        try {
            input = std::stold(str_input);
            error_flag = false;
        }
        catch (std::invalid_argument) {
            cout << "Invalid argument! Retry input" << '\n';
        }
    }
    return input;
}

ld function(ld& x) {
    ld sum = 0;
    int factorial = 1;
    for (int n = 0; n <= 100; ++n) {
        sum += (sin(x) + cos(n)) / (x + factorial);
        factorial *= (n > 0) ? n : 1;
    }
    return sum;
}

void server() {
    ld x;
    read(fd_in[0], &x, sizeof(ld));
    ld result = function(x);
    write(fd_out[1], &result, sizeof(ld));
}

void client() {
    ld x = enter("Enter x. X is the number with a floating point");
    write(fd_in[1], &x, sizeof(ld));
    ld result;
    read(fd_out[0], &result, sizeof(ld));
    cout << "f(x) = " << result << '\n';
}

void console_input() {
    cout << "Program for calculating function f(x) = sum(n = {0-100}) (sin(x) + cos(n)) / (x + n!)" << '\n';
    pipe(fd_in);
    pipe(fd_out);
    pid = fork();
    if (pid < 0) {
        std::cerr << "Failure in the forking!" << '\n';
        exit(1);
    }
    else if (pid > 0) {
        client();
    }
    else {
        server();
    }
    for (int i = 0; i < 2; ++i) {
        close(fd_in[i]);
        close(fd_out[i]);
    }
}

ld scan(const char* filename) {
    ifstream in;
    in.open(filename, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Incorrect input filename!" << '\n';
        exit(1);
    }
    cout << "File input" << '\n';
    std::string str_data;
    ld data;
    in >> str_data;
    in.close();
    try {
        data = std::stold(str_data);
    }
    catch (std::invalid_argument) {
        std::cerr << "Incorrect data in the input file!" << '\n';
        exit(1);
    }
    return data;
}

void load(const char* filename, ld& value) {
    ofstream out;
    out.open(filename, std::ios::binary);
    out << value;
    out.close();
}

void file_client(const char* in_filename, const char* out_filename) {
    ld x = scan(in_filename);
    write(fd_in[1], &x, sizeof(ld));
    ld result;
    read(fd_out[0], &result, sizeof(ld));
    cout << "Check the output file" << '\n';
    load(out_filename, result);
}

void file_input(const char* in_filename, const char* out_filename) {
    cout << "Program for calculating function f(x) = sum(n = {0-100}) (sin(x) + cos(n)) / (x + n!)" << '\n';
    pipe(fd_in);
    pipe(fd_out);
    pid = fork();
    if (pid < 0) {
        std::cerr << "Failure in the forking!" << '\n';
        exit(1);
    }
    else if (pid > 0) {
        file_client(in_filename, out_filename);
    }
    else {
        server();
    }
    for (int i = 0; i < 2; ++i) {
        close(fd_in[i]);
        close(fd_out[i]);
    }
}

int main(int argc, char* argv[]) {
    if (argc == 2 && !strcmp(argv[1], "--help")) {
        show_help_message();
        exit(0);
    }
    else if (argc == 4 && !strcmp(argv[1], "--file")) {
        file_input(argv[2], argv[3]);
    }
    else if (argc > 2 && argc != 4) {
        cout << "Restart program with the key [--help]" << '\n';
        exit(1);
    }
    else {
        console_input();
    }
    return 0;
}