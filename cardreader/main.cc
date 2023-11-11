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
void cat(ANC216::AFS &, std::string &);
void touch(ANC216::AFS &, std::string &);
void ls(ANC216::AFS &, std::string &);
void help();

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
    std::fstream file;
    file.open(argv[1], std::ios::in | std::ios::binary | std::ios::out);
    ANC216::AFS afs(&file);
    if (afs.get_corrupted())
    {
        std::cout << RED << "error: " << RESET << "Corrupted card" << std::endl;
        return -1;
    }
    start_console_loop(afs);
    file.write(afs.get_buffer(), sizeof(char) * 65'536);
    file.close();
    return 0;
}

void start_console_loop(ANC216::AFS &fs)
{
    char c_input[256];
    std::string input;
    while (true)
    {
        std::cout << GREEN << "$ " << RESET << fs.get_current_dir_absolute_path() << "> "; 
        std::cin.getline(c_input, sizeof(c_input));
        input = c_input;
        for (size_t i = 0; i < input.size(); i++)
        {
            input[i] = std::tolower(input[i]);
        }
        input += " ";
        if (input == "help ")
        {
            help();
            continue;
        }
        if (input.starts_with("mkdir "))
        {
            mkdir(fs, input);
            continue;
        }
        if (input.starts_with("cd "))
        {
            cd(fs, input);
            continue;
        }
        if (input.starts_with("cat "))
        {
            cat(fs, input);
            continue;
        }
        if (input.starts_with("touch "))
        {
            touch(fs, input);
            continue;
        }
        if (input.starts_with("ls "))
        {
            ls(fs, input);
            continue;
        }
        if (input.starts_with("exit "))
        {
            return;
        }
        std::cerr << RED << "error: " << RESET << "Unrecognized command" << std::endl;
    }
}

void help()
{
    std::cout <<
    CYAN << "cat <file>" << RESET << "\t\t" << "print the file content" << "\n" <<
    CYAN << "cd [dir]" << RESET << "\t\t" << "change current directory" << "\n" <<
    CYAN << "exit" << RESET << "\t\t\t" << "save end exit" << "\n" <<
    CYAN << "help" << RESET << "\t\t\t" << "print this list" << "\n" <<
    CYAN << "ls" << RESET << "\t\t\t" << "show the content of the current directory" << "\n" <<
    CYAN << "mkdir <name>" << RESET << "\t\t" << "create a new directory" << "\n" <<
    CYAN << "save" << RESET << "\t\t\t" << "write the content in the card" << "\n" <<
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
        std::cerr << RED << "error: " << RESET << "Directory already exist" << std::endl;
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

void cat(ANC216::AFS &fs, std::string &input)
{}

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
        std::cerr << RED << "error: " << RESET << "File already exist" << std::endl;
        return;
    case DIRNAME_FILENAME_TOO_LONG:
        std::cerr << RED << "error: " << RESET << "File name is too long" << std::endl;
        return;
    case NO_MORE_SPACE:
        std::cerr << RED << "error: " << RESET << "No more space" << std::endl;
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