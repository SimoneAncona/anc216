#include <iostream>
#include <fstream>
#include <filesystem>
#include "console.hh"
#include "afs.hh"

#define CARD_READER_MAJOR_VERSION 1
#define CARD_READER_MINOR_VERSION 0
#define CARD_READER_PATCH_VERSION 0

namespace fs = std::filesystem;

void start_console_loop(ANC216::AFS &);
void mkdir(ANC216::AFS &, std::string  &);
void cd(ANC216::AFS &, std::string &);
void get(ANC216::AFS &, std::string &);
void set(ANC216::AFS &, std::string &);
void touch(ANC216::AFS &, std::string &);
void ls(ANC216::AFS &, std::string &);
void rm(ANC216::AFS &, std::string &);
void find(ANC216::AFS &, std::string &);
void du(ANC216::AFS &, std::string &);
void help();

std::string tabs(const std::string &);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage:\n"
                  << "\t" << argv[0] << " <source file>"
                  << std::endl
                  << RESET;
        return 0;
    }
    if (!fs::exists(argv[1]))
    {
        std::cout << RED << "error: " << RESET << "Cannot find " << argv[1] << std::endl;
        return -1;
    }
    fs::current_path(fs::absolute(argv[1]).parent_path());
    std::ifstream ifile(argv[1], std::ios::binary);
    ANC216::AFS afs(ifile);
    std::ofstream ofile(argv[1], std::ios::binary);
    if (afs.get_corrupted())
    {
        std::cout << RED << "error: " << RESET << "Corrupted card" << std::endl;
        return -1;
    }
    start_console_loop(afs);
    ofile.write(afs.get_buffer(), 65'536);
    ofile.close();
    return 0;
}

void start_console_loop(ANC216::AFS &fs)
{
    char c_input[256];
    std::string input;
    std::string command;
    while (true)
    {
        std::cout << GREEN << "$ " << RESET << fs.get_current_dir_absolute_path() << "> "; 
        std::cin.getline(c_input, sizeof(c_input));
        input = c_input;
        command = input.substr(0, input.find_first_of(" ") != std::string::npos ? input.find_first_of(" ") : input.length());
        for (auto &c : command)
            c = towlower(c);

        input += " ";
        if (command == "help")
        {
            help();
            continue;
        }
        if (command == "mkdir")
        {
            mkdir(fs, input);
            continue;
        }
        if (command == "cd")
        {
            cd(fs, input);
            continue;
        }
        if (command == "get")
        {
            get(fs, input);
            continue;
        }
        if (command == "touch")
        {
            touch(fs, input);
            continue;
        }
        if (command == "ls")
        {
            ls(fs, input);
            continue;
        }
        if (command == "exit")
        {
            return;
        }
        if (command == "rm")
        {
            rm(fs, input);
            continue;
        }
        if (command == "find")
        {
            find(fs, input);
            continue;
        }
        if (command == "du")
        {
            du(fs, input);
            continue;
        }
        if (command == "set")
        {
            set(fs, input);
            continue;
        }
        if (command == "get")
        {
            get(fs, input);
            continue;
        }
        std::cerr << RED << "error: " << RESET << "Unrecognized command" << std::endl;
    }
}

std::string tabs(const std::string &str)
{
    std::string res = "";
    for (int i = 0; i < 4 - str.size() / 8; i++)
    {
        res += "\t";
    }
    return res;
}

void help()
{
    std::cout <<
    CYAN << "cd <dir>" << RESET << "\t\t" << "change current directory" << "\n" <<
    CYAN << "du <file | dir>" << RESET << "\t\t" << "disk usage" << "\n" <<
    CYAN << "exit" << RESET << "\t\t\t" << "save end exit" << "\n" <<
    CYAN << "find <file>" << RESET << "\t\t" << "find a file" << "\n" <<
    CYAN << "get <file>" << RESET << "\t\t" << "print the file content" << "\n" <<
    CYAN << "help" << RESET << "\t\t\t" << "print this list" << "\n" <<
    CYAN << "ls" << RESET << "\t\t\t" << "show the content of the current directory" << "\n" <<
    CYAN << "mkdir <name>" << RESET << "\t\t" << "create a new directory" << "\n" <<
    CYAN << "rm <file>" << RESET << "\t\t" << "remove a file or a directory" << "\n" <<
    CYAN << "set <file> <real file>" << RESET << "\t" << "set the content of a file based on real file" << "\n" <<
    CYAN << "touch <name>" << RESET << "\t\t" << "create a new file" << "\n" <<
    "";
}

void mkdir(ANC216::AFS &fs, std::string &input)
{
    auto arg = input.substr(input.find_first_of(" ") + 1, input.length() - 7);
    if (arg.empty() || arg == " ")
    {
        std::cerr << RED << "error: " << RESET << "Expected one argument" << std::endl;
        return;
    }
    auto res = fs.make_dir(arg);
    switch (res)
    {
    case DIRNAME_FILENAME_ALREADY_EXIST:
        std::cerr << RED << "error: " << RESET << "File or directory already exist" << std::endl;
        return;
    case DIRNAME_FILENAME_TOO_LONG:
        std::cerr << RED << "error: " << RESET << "Directory name is too long" << std::endl;
        return;
    case NO_MORE_SPACE:
        std::cerr << RED << "error: " << RESET << "No more space" << std::endl;
        return;
    default:
        std::cout << "Directory created" << std::endl;
        return;
    }
}

