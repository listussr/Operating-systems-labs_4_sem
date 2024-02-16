#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fcntl.h>
#include <fstream>
#include<cstring>
#include<string>
#include<iomanip>
#include<regex>

#ifdef __linux__ 
#define slash char{47}
#define slash_str "/"
#include <unistd.h>
#include <sys/stat.h>
#elif _WIN32
#define slash char{92}
#define slash_str "\\"
#endif

using std::cin;
using std::ifstream;
using std::ofstream;
using std::cout;
/*
Программа должна корректно обрабатывать ключи и аргумен-
ты в случае ввода из командной строки, либо программа должна

показывать список действий в случае ввода из консоли ввода.
Программа должна иметь возможность осуществлять:
- копирование файлов,
- перемещение файлов,
- получение информации о файле (права, размер, время изменения),
- изменение прав на выбранный файл.

Просто вызвать функцию для копирования файла нельзя.
Также программа должна иметь help для работы с ней, он должен
вызываться при запуске программы с ключом --help.
*/

void show_help_message() {
    cout << "Options:" << '\n';
    cout << "Enter --copy [FILENAME] [NEWFILENAME] to copy first file into second file" << '\n';
    cout << "Enter --move [FILENAME] [DIRNAME] to move file into the marked directory" << '\n';
    cout << "Enter --info [FILENAME] to get information about the marked file" << '\n';
    cout << "Enter --chmod [FILENAME] [MODE] to change mode of the marked file" << '\n';
    cout << "[MODE] = [*** where * equals a number 0-7] | [rwxrwxrwx]" << '\n';
}

void mistake(const char* message) {
    cout << "<" << message << ">" << '\n';
    cout << "[Enter --help to see possible commands]" << '\n';
}

int copy_file(const char* filename, const char* newfilename) {
    //проверка на копирование файла в себя
    if (!strcmp(filename, newfilename)) {
        mistake("File can't be copied inside itself!");
        return 2; // код ошибки 2 - названия файлов совпадают
    }
    // код ошибки
    int code{ 0 }; // 0 - всё в порядке
    ifstream in;
    ofstream out;
    in.open(filename, std::ios::binary);
    if (in.is_open()) {
        const int buffer_size = 4;
        char* buffer = new char[buffer_size];
        out.open(newfilename, std::ios::binary);
        // копируем один файл в другой
        while (!in.eof()) {
            in.read(buffer, buffer_size);
            if (in.gcount())
                out.write(buffer, in.gcount());
        }
        cout << "{File has been copied succesfully!}" << '\n';
        code = 0;
        out.close();
        delete[] buffer;
    }
    else {
        //отправляем сообщение об ошибке
        mistake("Source file doesn't exist!");
        code = 1; // код ошибки 1 - файла с названием <filename> не существует
    }
    in.close();
    return code;
}

int move_file(const char* filename, const char* dirname) {
    //проверка на существование файла
    ifstream in;
    in.open(filename, std::ios::binary);
    if (!in.is_open()) {
        mistake("Incorrect filename!");
        return 1; // флаг ошибки 1 - файла не существует
    }
    in.close();
    // выбираем только название файла без пути к нему
    bool slash_flag = 1;
    int ptr = strlen(filename) - 1;
    for (; ptr > 0 && slash_flag; --ptr) {
        if (filename[ptr] == slash)
            slash_flag = 0;
    }
    char* file = new char[strlen(filename) - ptr];
    strcpy(file, filename + ptr);
    // создаём массив под новое название
    char* newfilename = new char[strlen(dirname) + strlen(filename)];
    // конкатенации newfilename и строки
    strcpy(newfilename, dirname);
    strcpy(newfilename + strlen(dirname), slash_str);
    strcpy(newfilename + strlen(dirname) + 1, file);
    strcpy(newfilename + strlen(dirname) + 1 + strlen(file), "\0");
    cout << "{File has been moved succefully!}" << '\n';
    // переименовываем файл <=> переносим в новую директорию
    return rename(filename, newfilename);
}

const char* file_type(struct stat statbuffer) {
    char* type;
    if (S_ISREG(statbuffer.st_mode)) {
        strcpy(type, "regular");
    }
    else if (S_ISDIR(statbuffer.st_mode)) {
        strcpy(type, "directory");
    }
    else if (S_ISBLK(statbuffer.st_mode)) {
        strcpy(type, "block special");
    }
    else if (S_ISFIFO(statbuffer.st_mode)) {
        strcpy(type, "pipe or FIFO");
    }
    else if (S_ISLNK(statbuffer.st_mode)) {
        strcpy(type, "symbolic link");
    }
    else if (S_ISCHR(statbuffer.st_mode)) {
        strcpy(type, "character special");
    }
    else if (S_ISSOCK(statbuffer.st_mode)) {
        strcpy(type, "socket");
    }
    else {
        strcpy(type, "unknown type");
    }
    return type;
}

