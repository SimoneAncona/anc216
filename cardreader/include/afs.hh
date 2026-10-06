#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

// AFS v1 uses the original repository's on-disk layout. See doc/IMPLEMENTATION.md.
#define MAGIC_NUMBER 0xFE
#define VERSION 0x01
#define DIR_NAME_SIZE 15
#define FILE_NAME_SIZE 17
#define FILE_EXT_SIZE 3
#define CLUSTER_SIZE 323
#define MAX_CLUSTER_COUNT 196
#define MAX_DIR_COUNT 96
#define BOOTX_SIZE 255
#define BLOCK_INFO_ADDRESS (2 + BOOTX_SIZE)
#define DIR_INFO_ADDRESS (BLOCK_INFO_ADDRESS + MAX_CLUSTER_COUNT * 2)
#define DATA_ADDRESS (DIR_INFO_ADDRESS + (DIR_NAME_SIZE + 1) * MAX_DIR_COUNT)
#define CLUSTER_UNUSED 0
#define CLUSTER_USED 1
#define CLUSTER_CONTINUE 2
#define INVALID_DIRNAME_FILENAME 1
#define DIRNAME_FILENAME_TOO_LONG 2
#define DIRNAME_FILENAME_EMPTY 3
#define DIRNAME_FILENAME_ALREADY_EXIST 4
#define FILENAME_EXTENSION_TOO_LONG 5
#define NO_MORE_SPACE -1
#define get_block_address(blockid) (((blockid) - 1) * CLUSTER_SIZE + DATA_ADDRESS)
#define get_real_dir_address(dirid) (((dirid) - 1) * (DIR_NAME_SIZE + 1) + DIR_INFO_ADDRESS)

