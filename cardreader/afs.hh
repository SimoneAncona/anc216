#include <string>
#include <vector>
#include <filesystem>

#pragma once

namespace fs = std::filesystem;

#define MAGIC_NUMBER 0xFE
#define BLOCK_INFO_ADDRESS 257
#define DIR_INFO_ADDRESS 649
#define DATA_ADDRESS 2185

#define DIR_NAME_SIZE 15
#define FILE_NAME_SIZE 17
#define FILE_EXT_SIZE 3

#define CLUSTER_UNUSED 0
#define CLUSTER_USED 1
#define CLUSTER_CONTINUE 2

#define INVALID_DIRNAME_FILENAME 1
#define DIRNAME_FILENAME_TOO_LONG 2
#define DIRNAME_FILENAME_EMPTY 3
#define DIRNAME_FILENAME_ALREADY_EXIST 4
#define NO_MORE_SPACE -1

#define get_block_address(blockid) ((blockid - 1) * 322) + DATA_ADDRESS
#define get_real_dir_address(dirid) ((dirid - 1) * DIR_NAME_SIZE) + DIR_INFO_ADDRESS

namespace ANC216
{
    class AFS
    {
    private:
        char buffer[65'536];
        const char* current_dir;
        char current_dir_addr;
        bool is_corrupt;
        std::fstream &file;

        bool check_corrupt()
        {
            if (buffer[0] != MAGIC_NUMBER) return false;
        }

        char get_upper_dir(char dir_addr)
        {
            return buffer[get_real_dir_address(dir_addr)];
        }

        std::string get_dirname(size_t index)
        {
            std::string res = "";
            if (!isalpha(buffer[index])) return res;
            for (size_t i = index; i < index + DIR_NAME_SIZE; i++)
            {
                if (!isalnum(buffer[i])) return "";
                res += buffer[i];
            }
            return res;
        }

        size_t find_file_current_dir(const std::string &filename)
        {
            for (size_t i = BLOCK_INFO_ADDRESS, block_id = 1; i < DIR_INFO_ADDRESS; i += 2, block_id++)
            {
                if (buffer[i] == current_dir_addr && buffer[i + 1] == CLUSTER_USED && filename == get_filename(get_block_address(block_id)))
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
                if (buffer[i] == 0) break;
                res += buffer[i];
            }
            for (size_t i = index + FILE_NAME_SIZE; i < index + FILE_NAME_SIZE + FILE_EXT_SIZE; i++)
            {
                if (buffer[i] == 0) break;
                res += buffer[i];
            }
            return res;
        }

    public:
        AFS(std::fstream &file)
            :file(file)
        {
            this->file.read(buffer, sizeof(buffer));
            this->is_corrupt = check_corrupt();
            this->current_dir = "";
            current_dir_addr = 0;
        }

        inline std::string get_current_dir()
        {
            return current_dir;
        }

        std::string get_current_dir_absolute_path()
        {
            std::string res = "/";
            res += current_dir;

            char temp_dir_addr = current_dir_addr;
            while (get_upper_dir(temp_dir_addr) != 0 && temp_dir_addr != 0)
            {
                temp_dir_addr = get_upper_dir(temp_dir_addr);
                res = "/" + get_dirname(get_real_dir_address(temp_dir_addr)) + res;
            }
            return res;
        }

        std::vector<std::string> get_sub_diectories()
        {
            std::vector<std::string> dirs;
            std::string dirname;
            for (size_t i = DIR_INFO_ADDRESS; i < DATA_ADDRESS; i += DIR_NAME_SIZE + 1)
            {
                if (buffer[i] == current_dir_addr && buffer[i + 1] != 0)
                {
                    dirname = get_dirname(i + 1);
                    if (dirname != "")
                        dirs.push_back(dirname);
                }
            }
            return dirs;
        }

        void change_directory(const std::string& dirname)
        {

        }

        std::vector<char> get_file_content(const std::string &filename)
        {
            std::vector<char> res;
            size_t index = find_file_current_dir(filename);
            char next = buffer[index + FILE_NAME_SIZE + FILE_EXT_SIZE];
            short size = buffer[index + FILE_NAME_SIZE + FILE_EXT_SIZE + 2] << 8 | buffer[index + FILE_NAME_SIZE + FILE_EXT_SIZE + 1];
            return res;
        }

        void save()
        {
            this->file.write(buffer, sizeof(buffer));
        }

        void close()
        {
            this->file.close();
        }

        bool get_corrupted()
        {
            return is_corrupt;
        }

        std::vector<std::string> get_files()
        {
            std::vector<std::string> res;
            for (size_t i = BLOCK_INFO_ADDRESS, block_id = 1; i < DIR_INFO_ADDRESS; i += 2, block_id++) 
            {
                if (buffer[i] == current_dir_addr && buffer[i + 1] == CLUSTER_USED)
                    res.push_back(get_filename(get_block_address(block_id)));
            }
            return res;
        }

        int touch(const std::string &fname)
        {
            std::string extension = fname.substr(fname.find_first_of("."));
            std::string filename = fname.substr(0, fname.find_first_of("."));
            for (char ch : filename)
            {
                if (!isalnum(ch) && ch != '_' && ch != '(' && ch != ' ' && ch != ')')
                    return INVALID_DIRNAME_FILENAME;
            }
            if (filename.empty())
                return DIRNAME_FILENAME_TOO_LONG;
            if (filename.size() > FILE_NAME_SIZE)
                return DIRNAME_FILENAME_TOO_LONG;
            if (extension.size() > FILE_EXT_SIZE)
                return DIRNAME_FILENAME_TOO_LONG;
            auto files = get_files();
            for (auto file : files)
            {
                if (file == filename)
                    return DIRNAME_FILENAME_ALREADY_EXIST;
            }
            for (size_t i = BLOCK_INFO_ADDRESS, block_id = 1; i < DIR_INFO_ADDRESS; i += 2, block_id++)
            {
                if (buffer[i + 1] == CLUSTER_UNUSED)
                {
                    buffer[i] = current_dir_addr;
                    buffer[i + 1] = CLUSTER_USED;
                    for (size_t j = get_block_address(block_id), k = 0; j < get_block_address(block_id) + FILE_NAME_SIZE; j++, k++)
                        buffer[j] = filename[k];
                    for (size_t j = get_block_address(block_id) + FILE_NAME_SIZE, k = 0; j < get_block_address(block_id) + FILE_NAME_SIZE + FILE_EXT_SIZE; j++, k++)
                        buffer[j] = extension[k];
                    buffer[get_block_address(block_id) + FILE_NAME_SIZE + FILE_EXT_SIZE] = 0;
                    buffer[get_block_address(block_id) + FILE_NAME_SIZE + FILE_EXT_SIZE + 1] = 0;
                    buffer[get_block_address(block_id) + FILE_NAME_SIZE + FILE_EXT_SIZE + 2] = 0;
                    return 0;
                }
            }
            return NO_MORE_SPACE;
        }

        int make_dir(const std::string& dirname)
        {
            for (char ch : dirname)
            {
                if (!isalnum(ch) && ch != '_' && ch != '(' && ch != ' ' && ch != ')')
                    return INVALID_DIRNAME_FILENAME;
            }
            if (dirname.empty())
                return DIRNAME_FILENAME_TOO_LONG;
            if (dirname.size() > DIR_NAME_SIZE)
                return DIRNAME_FILENAME_TOO_LONG;
            auto dirs = get_sub_diectories();
            for (auto dir : dirs)
            {
                if (dir == dirname)
                    return DIRNAME_FILENAME_ALREADY_EXIST;
            }
            for (size_t i = DIR_INFO_ADDRESS; i < DATA_ADDRESS; i += DIR_NAME_SIZE + 1)
            {
                if (buffer[i] == 0 && buffer[i + 1] == 0)
                {
                    buffer[i] = current_dir_addr;
                    for (size_t j = 0; j < DIR_NAME_SIZE; j++)
                        buffer[i + j] = dirname[j];
                    return 0;
                }
            }
            return NO_MORE_SPACE;
        }
    };
}