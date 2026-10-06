#include <afs.hh>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <chrono>
namespace fs = std::filesystem;

static void help()
{
    std::cout << "Usage: cardreader <image> [command [arguments...]]\n"
                 "       cardreader --format <new image> [--boot <file>]\n"
                 "Commands (paths may be quoted in the interactive shell):\n"
                 "  mkdir <path>              Create directory\n"
                 "  cd <path>                 Change directory\n"
                 "  ls [path]                 List directory\n"
                 "  touch <path>              Create empty file\n"
                 "  set <path> <host file>    Replace existing file contents\n"
                 "  put <path> <host file>    Import or replace file\n"
                 "  get <path> [host file]    Print or export file\n"
                 "  rm <path>                 Remove file or directory recursively\n"
                 "  find <name>               Find files from the root\n"
                 "  du [path]                 File bytes and allocated bytes\n"
                 "  boot <host file>          Replace boot code (up to 255 bytes)\n"
                 "  exit                      Save and quit (EOF also saves)\n";
}
static void result(int code)
{
    if (!code)
        return;
    switch (code)
    {
    case INVALID_DIRNAME_FILENAME:
        throw std::runtime_error("Invalid name, missing file, or missing parent directory");
    case DIRNAME_FILENAME_TOO_LONG:
        throw std::runtime_error("Name exceeds AFS field size");
    case DIRNAME_FILENAME_ALREADY_EXIST:
        throw std::runtime_error("File or directory already exists");
    case FILENAME_EXTENSION_TOO_LONG:
        throw std::runtime_error("Extension exceeds three bytes");
    case NO_MORE_SPACE:
        throw std::runtime_error("Not enough space");
    default:
        throw std::runtime_error("AFS operation failed");
    }
}
static std::vector<uint8_t> read_boot(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot read boot file");
    std::vector<uint8_t> bytes;
    int ch;
    while ((ch = in.get()) != EOF)
    {
        bytes.push_back(ch);
        if (bytes.size() > BOOTX_SIZE)
            throw std::runtime_error("AFS boot code exceeds 255 bytes");
    }
    return bytes;
}
static bool command(ANC216::AFS &afs, const std::vector<std::string> &args, bool &dirty)
{
    if (args.empty())
        return true;
    const auto &op = args[0];
    auto arity = [&](unsigned min, unsigned max)
    {
        if (args.size() < min + 1 || args.size() > max + 1)
            throw std::runtime_error("Wrong number of arguments for " + op);
    };
    if (op == "exit")
    {
        arity(0, 0);
        return false;
    }
    if (op == "help")
    {
        arity(0, 0);
        help();
    }
    else if (op == "mkdir")
    {
        arity(1, 1);
        result(afs.make_dir(args[1]));
        dirty = true;
    }
    else if (op == "touch")
    {
        arity(1, 1);
        result(afs.touch(args[1]));
        dirty = true;
    }
    else if (op == "cd")
    {
        arity(1, 1);
        if (!afs.change_directory(args[1]))
            throw std::runtime_error("Directory not found");
    }
    else if (op == "ls")
    {
        arity(0, 1);
        auto copy = afs;
        if (args.size() == 2 && !copy.change_directory(args[1]))
            throw std::runtime_error("Directory not found");
        for (auto &name : copy.get_sub_diectories())
            std::cout << name << "/\n";
        for (auto &name : copy.get_files())
            std::cout << name << '\n';
    }
    else if (op == "set" || op == "put")
    {
        arity(2, 2);
        std::ifstream in(args[2], std::ios::binary);
        if (!in)
            throw std::runtime_error("Cannot read host file: " + args[2]);
        auto copy = afs;
        if (op == "put")
        {
            int code = copy.touch(args[1]);
            if (code != DIRNAME_FILENAME_ALREADY_EXIST)
                result(code);
        }
        result(copy.set_content(args[1], in));
        afs = copy;
        dirty = true;
    }
    else if (op == "get")
    {
        arity(1, 2);
        std::string content;
        result(afs.get_content(args[1], content));
        if (args.size() == 2)
            std::cout.write(content.data(), content.size());
        else
        {
            std::ofstream out(args[2], std::ios::binary | std::ios::trunc);
            if (!out || !out.write(content.data(), content.size()))
                throw std::runtime_error("Cannot write exported file");
        }
    }
    else if (op == "rm")
    {
        arity(1, 1);
        if (!afs.remove(args[1]))
            throw std::runtime_error("File or directory not found");
        dirty = true;
    }
    else if (op == "find")
    {
        arity(1, 1);
        for (auto &name : afs.find_file(args[1]))
            std::cout << name << '\n';
    }
    else if (op == "du")
    {
        arity(0, 1);
        const std::string path = args.size() == 2 ? args[1] : ".";
        auto copy = afs;
        std::string content;
        if (!copy.change_directory(path) && afs.get_content(path, content))
            throw std::runtime_error("File or directory not found");
        size_t allocated = 0, size = 0;
        for (auto &[name, usage] : afs.disk_usage(path))
        {
            std::cout << name << " allocated=" << usage.first << " bytes=" << usage.second << '\n';
            allocated += usage.first;
            size += usage.second;
        }
        std::cout << "total allocated=" << allocated << " bytes=" << size << '\n';
    }
    else if (op == "boot")
    {
        arity(1, 1);
        afs.set_boot(read_boot(args[1]));
        dirty = true;
    }
    else
        throw std::runtime_error("Unknown command: " + op);
    return true;
}
static void save(const fs::path &path, const ANC216::AFS &afs)
{
    auto temporary = path;
    temporary += ".tmp." + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    try
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out || !out.write(afs.get_buffer(), 65536))
            throw std::runtime_error("Cannot save card image");
        out.close();
        if (!out)
            throw std::runtime_error("Cannot close card image");
        fs::rename(temporary, path);
    }
    catch (...)
    {
        std::error_code error;
        fs::remove(temporary, error);
        throw;
    }
}
int main(int argc, char **argv)
{
    try
    {
        if (argc < 2 || std::string(argv[1]) == "--help")
        {
            help();
            return 0;
        }
        if (std::string(argv[1]) == "--format")
        {
            if (argc != 3 && !(argc == 5 && std::string(argv[3]) == "--boot"))
                throw std::runtime_error("Expected --format <new image> [--boot <file>]");
            if (fs::exists(argv[2]))
                throw std::runtime_error("Image already exists; choose a new image path");
            ANC216::AFS afs;
            if (argc == 5)
                afs.set_boot(read_boot(argv[4]));
            save(argv[2], afs);
            return 0;
        }
        const fs::path path = fs::absolute(argv[1]);
        std::ifstream in(path, std::ios::binary);
        if (!in)
            throw std::runtime_error("Cannot read card image");
        ANC216::AFS afs(in);
        in.close();
        if (afs.get_corrupted())
            throw std::runtime_error("Corrupted AFS v1 card image");
        bool dirty = false;
        if (argc > 2)
        {
            std::vector<std::string> args(argv + 2, argv + argc);
            command(afs, args, dirty);
        }
        else
        {
            std::string line;
            while (std::cout << afs.get_current_dir_absolute_path() << "> " && std::getline(std::cin, line))
            {
                std::istringstream stream(line);
                std::vector<std::string> args;
                std::string arg;
                while (stream >> std::quoted(arg))
                    args.push_back(arg);
                try
                {
                    if (!command(afs, args, dirty))
                        break;
                }
                catch (const std::exception &e)
                {
                    std::cerr << "cardreader: " << e.what() << '\n';
                }
            }
        }
        if (dirty)
            save(path, afs);
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "cardreader: " << e.what() << '\n';
        return 1;
    }
}