namespace ANC216
{
    class AFS
    {
        static constexpr unsigned first_header = FILE_NAME_SIZE + FILE_EXT_SIZE + 3;
        static constexpr unsigned first_capacity = CLUSTER_SIZE - first_header;
        static constexpr unsigned next_capacity = CLUSTER_SIZE - 3;
        std::array<uint8_t, 65536> buffer{};
        unsigned current = 0;
        bool corrupt = false;
        static unsigned block(unsigned id)
        {
            return get_block_address(id);
        }
        static unsigned dir(unsigned id)
        {
            return get_real_dir_address(id);
        }
        unsigned kind(unsigned id) const
        {
            return buffer[BLOCK_INFO_ADDRESS + 2 * (id - 1) + 1];
        }
        unsigned owner(unsigned id) const
        {
            return buffer[BLOCK_INFO_ADDRESS + 2 * (id - 1)];
        }
        void metadata(unsigned id, unsigned parent, unsigned type)
        {
            buffer[BLOCK_INFO_ADDRESS + 2 * (id - 1)] = parent;
            buffer[BLOCK_INFO_ADDRESS + 2 * (id - 1) + 1] = type;
        }
        std::string text(unsigned at, unsigned size) const
        {
            std::string result;
            for (unsigned i = 0; i < size && buffer[at + i]; ++i)
                result += char(buffer[at + i]);
            return result;
        }
        void text(unsigned at, unsigned size, const std::string &value)
        {
            std::fill_n(buffer.begin() + at, size, 0);
            std::copy(value.begin(), value.end(), buffer.begin() + at);
        }
        unsigned word(unsigned at) const
        {
            return (unsigned(buffer[at]) << 8) | buffer[at + 1];
        }
        void word(unsigned at, unsigned value)
        {
            buffer[at] = value >> 8;
            buffer[at + 1] = value;
        }
        bool valid_dir(unsigned id) const
        {
            return id == 0 || (id <= MAX_DIR_COUNT && buffer[dir(id) + 1] != 0);
        }
        std::string filename(unsigned id) const
        {
            unsigned at = block(id);
            auto base = text(at, FILE_NAME_SIZE), ext = text(at + FILE_NAME_SIZE, FILE_EXT_SIZE);
            return base + (ext.empty() ? "" : "." + ext);
        }
        int child(const std::string &name, unsigned parent) const
        {
            for (unsigned id = 1; id <= MAX_DIR_COUNT; ++id)
                if (buffer[dir(id)] == parent && text(dir(id) + 1, DIR_NAME_SIZE) == name && valid_dir(id))
                    return id;
            return -1;
        }
        int file(const std::string &name, unsigned parent) const
        {
            for (unsigned id = 1; id <= MAX_CLUSTER_COUNT; ++id)
                if (kind(id) == CLUSTER_USED && owner(id) == parent && filename(id) == name)
                    return id;
            return -1;
        }
        int directory(const std::string &path, unsigned start) const
        {
            unsigned result = path.starts_with('/') ? 0 : start;
            std::istringstream stream(path);
            std::string part;
            while (std::getline(stream, part, '/'))
            {
                if (part.empty() || part == ".")
                    continue;
                if (part == "..")
                {
                    result = result ? buffer[dir(result)] : 0;
                    continue;
                }
                int found = child(part, result);
                if (found < 0)
                    return -1;
                result = found;
            }
            return result;
        }
        std::pair<int, std::string> parent(const std::string &path, int start = -1) const
        {
            if (path.empty() || path.back() == '/')
                return {-1, ""};
            auto pos = path.find_last_of('/');
            if (pos == std::string::npos)
                return {start < 0 ? int(current) : start, path};
            return {directory(pos == 0 ? "/" : path.substr(0, pos), start < 0 ? current : start), path.substr(pos + 1)};
        }
        static bool name_ok(const std::string &name)
        {
            if (name.empty() || name == "." || name == "..")
                return false;
            for (unsigned char c : name)
                if (c < 32 || c == 127 || c == '/' || c == '\\' || c == ':')
                    return false;
            return true;
        }
        std::vector<unsigned> chain(unsigned id) const
        {
            std::vector<unsigned> ids;
            std::set<unsigned> seen;
            while (id)
            {
                if (id > MAX_CLUSTER_COUNT || !seen.insert(id).second)
                    throw std::runtime_error("Invalid AFS cluster chain");
                ids.push_back(id);
                id = buffer[block(id) + (ids.size() == 1 ? FILE_NAME_SIZE + FILE_EXT_SIZE : 0)];
            }
            return ids;
        }
        void free_file(unsigned id)
        {
            for (auto b : chain(id))
            {
                metadata(b, 0, 0);
                std::fill_n(buffer.begin() + block(b), CLUSTER_SIZE, 0);
            }
        }
        void free_dir(unsigned id)
        {
            for (unsigned f = 1; f <= MAX_CLUSTER_COUNT; ++f)
                if (kind(f) == CLUSTER_USED && owner(f) == id)
                    free_file(f);
            for (unsigned d = 1; d <= MAX_DIR_COUNT; ++d)
                if (valid_dir(d) && buffer[dir(d)] == id)
                    free_dir(d);
            std::fill_n(buffer.begin() + dir(id), 16, 0);
        }
        bool validate() const
        {
            if (buffer[0] != MAGIC_NUMBER || buffer[1] != VERSION)
                return false;
            std::set<std::pair<unsigned, std::string>> names;
            for (unsigned d = 1; d <= MAX_DIR_COUNT; ++d)
                if (valid_dir(d))
                {
                    auto name = text(dir(d) + 1, DIR_NAME_SIZE);
                    if (!name_ok(name) || !valid_dir(buffer[dir(d)]) || !names.emplace(buffer[dir(d)], name).second)
                        return false;
                    std::set<unsigned> seen;
                    for (unsigned p = d; p; p = buffer[dir(p)])
                        if (!valid_dir(p) || !seen.insert(p).second)
                            return false;
                }
            std::set<unsigned> used;
            for (unsigned b = 1; b <= MAX_CLUSTER_COUNT; ++b)
            {
                if (kind(b) > CLUSTER_CONTINUE)
                    return false;
                if (kind(b) != CLUSTER_USED)
                    continue;
                if (!valid_dir(owner(b)) || !name_ok(filename(b)) || text(block(b), FILE_NAME_SIZE).empty() || !names.emplace(owner(b), filename(b)).second)
                    return false;
                auto ids = chain(b);
                for (unsigned j = 0; j < ids.size(); ++j)
                {
                    unsigned id = ids[j], at = block(id), header = j == 0 ? first_header : 3;
                    if (!used.insert(id).second || kind(id) != (j == 0 ? CLUSTER_USED : CLUSTER_CONTINUE) || word(at + header - 2) > CLUSTER_SIZE - header)
                        return false;
                }
            }
            for (unsigned b = 1; b <= MAX_CLUSTER_COUNT; ++b)
                if (kind(b) == CLUSTER_CONTINUE && !used.contains(b))
                    return false;
            return true;
        }

