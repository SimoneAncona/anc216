#include <ast.hh>
#include <isa.hh>
#include <analyzer.hh>
#ifdef _DEBUG
#include <iostream>
#include <iomanip>
#endif

namespace ANC216
{
    class Assembler
    {
    private:
        Environment &env;
        std::vector<Error> error_stack;

#ifdef _DEBUG
        void print_env()
        {
            for (auto &label : env.labels)
            {
                std::cout << YELLOW << label.first << "\t"
                          << " " << CYAN << std::setw(4) << std::setfill('0') << std::hex << label.second.address << std::endl;
            }
            size_t current_address = 0;
            for (auto &ins : env.instructions)
            {
                std::cout << CYAN << std::hex << std::setw(4) << std::setfill('0') << current_address << RESET << "\t" << ins.instruction << std::endl;
                if (ins.instruction != "reserve" && ins.instruction != "string" && ins.instruction != "expression")
                    current_address += ins.addr_mode_size + WORD_S;
                else
                    current_address += ins.addr_mode_size;
            }
        }
#endif
        size_t get_label_address(const std::string &label)
        {
            return env.labels[label].address;
        }

    public:
        Assembler(Environment &env)
            : env(env)
        {
#ifdef _DEBUG
            print_env();
#endif
        }

        inline std::vector<char> assemble()
        {
        }
    };
}