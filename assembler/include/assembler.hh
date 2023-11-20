#include <ast.hh>
#include <isa.hh>
#include <analyzer.hh>

namespace ANC216
{
    class Assembler
    {
    private:
        Environment &env;
        std::vector<Error> error_stack;

#ifdef _DEBUG
#include <iostream>
#include <iomanip>
        void print_env()
        {
            for (auto &label : env.labels)
            {
                std::cout << YELLOW << label.first << "\t" << " " << CYAN << std::setw(4) << std::setfill('0') << std::hex << label.second.address << std::endl;
            }
            size_t current_address = 0;
            for (auto &ins : env.instructions)
            {
                std::cout << CYAN << std::setw(4) << std::setfill('0') << std::hex << get_label_adddress(ins.label) + current_address << RESET << "\t" << ins.instruction << std::endl;
                if (ins.instruction != "reserve" && ins.instruction != "string" && ins.instruction != "expression")
                    current_address += ins.addr_mode_size;
                else 
                    current_address += ins.addr_mode_size + WORD_S;
            }
        }
#endif
        size_t get_label_adddress(const std::string &label)
        {
            return env.labels[label].address;
        }
    public:
        Assembler(Environment &env)
            : env(env)
        {
            print_env();
        }

        inline std::vector<char> assemble()
        {
            
        }
    };
}