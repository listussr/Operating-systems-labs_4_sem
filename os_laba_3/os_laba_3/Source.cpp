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

/// @brief namespace with the global variables
namespace lab
{
    int fd_in [2];
    int fd_out[2];
    pid_t pid;
    bool file_flag = false;
    const char* input_filename;
    const char* output_filename;
}

/// @brief function that shows an informational message
void show_help_message()
{
    cout << "To count current function with data from console you should restart program without any keys" << '\n';
    cout << "If you want to compute function with file input and output restart programm with the keys --file [INPUT FILENAME] [OUTPUT FILENAME]" << '\n';
}


/// @brief function that takes the value from the file
/// @param filename 
/// @return value -> long double
ld scan(const char* filename)
{
    ifstream in;
    in.open(filename, std::ios::binary);
    if(!in.is_open())
    {
        std::cerr << "Incorrect input filename!" << '\n';
        exit(1);
    }
    cout << "File input" << '\n';
    std::string str_data;
    ld data;
    in >> str_data;
    in.close();
    try
    {
        data = std::stold(str_data);
    }
    catch(std::invalid_argument)
    {
        std::cerr << "Incorrect data in the input file!" << '\n';
        exit(1);
    }
    return data;
}

/// @brief function of loading value to the output file
/// @param filename 
/// @param value 
void load(const char* filename, ld& value)
{
    ofstream out;
    out.open(filename, std::ios::binary);
    out << value;
    out.close();
}

/// @brief function of getting a console input
/// @param msg 
/// @return value -> long double
ld enter(const char* msg)
{
    cout << msg << '\t';
    std::string str_input;
    ld input;
    bool error_flag = true;
    // waiting until we do not get a correct value
    while(error_flag)
    {
        cin >> str_input;
        try
        {
            input = std::stold(str_input);
            error_flag = false;
        }
        catch(std::invalid_argument)
        {
            cout << "Invalid argument! Retry input" << '\n';
        }
    }
    return input;
}

/// @brief function that counts Sum from the task
/// @param x 
/// @return result -> long double
ld function(ld& x)
{
    ld sum = 0;
    int factorial = 1;
    for(int n = 0; n <= 100; ++n)
    {
        sum += (sin(x) + cos(n)) / (x + factorial);
        factorial *= (n > 0)? n: 1;
    }
    return sum;
}

/// @brief function with the "backend" side of the program
void server()
{
    ld x;
    read(lab::fd_in[0], &x, sizeof(ld));
    ld result = function(x);
    write(lab::fd_out[1], &result, sizeof(ld));
}

/// @brief function with the "frontend" side of the program
void client()
{
    ld x;
    if(lab::file_flag)
    {
        x = scan(lab::input_filename);
    } 
    else
    {
        x = enter("Enter x. X is the number with a floating point");
    }
    write(lab::fd_in[1], &x, sizeof(ld));
    ld result;
    read(lab::fd_out[0], &result, sizeof(ld));
    if(lab::file_flag)
    {
        cout << "Check the output file" << '\n';
        load(lab::output_filename, result);
    }
    else
    {
        cout << "f(x) = " << result << '\n';
    }
}

/// @brief main processing function of the program
void process()
{
    cout << "Program for calculating function f(x) = sum(n = {0-100}) (sin(x) + cos(n)) / (x + n!)" << '\n';
    pipe(lab::fd_in);
    pipe(lab::fd_out);
    lab::pid = fork();
    if(lab::pid < 0)
    {
        std::cerr << "Failure in the forking!" << '\n';
        exit(1);
    }
    else if(lab::pid > 0)
    {
        client();
    }
    else
    {
        server();
    }
    for(int i = 0; i < 2; ++i)
    {
        close(lab::fd_in[i]);
        close(lab::fd_out[i]);
    }
}

int main(int argc, char* argv[])
{
    if(argc == 2 && !strcmp(argv[1], "--help"))
    {
        show_help_message();
        return 0;
    }
    else if(argc == 4 && !strcmp(argv[1], "--file"))
    {
        lab::file_flag = true;
        lab::input_filename = argv[2];
        lab::output_filename = argv[3];
    }
    else if(argc > 2 && argc != 4)
    {
        cout << "Restart program with the key [--help]" << '\n';
        return 1;
    }
    process();
    return 0;
}
