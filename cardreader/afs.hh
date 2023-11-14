#include <string>
#include <vector>
#include <filesystem>
#include <tuple>
#include <map>

#pragma once

namespace fs = std::filesystem;

#define MAGIC_NUMBER 0xFE
#define VERSION 0x01

#define DIR_NAME_SIZE 15
#define FILE_NAME_SIZE 17
#define FILE_EXT_SIZE 3
#define CLUSTER_SIZE 323

#define MAX_CLUSTER_COUNT 196
#define MAX_DIR_COUNT 96

#define BOOTX_SIZE 255
#define BLOCK_INFO_ADDRESS 2 + BOOTX_SIZE
#define DIR_INFO_ADDRESS BLOCK_INFO_ADDRESS + (MAX_CLUSTER_COUNT * 2)
#define DATA_ADDRESS DIR_INFO_ADDRESS + (DIR_NAME_SIZE + 1) * MAX_DIR_COUNT


#define CLUSTER_UNUSED 0
#define CLUSTER_USED 1
#define CLUSTER_CONTINUE 2

#define INVALID_DIRNAME_FILENAME 1
#define DIRNAME_FILENAME_TOO_LONG 2
#define DIRNAME_FILENAME_EMPTY 3
#define DIRNAME_FILENAME_ALREADY_EXIST 4
#define FILENAME_EXTENSION_TOO_LONG 5
#define NO_MORE_SPACE -1

#define get_block_address(blockid) ((blockid - 1) * CLUSTER_SIZE) + DATA_ADDRESS
#define get_real_dir_address(dirid) ((dirid - 1) * (DIR_NAME_SIZE + 1)) + DIR_INFO_ADDRESS