const char* file_mode(struct stat statbuffer) {
    char* mode = new char[9];
    mode[0] = (statbuffer.st_mode & S_IRUSR) ? 'r' : '-';
    mode[1] = (statbuffer.st_mode & S_IWUSR) ? 'w' : '-';
    mode[2] = (statbuffer.st_mode & S_IXUSR) ? 'x' : '-';
    mode[3] = (statbuffer.st_mode & S_IRGRP) ? 'r' : '-';
    mode[4] = (statbuffer.st_mode & S_IWGRP) ? 'w' : '-';
    mode[5] = (statbuffer.st_mode & S_IXGRP) ? 'x' : '-';
    mode[6] = (statbuffer.st_mode & S_IROTH) ? 'r' : '-';
    mode[7] = (statbuffer.st_mode & S_IWOTH) ? 'w' : '-';
    mode[8] = (statbuffer.st_mode & S_IXOTH) ? 'x' : '-';
    return mode;
}

int info_file(const char* filename) {
    // stat from library
    struct stat statbuffer;
    if (lstat(filename, &statbuffer)) {
        mistake("Incorrect filename");
        return 1;
    }
    //const char* type = file_type(statbuffer);
    const char* mode = file_mode(statbuffer);
    cout << "Name:" << std::setw(30) << std::right << filename << '\n';
    cout << "Size:" << std::setw(23) << std::right << statbuffer.st_size << " bytes" << '\n';
    cout << "User ID of owner:" << std::setw(14) << std::right << statbuffer.st_uid << '\n';
    cout << "Group ID of owner: " << std::setw(12) << std::right << statbuffer.st_gid << "\n";
    cout << "Devise ID: " << std::setw(17) << std::right << statbuffer.st_rdev << "\n";
    cout << "Last modification:" << std::setw(34) << std::right << ctime(&statbuffer.st_mtime);
    cout << "Last access:" << std::setw(40) << std::right << ctime(&statbuffer.st_atime);
    cout << "Last state change:" << std::setw(34) << std::right << ctime(&statbuffer.st_ctime);
    //cout << "Type:" << '\t' << type << '\n';
    cout << "Mode:" << std::setw(31) << std::right << mode << '\n';
    //delete[] type;
    delete[] mode;
    return 0;
}

int get_code(std::string mask) {
    int ans = 0;
    int x = 256;
    for (int i = 0; i < mask.length(); ++i) {
        if (mask[i] != '-') {
            ans += x;
        }
        x /= 2;
    }
    return ans;
}

void change_mode(char* name, char* mode) {
    ifstream in;
    in.open(name, std::ios::binary);
    /*if (!in.is_open()) {
            mistake("File with such name doesn't exist!");
    }*/
    //else {
    const std::regex reg3("([0-7])([0-7])([0-7])");
    const std::regex reg9("(r|-)(w|-)(x|-)(r|-)(w|-)(x|-)(r|-)(w|-)(x|-)");
    if (std::regex_match(mode, reg3)) {
        std::string samples[] = { "---","--x","-w-","-wx","r--","r-x" };
        /*int mask = std::stoi(mode);
                    std::string ans;
                    while (mask != 0) {
                            ans = samples[mask % 10] + ans;
                            mask /= 10;
                    }
                    int code = get_code(ans);
                    chmod(name, code);
                    cout << "Successfully!\n";
                    */
                    /*std::string answer;
                    for(int i = 0; i < 3; ++i){
                    answer += samples[static_cast<int>(mode[i]) - 48];
                    }*/
        int code = get_code(ans);
        chmod(name, code);
        cout << "{Mode has been changed succesfully!\n}";
    }
    else if (regex_match(mode, reg9)) {
        int code = get_code(mode);
        chmod(name, code);
        cout << "{Mode has been changed succesfully!\n}";
    }
    else {
        mistake("Incorrect format of the input!");
    }
    //}
    in.close();
}

int main(int argc, char* argw[]) {
    if (argc == 2 && !strcmp(argw[1], "--help")) {
        show_help_message();
    }
    else if (argc == 3 && !strcmp(argw[1], "--info")) {
        info_file(argw[2]);
    }
    else if (argc == 4) {
        if (!strcmp(argw[1], "--move"))
            move_file(argw[2], argw[3]);
        else if (!strcmp(argw[1], "--copy"))
            copy_file(argw[2], argw[3]);
        else if (!strcmp(argw[1], "--chmod"))
            change_mode(argw[2], argw[3]);
        else {
            mistake("Incorrect command!");
        }
    }
    else {
        mistake("Incorrect command!");
    }
    return 0;
}