void cd(ANC216::AFS &fs, std::string &input)
{
    auto arg = input.substr(input.find_first_of(" ") + 1, input.length() - 4);
    if (arg.empty() || arg == " ")
    {
        std::cerr << RED << "error: " << RESET << "Expected one argument" << std::endl;
        return;
    }
    if (!fs.change_directory(arg))
    {
        std::cerr << RED << "error: " << RESET << "Cannot find directory " << arg << std::endl;
    }
}

void du(ANC216::AFS &fs, std::string &input)
{
    auto arg = input.substr(input.find_first_of(" ") + 1, input.length() - 4);
    if (arg.empty() || arg == " ")
    {
        std::cerr << RED << "error: " << RESET << "Expected one argument" << std::endl;
        return;
    }
    auto res = fs.disk_usage(arg);
    if (res.empty())
    {
        std::cerr << RED << "error: " << RESET << "Cannot find file or directory " << arg << std::endl;
    }
    size_t real = 0;
    size_t file = 0;
    for (auto &s : res)
    {
        real += s.second.first;
        file += s.second.second;
        std::cout << s.first << "\t" << tabs(s.first) << "real: " << CYAN << s.second.first << RESET << "\tfile: " << CYAN << s.second.second << RESET << std::endl;
    }
    std::cout << "\nreal: " << CYAN << real << RESET << "\tfile: " << CYAN << file << RESET << std::endl; 
}

void rm(ANC216::AFS &fs, std::string &input)
{
    auto arg = input.substr(input.find_first_of(" ") + 1, input.length() - 4);
    if (arg.empty() || arg == " ")
    {
        std::cerr << RED << "error: " << RESET << "Expected one argument" << std::endl;
        return;
    }
    if (!fs.remove(arg))
    {
        std::cerr << RED << "error: " << RESET << "Cannot find the file or directory " << arg << std::endl;
    }
}

void find(ANC216::AFS &fs, std::string &input)
{
    auto arg = input.substr(input.find_first_of(" ") + 1, input.length() - 6);
    if (arg.empty() || arg == " ")
    {
        std::cerr << RED << "error: " << RESET << "Expected one argument" << std::endl;
        return;
    }
    auto res = fs.find_file(arg);
    if (res.size() == 0)
    {
        std::cout << "No file found" << std::endl;
        return;
    }
    for (auto e : res)
    {
        std::cout << "./" << e << "\n";
    }
}


void set(ANC216::AFS &fs, std::string &input)
{
    input = input.substr(0, input.find_last_of(" "));
    auto arg1 = input.substr(input.find_first_of(" ") + 1, input.find_last_of(" ") - input.find_first_of(" ") - 1);
    auto arg2 = input.substr(input.find_last_of(" ") + 1, input.length() - input.find_last_of(" "));
    if (arg1.empty() || arg1 == " " || arg2.empty() || arg2 == " ")
    {
        std::cerr << RED << "error: " << RESET << "Expected two arguments" << std::endl;
        return;
    }

    if (!fs::exists(arg2))
    {
        std::cerr << RED << "error: " << RESET << "File " << arg2 << " not found" << std::endl;
        return;
    }
    std::ifstream infile(arg2, std::ios::binary);
    auto res = fs.set_content(arg1, infile);
    switch (res)
    {
    case INVALID_DIRNAME_FILENAME:
        std::cerr << RED << "error: " << RESET << "Cannot find file " << arg2 << std::endl;
        return;
    case NO_MORE_SPACE:
        std::cerr << RED << "error: " << RESET << "No more space" << std::endl;
        return;
    default:
        std::cout << "File updated" << std::endl;
        return;
    }
}

void get(ANC216::AFS &fs, std::string &input)
{
    auto arg = input.substr(input.find_first_of(" ") + 1, input.length() - 5);
     if (arg.empty() || arg == " ")
    {
        std::cerr << RED << "error: " << RESET << "Expected one argument" << std::endl;
        return;
    }
    std::string content;
    auto res = fs.get_content(arg, content);
    switch (res)
    {
    case INVALID_DIRNAME_FILENAME:
        std::cerr << RED << "error: " << RESET << "Cannot find file " << arg << std::endl;
        return;
    default:
        std::cout << content;
    }
}

void touch(ANC216::AFS &fs, std::string &input)
{
    auto arg = input.substr(input.find_first_of(" ") + 1, input.length() - 7);
    if (arg.empty() || arg == " ")
    {
        std::cerr << RED << "error: " << RESET << "Expected one argument" << std::endl;
        return;
    }
    auto res = fs.touch(arg);
    switch (res)
    {
    case DIRNAME_FILENAME_ALREADY_EXIST:
        std::cerr << RED << "error: " << RESET << "File or directory already exist" << std::endl;
        return;
    case DIRNAME_FILENAME_TOO_LONG:
        std::cerr << RED << "error: " << RESET << "File name is too long" << std::endl;
        return;
    case NO_MORE_SPACE:
        std::cerr << RED << "error: " << RESET << "No more space" << std::endl;
        return;
    case FILENAME_EXTENSION_TOO_LONG:
        std::cerr << RED << "error: " << RESET << "File extension must be less than or equal to 3 characters" << std::endl;
        return;
    default:
        std::cout << "File created" << std::endl;
        return;
    }
}

void ls(ANC216::AFS &fs, std::string &input)
{
    auto subdirs = fs.get_sub_diectories();
    auto subfiles = fs.get_files();
    for (auto &dir : subdirs)
    {
        std::cout << CYAN << dir << RESET << std::endl;
    }
    for (auto &file : subfiles)
    {
        std::cout << file << RESET << std::endl;
    }
}