    public:
        AFS()
        {
            buffer[0] = MAGIC_NUMBER;
            buffer[1] = VERSION;
        }
        explicit AFS(std::ifstream &input)
        {
            input.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
            corrupt = input.gcount() != long(buffer.size()) || input.peek() != EOF;
            if (!corrupt)
            {
                try
                {
                    corrupt = !validate();
                }
                catch (...)
                {
                    corrupt = true;
                }
            }
        }
        bool get_corrupted() const
        {
            return corrupt;
        }
        char *get_buffer()
        {
            return reinterpret_cast<char *>(buffer.data());
        }
        const char *get_buffer() const
        {
            return reinterpret_cast<const char *>(buffer.data());
        }
        std::string get_current_dir() const
        {
            return current ? text(dir(current) + 1, DIR_NAME_SIZE) : "";
        }
        std::string get_current_dir_absolute_path(int address = -1) const
        {
            unsigned d = address < 0 ? current : unsigned(address);
            std::string result;
            for (; d; d = buffer[dir(d)])
                result = "/" + text(dir(d) + 1, DIR_NAME_SIZE) + result;
            return result.empty() ? "/" : result;
        }
        bool change_directory(const std::string &path, int start = -1)
        {
            int found = directory(path, start < 0 ? current : start);
            if (found < 0)
                return false;
            current = found;
            return true;
        }
        std::vector<std::string> get_sub_diectories(int address = -1) const
        {
            unsigned parent = address < 0 ? current : address;
            std::vector<std::string> result;
            for (unsigned d = 1; d <= MAX_DIR_COUNT; ++d)
                if (valid_dir(d) && buffer[dir(d)] == parent)
                    result.push_back(text(dir(d) + 1, DIR_NAME_SIZE));
            return result;
        }
        std::vector<std::string> get_files(int address = -1) const
        {
            unsigned parent = address < 0 ? current : address;
            std::vector<std::string> result;
            for (unsigned b = 1; b <= MAX_CLUSTER_COUNT; ++b)
                if (kind(b) == CLUSTER_USED && owner(b) == parent)
                    result.push_back(filename(b));
            return result;
        }
        int make_dir(const std::string &path)
        {
            auto [p, name] = parent(path);
            if (p < 0 || !name_ok(name))
                return INVALID_DIRNAME_FILENAME;
            if (name.size() > DIR_NAME_SIZE)
                return DIRNAME_FILENAME_TOO_LONG;
            if (child(name, p) >= 0 || file(name, p) >= 0)
                return DIRNAME_FILENAME_ALREADY_EXIST;
            for (unsigned d = 1; d <= MAX_DIR_COUNT; ++d)
                if (!valid_dir(d))
                {
                    buffer[dir(d)] = p;
                    text(dir(d) + 1, DIR_NAME_SIZE, name);
                    return 0;
                }
            return NO_MORE_SPACE;
        }
        int touch(const std::string &path)
        {
            auto [p, name] = parent(path);
            if (p < 0 || !name_ok(name))
                return INVALID_DIRNAME_FILENAME;
            auto pos = name.find_last_of('.');
            auto base = pos == std::string::npos ? name : name.substr(0, pos), ext = pos == std::string::npos ? "" : name.substr(pos + 1);
            if (base.empty() || base.size() > FILE_NAME_SIZE)
                return DIRNAME_FILENAME_TOO_LONG;
            if (ext.size() > FILE_EXT_SIZE)
                return FILENAME_EXTENSION_TOO_LONG;
            if (pos != std::string::npos && ext.empty())
                return INVALID_DIRNAME_FILENAME;
            if (child(name, p) >= 0 || file(name, p) >= 0)
                return DIRNAME_FILENAME_ALREADY_EXIST;
            for (unsigned b = 1; b <= MAX_CLUSTER_COUNT; ++b)
                if (kind(b) == CLUSTER_UNUSED)
                {
                    std::fill_n(buffer.begin() + block(b), CLUSTER_SIZE, 0);
                    metadata(b, p, CLUSTER_USED);
                    text(block(b), FILE_NAME_SIZE, base);
                    text(block(b) + FILE_NAME_SIZE, FILE_EXT_SIZE, ext);
                    return 0;
                }
            return NO_MORE_SPACE;
        }
        int get_content(const std::string &path, std::string &content, int start = -1) const
        {
            auto [p, name] = parent(path, start);
            if (p < 0)
                return INVALID_DIRNAME_FILENAME;
            int id = file(name, p);
            if (id < 0)
                return INVALID_DIRNAME_FILENAME;
            content.clear();
            auto ids = chain(id);
            for (unsigned j = 0; j < ids.size(); ++j)
            {
                unsigned at = block(ids[j]), header = j == 0 ? first_header : 3;
                content.append(reinterpret_cast<const char *>(buffer.data() + at + header), word(at + header - 2));
            }
            return 0;
        }
        std::vector<char> get_file_content(const std::string &name) const
        {
            std::string result;
            if (get_content(name, result))
                return {};
            return {result.begin(), result.end()};
        }
        int set_content(const std::string &path, std::ifstream &input, int start = -1)
        {
            std::string content;
            char chunk[4096];
            while (input.read(chunk, sizeof(chunk)) || input.gcount())
            {
                content.append(chunk, input.gcount());
                if (content.size() > first_capacity + (MAX_CLUSTER_COUNT - 1) * next_capacity)
                    return NO_MORE_SPACE;
            }
            if (input.bad())
                throw std::runtime_error("Cannot read file content");
            return set_content(path, content, start);
        }
        int set_content(const std::string &path, const std::string &content, int start = -1)
        {
            auto [p, name] = parent(path, start);
            if (p < 0)
                return INVALID_DIRNAME_FILENAME;
            int id = file(name, p);
            if (id < 0)
                return INVALID_DIRNAME_FILENAME;
            size_t need = 1 + (content.size() > first_capacity ? (content.size() - first_capacity + next_capacity - 1) / next_capacity : 0);
            auto old = chain(id);
            std::set<unsigned> reusable(old.begin(), old.end());
            std::vector<unsigned> ids{unsigned(id)};
            for (unsigned b = 1; b <= MAX_CLUSTER_COUNT && ids.size() < need; ++b)
                if (b != unsigned(id) && (kind(b) == CLUSTER_UNUSED || reusable.contains(b)))
                    ids.push_back(b);
            if (ids.size() != need)
                return NO_MORE_SPACE; // no writes until capacity is known
            std::string base = text(block(id), FILE_NAME_SIZE), ext = text(block(id) + FILE_NAME_SIZE, FILE_EXT_SIZE);
            free_file(id);
            size_t consumed = 0;
            for (unsigned j = 0; j < ids.size(); ++j)
            {
                unsigned at = block(ids[j]), header = j == 0 ? first_header : 3;
                unsigned count = std::min<size_t>(CLUSTER_SIZE - header, content.size() - consumed);
                metadata(ids[j], p, j == 0 ? CLUSTER_USED : CLUSTER_CONTINUE);
                if (j == 0)
                {
                    text(at, FILE_NAME_SIZE, base);
                    text(at + FILE_NAME_SIZE, FILE_EXT_SIZE, ext);
                }
                buffer[at + header - 3] = j + 1 < ids.size() ? ids[j + 1] : 0;
                word(at + header - 2, count);
                std::copy_n(content.begin() + consumed, count, buffer.begin() + at + header);
                consumed += count;
            }
            return 0;
        }
        bool remove(const std::string &path, int start = -1)
        {
            auto [p, name] = parent(path, start);
            if (p < 0)
                return false;
            int f = file(name, p);
            if (f >= 0)
            {
                free_file(f);
                return true;
            }
            int d = child(name, p);
            if (d < 0)
                return false;
            for (unsigned a = current; a; a = buffer[dir(a)])
                if (a == unsigned(d))
                {
                    current = 0;
                    break;
                }
            free_dir(d);
            return true;
        }
        bool remove_file(const std::string &path, int start = -1)
        {
            auto [p, n] = parent(path, start);
            int f = p < 0 ? -1 : file(n, p);
            if (f < 0)
                return false;
            free_file(f);
            return true;
        }
        bool remove_dir(const std::string &path, int start = -1)
        {
            auto [p, n] = parent(path, start);
            if (p < 0 || child(n, p) < 0)
                return false;
            return remove(path, start);
        }
        std::vector<std::string> find_file(const std::string &name, int address = 0) const
        {
            std::vector<std::string> result;
            for (auto &f : get_files(address))
                if (f == name)
                    result.push_back((get_current_dir_absolute_path(address) == "/" ? "" : get_current_dir_absolute_path(address)) + "/" + f);
            for (unsigned d = 1; d <= MAX_DIR_COUNT; ++d)
                if (valid_dir(d) && buffer[dir(d)] == address)
                {
                    auto r = find_file(name, d);
                    result.insert(result.end(), r.begin(), r.end());
                }
            return result;
        }
        std::map<std::string, std::pair<size_t, size_t>> disk_usage(const std::string &path, int start = -1) const
        {
            unsigned initial = start < 0 ? current : start;
            int d = directory(path, initial);
            std::map<std::string, std::pair<size_t, size_t>> result;
            if (d >= 0)
            {
                for (auto &f : get_files(d))
                {
                    auto r = disk_usage(f, d);
                    result.insert(r.begin(), r.end());
                }
                for (auto &sub : get_sub_diectories(d))
                {
                    auto r = disk_usage(sub, d);
                    result.insert(r.begin(), r.end());
                }
            }
            else
            {
                auto [p, n] = parent(path, initial);
                int f = p < 0 ? -1 : file(n, p);
                if (f < 0)
                    return {};
                auto ids = chain(f);
                size_t size = 0;
                for (unsigned j = 0; j < ids.size(); ++j)
                    size += word(block(ids[j]) + (j == 0 ? first_header : 3) - 2);
                auto root = get_current_dir_absolute_path(p);
                result[(root == "/" ? root : root + "/") + n] = {ids.size() * CLUSTER_SIZE, size};
            }
            return result;
        }
        void set_boot(const std::vector<uint8_t> &boot)
        {
            if (boot.size() > BOOTX_SIZE)
                throw std::runtime_error("AFS boot code exceeds 255 bytes");
            std::fill_n(buffer.begin() + 2, BOOTX_SIZE, 0);
            std::copy(boot.begin(), boot.end(), buffer.begin() + 2);
        }
    };
} // namespace ANC216