namespace ANC216
{
    class AFS
    {
    private:
        unsigned char buffer[65'536] = {0};
        std::string current_dir;
        char current_dir_addr;
        bool is_corrupt;
        std::ifstream &file;

        char get_dir_addr(const std::string &dirname, char diraddr)
        {
            char j = 1;
            for (size_t i = DIR_INFO_ADDRESS; i < DATA_ADDRESS; i += DIR_NAME_SIZE + 1, j++)
            {
                if (buffer[i] == diraddr && get_dirname(get_real_dir_address(j) + 1) == dirname)
                {
                   return j;
                }
            }
            return -1;
        }

        bool check_corrupt()
        {
            if (buffer[0] != MAGIC_NUMBER)
                return false;
            return true;
        }

        char get_upper_dir(char dir_addr)
        {
            return buffer[get_real_dir_address(dir_addr)];
        }

        std::string get_dirname(size_t index)
        {
            std::string res = "";
            for (size_t i = index; i < index + DIR_NAME_SIZE; i++)
            {
                if (buffer[i] == 0)
                    break;
                res += buffer[i];
            }
            return res;
        }

        size_t find_file_current_dir(const std::string &filename, char diraddr = -1)
        {
            if (diraddr == -1)
                diraddr = current_dir_addr;
            for (size_t i = BLOCK_INFO_ADDRESS, block_id = 1; i < DIR_INFO_ADDRESS; i += 2, block_id++)
            {
                if (buffer[i] == diraddr && buffer[i + 1] == CLUSTER_USED && filename == get_filename(get_block_address(block_id)))
                {
                    return get_block_address(block_id);
                }
            }
            return -1;
        }

        std::string get_filename(size_t index)
        {
            std::string res = "";
            for (size_t i = index; i < index + FILE_NAME_SIZE; i++)
            {
                if (buffer[i] == 0)
                    break;
                res += buffer[i];
            }
            if (buffer[index + FILE_NAME_SIZE] != 0)
                res += ".";
            for (size_t i = index + FILE_NAME_SIZE; i < index + FILE_NAME_SIZE + FILE_EXT_SIZE; i++)
            {
                if (buffer[i] == 0)
                    break;
                res += buffer[i];
            }
            return res;
        }

        std::pair<size_t, size_t> get_file_size(size_t index)
        {
            size_t real = CLUSTER_SIZE;
            size_t file = buffer[index + FILE_NAME_SIZE + FILE_EXT_SIZE + 1] << 8 | buffer[index + FILE_NAME_SIZE + FILE_EXT_SIZE + 2];

            if (buffer[index + FILE_NAME_SIZE + FILE_EXT_SIZE] != 0)
            {
                auto size = get_next_size(get_block_address(buffer[index + FILE_NAME_SIZE + FILE_EXT_SIZE]));
                real += size.first;
                file += size.second;
            }

            return {real, file};
        }

        std::pair<size_t, size_t> get_next_size(size_t index)
        {
            size_t real = CLUSTER_SIZE;
            size_t file = buffer[index + 1] << 8 | buffer[index + 2];

            if (buffer[index] != 0)
            {
                auto size = get_next_size(get_block_address(buffer[index]));
                real += size.first;
                file += size.second;
            }

            return {real, file};
        }

        int resize_write_file(std::ifstream &file, size_t old_block_id, bool is_continue = false)
        {
            for (size_t i = BLOCK_INFO_ADDRESS, block_id = 1; i < DIR_INFO_ADDRESS; i += 2, block_id++)
            {
                if (buffer[i + 1] == CLUSTER_UNUSED)
                {
                    buffer[i + 1] = CLUSTER_CONTINUE;
                    buffer[get_block_address(old_block_id) + (is_continue ? 0 : FILE_NAME_SIZE + FILE_EXT_SIZE)] = block_id;
                    short j;
                    int value;
                    for (j = 3; j < CLUSTER_SIZE; j++)
                    {
                        value = file.get();
                        if (value == EOF) 
                            break;
                        buffer[get_block_address(block_id) + j] = (unsigned char) value;
                    }
                    buffer[get_block_address(block_id) + 1] = (unsigned char) ((j - 3) >> 8);
                    buffer[get_block_address(block_id) + 2] = (unsigned char) ((j - 3) & 0xFF);
                    if (!file.eof())
                    {
                        resize_write_file(file, block_id, true);
                    }
                    return 0;
                }
            }
            return NO_MORE_SPACE;
        }

    public:
        AFS(std::ifstream &fl)
            : file(fl)
        {
            this->file.read((char*)buffer, sizeof(buffer));
            file.close();
            buffer[0] = MAGIC_NUMBER;
            buffer[1] = VERSION;
            // this->is_corrupt = check_corrupt();
            this->is_corrupt = false;
            this->current_dir = "";
            current_dir_addr = 0;
        }

        inline std::string get_current_dir()
        {
            return current_dir;
        }

        std::string get_current_dir_absolute_path(char diraddr = -1)
        {
            if (diraddr == -1)
                diraddr = current_dir_addr;
            std::string res = "/";
            res += get_dirname(get_real_dir_address(diraddr) + 1);

            while (get_upper_dir(diraddr) != 0 && diraddr != 0)
            {
                diraddr = get_upper_dir(diraddr);
                res = "/" + get_dirname(get_real_dir_address(diraddr) + 1) + res;
            }
            return res;
        }

        std::vector<std::string> find_file(const std::string &filename, char diraddr = 0)
        {
            std::vector<std::string> res;

            auto files = get_files(diraddr);
            for (auto file : files)
            {
                if (file == filename)
                    res.push_back(file);
            }

            char j = 1;
            for (size_t i = DIR_INFO_ADDRESS; i < DATA_ADDRESS; i += DIR_NAME_SIZE + 1, j++)
            {
                if (buffer[i] == diraddr && buffer[i + 1] != 0)
                {
                    auto r = find_file(filename, j);
                    for (auto file : r)
                        res.push_back(get_dirname(get_real_dir_address(j) + 1) + "/" + file);
                }
            }

            return res;
        }

        bool remove(std::string name, char diraddr = -1)
        {
            if (diraddr == -1)
                diraddr = current_dir_addr;
            if (get_dir_addr(name, diraddr) != -1)
                return remove_dir(name, diraddr);
            return remove_file(name, diraddr);
        }

        int set_content(const std::string &file, std::ifstream &content, char diraddr = -1)
        {
            if (diraddr == -1)
                diraddr = current_dir_addr;

            for (size_t i = BLOCK_INFO_ADDRESS, block_id = 1; i < DIR_INFO_ADDRESS; i += 2, block_id++)
            {
                if (buffer[i] == diraddr && buffer[i + 1] == CLUSTER_USED && get_filename(get_block_address(block_id)) == file)
                {
                    int value;
                    short j;
                    for (j = FILE_NAME_SIZE + FILE_EXT_SIZE + 3; j < CLUSTER_SIZE; j++)
                    {
                        value = content.get();
                        if (value == EOF) return 0;
                        buffer[j + get_block_address(block_id)] = value;
                    }
                    buffer[get_block_address(block_id) + FILE_NAME_SIZE + FILE_EXT_SIZE + 1] = (unsigned char) ((j - (FILE_NAME_SIZE + FILE_EXT_SIZE + 3)) >> 8);
                    buffer[get_block_address(block_id) + FILE_NAME_SIZE + FILE_EXT_SIZE + 2] = (unsigned char) ((j - (FILE_NAME_SIZE + FILE_EXT_SIZE + 3)) & 0xFF);
                    if (!content.eof())
                    {
                        return resize_write_file(content, block_id);
                    }
                }
            }
            return INVALID_DIRNAME_FILENAME;
        }

        bool remove_file(std::string file, char diraddr = -1)
        {
            if (diraddr == -1)
                diraddr = current_dir_addr;
            for (size_t i = BLOCK_INFO_ADDRESS, block_id = 1; i < DIR_INFO_ADDRESS; i += 2, block_id++)
            {
                if (buffer[i] == diraddr && buffer[i + 1] == CLUSTER_USED && get_filename(get_block_address(block_id)) == file)
                {
                    buffer[i + 1] = CLUSTER_UNUSED;
                    for (size_t j = get_block_address(block_id); j < get_block_address(block_id) + FILE_NAME_SIZE + FILE_EXT_SIZE; j++)
                        buffer[j] = 0;
                    return true;
                }
            }
            return false;
        }

        std::map<std::string, std::pair<size_t, size_t>> disk_usage(const std::string &name, char diraddr = -1)
        {
            if (diraddr == -1)
                diraddr = current_dir_addr;
            char addr;
            if (name == "." || name == "./")
                addr = current_dir_addr;
            else if (name == "/" || name == "\\")
                addr = 0;
            else
                addr = get_dir_addr(name, diraddr);
            if (addr != -1)
            {
                std::map<std::string, std::pair<size_t, size_t>> res;
                std::map<std::string, std::pair<size_t, size_t>> r;
                auto dirs = get_sub_diectories(addr);
                auto files = get_files(addr);
                for (auto &dir : dirs)
                {
                    r = disk_usage(dir, addr);
                    for (auto s : r)
                        res[s.first] = s.second;
                }
                for (auto &file : files)
                {
                    r = disk_usage(file, addr);
                    for (auto s : r)
                        res[s.first] = s.second;
                }
                return res;
            }
            auto file_addr = find_file_current_dir(name, diraddr);
            if (file_addr == -1) return {};
            return {{diraddr == 0 ? "./" + name : "./" + get_dirname(get_real_dir_address(diraddr) + 1) + "/" + name, get_file_size(file_addr)}};
        }

        bool remove_dir(const std::string &dirname, char diraddr = -1)
        {
            if (diraddr == -1)
                diraddr = current_dir_addr;
            char j = 1;
            for (size_t i = DIR_INFO_ADDRESS; i < DATA_ADDRESS; i += DIR_NAME_SIZE + 1, j++)
            {
                if (buffer[i] == diraddr && get_dirname(i + 1) == dirname)
                {
                    auto subdirs = get_sub_diectories(j);
                    auto files = get_files(j);
                    for (auto& dir : subdirs)
                    {
                        remove_dir(dir, j);
                    }
                    for (auto& file : files)
                    {
                        remove_file(file, j);
                    }
                    buffer[i] = 0;
                    for (size_t k = i + 1; k < DIR_NAME_SIZE + i + 1; k++)
                        buffer[k] = 0;
                    
                    return true;
                }
            }
            return false;
        }

        std::vector<std::string> get_sub_diectories(char diraddr = -1)
        {
            if (diraddr == -1)
                diraddr = current_dir_addr;
            std::vector<std::string> dirs;
            std::string dirname;
            for (size_t i = DIR_INFO_ADDRESS; i < DATA_ADDRESS; i += DIR_NAME_SIZE + 1)
            {
                if (buffer[i] == diraddr && buffer[i + 1] != 0)
                {
                    dirname = get_dirname(i + 1);
                    if (dirname != "")
                        dirs.push_back(dirname);
                }
            }
            return dirs;
        }

        bool change_directory(const std::string &dirname, char diraddr = -1)
        {
            if (diraddr == -1)
                diraddr = current_dir_addr;
            if (dirname == "./" || dirname == ".")
            {
                return true;
            }
            if (dirname == "/")
            {
                current_dir_addr = 0;
                current_dir = "";
                return true;
            }
            if (dirname == "..")
            {
                current_dir_addr = buffer[get_real_dir_address(diraddr)];
                current_dir = get_dirname(get_real_dir_address(diraddr) + 1);
                return true;
            }
            std::string dir;
            char j = 1;
            for (size_t i = DIR_INFO_ADDRESS; i < DATA_ADDRESS; i += DIR_NAME_SIZE + 1, j++)
            {
                dir = get_dirname(i + 1);
                if (dir == dirname && buffer[i] == diraddr)
                {
                    current_dir_addr = j;
                    current_dir = dir;
                    return true;
                }
            }
            return false;
        }

        std::vector<char> get_file_content(const std::string &filename)
        {
            std::vector<char> res;
            size_t index = find_file_current_dir(filename);
            char next = buffer[index + FILE_NAME_SIZE + FILE_EXT_SIZE];
            short size = buffer[index + FILE_NAME_SIZE + FILE_EXT_SIZE + 2] << 8 | buffer[index + FILE_NAME_SIZE + FILE_EXT_SIZE + 1];
            return res;
        }

        bool get_corrupted()
        {
            return is_corrupt;
        }

        std::vector<std::string> get_files(char diraddr = -1)
        {
            if (diraddr == -1)
                diraddr = current_dir_addr;
            std::vector<std::string> res;
            for (size_t i = BLOCK_INFO_ADDRESS, block_id = 1; i < DIR_INFO_ADDRESS; i += 2, block_id++)
            {
                if (buffer[i] == diraddr && buffer[i + 1] == CLUSTER_USED)
                    res.push_back(get_filename(get_block_address(block_id)));
            }
            return res;
        }

        int touch(const std::string &fname)
        {
            std::string extension;
            std::string filename = fname;
            if (fname.find_first_of(".") == std::string::npos)
            {
                extension = "";
            }
            else
            {
                extension = fname.substr(fname.find_first_of(".") + 1);
                filename = fname.substr(0, fname.find_first_of("."));
            }
            for (char ch : filename)
            {
                if (!iswalnum(ch) && ch != '_' && ch != '(' && ch != ' ' && ch != ')')
                    return INVALID_DIRNAME_FILENAME;
            }
            for (char ch : extension)
            {
                if (!iswalnum(ch) && ch != '_' && ch != '(' && ch != ' ' && ch != ')')
                    return INVALID_DIRNAME_FILENAME;
            }
            if (filename.empty())
                return DIRNAME_FILENAME_TOO_LONG;
            if (filename.size() > FILE_NAME_SIZE)
                return DIRNAME_FILENAME_TOO_LONG;
            if (extension.size() > FILE_EXT_SIZE)
                return FILENAME_EXTENSION_TOO_LONG;
            auto files = get_files();
            for (auto file : files)
            {
                if (file == fname)
                    return DIRNAME_FILENAME_ALREADY_EXIST;
            }
            auto dirs = get_sub_diectories();
            for (auto dir : dirs)
            {
                if (dir == fname)
                    return DIRNAME_FILENAME_ALREADY_EXIST;
            }
            for (size_t i = BLOCK_INFO_ADDRESS, j, block_id = 1; i < DIR_INFO_ADDRESS; i += 2, block_id++)
            {
                if (buffer[i + 1] == CLUSTER_UNUSED)
                {
                    buffer[i] = current_dir_addr;
                    buffer[i + 1] = CLUSTER_USED;
                    for (j = 0; j < filename.size(); j++)
                        buffer[j + get_block_address(block_id)] = filename[j];
                    for (j++; j < FILE_NAME_SIZE; j++)
                        buffer[j + get_block_address(block_id)] = 0;
                    for (j = 0; j < extension.size(); j++)
                        buffer[j + get_block_address(block_id) + FILE_NAME_SIZE] = extension[j];
                    for (j++; j < FILE_EXT_SIZE; j++)
                        buffer[j + get_block_address(block_id) + FILE_NAME_SIZE] = 0;
                    buffer[get_block_address(block_id) + FILE_NAME_SIZE + FILE_EXT_SIZE] = 0;
                    buffer[get_block_address(block_id) + FILE_NAME_SIZE + FILE_EXT_SIZE + 1] = 0;
                    buffer[get_block_address(block_id) + FILE_NAME_SIZE + FILE_EXT_SIZE + 2] = 0;
                    return 0;
                }
            }
            return NO_MORE_SPACE;
        }

        int make_dir(const std::string &dirname)
        {
            for (char ch : dirname)
            {
                if (!iswalnum(ch) && ch != '_' && ch != '(' && ch != ' ' && ch != ')')
                    return INVALID_DIRNAME_FILENAME;
            }
            if (dirname.empty())
                return DIRNAME_FILENAME_TOO_LONG;
            if (dirname.size() > DIR_NAME_SIZE)
                return DIRNAME_FILENAME_TOO_LONG;
            auto files = get_files();
            for (auto file : files)
            {
                if (file == dirname)
                    return DIRNAME_FILENAME_ALREADY_EXIST;
            }
            auto dirs = get_sub_diectories();
            for (auto dir : dirs)
            {
                if (dir == dirname)
                    return DIRNAME_FILENAME_ALREADY_EXIST;
            }
            char j = 0;
            for (size_t i = DIR_INFO_ADDRESS; i < DATA_ADDRESS; i += DIR_NAME_SIZE + 1)
            {
                if (buffer[i] == 0 && buffer[i + 1] == 0)
                {
                    buffer[i] = current_dir_addr;
                    i++;
                    for (j = 0; j < dirname.size(); j++)
                        buffer[i + j] = dirname[j];
                    for (j++; j < DIR_NAME_SIZE; j++)
                        buffer[i + j] = 0;
                    return 0;
                }
            }
            return NO_MORE_SPACE;
        }

        char *get_buffer()
        {
            return (char*)buffer;
        }
    };
}