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
        size_t current_address = 0;

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
        size_t get_label_address(const std::string &name)
        {
            auto label = env.labels.find(name);
            if (label == env.labels.end())
            {
                return -1;
            }
            return (*label).second.address;
        }

        std::vector<char> dump_instructions()
        {
            std::vector<char> res;
            for (auto &ins : env.instructions)
            {
                auto x = dump_instruction(ins);
                for (auto ch : x)
                    res.push_back(ch);
            }
            return res;
        }

        Label current_label()
        {
            Label label;
            for (auto &lb : env.labels)
            {
                if (current_address >= lb.second.address)
                {
                    label = lb.second;
                }
            }
            return label;
        }

        std::string current_label_name()
        {
            std::string label;
            for (auto &lb : env.labels)
            {
                if (current_address >= lb.second.address)
                {
                    label = lb.first;
                }
            }
            return label;
        }

        int eval_expression(AST *ast)
        {
            auto children = ast->get_children();

            if (children.size() == 0)
            {
                if (ast->get_token() == "$")
                    return static_cast<int>(current_address);

                if (ast->get_token().type == NUMBER_LITERAL)
                    return stoi(ast->get_token().value, nullptr, 0);

                if (ast->get_token().type == IDENTIFIER)
                {
                    auto label = env.labels.find(ast->get_token().value);
                    if (label == env.labels.end())
                    {
                        error_stack.push_back({"Undefined reference to '" + ast->get_token().value + "'", ast->get_token()});
                        return 0;
                    }
                    if ((*label).second.is_local && current_label().module_name != (*label).second.module_name)
                    {
                        error_stack.push_back({"Cannot access to '" + ast->get_token().value + "'", ast->get_token()});
                        return 0;
                    }
                    return static_cast<int>((*label).second.address);
                }
            }

            if (children.size() == 1)
            {
                if (ast->get_token() == "+")
                    return eval_expression(ast->get_children()[1]);

                if (ast->get_token() == "-")
                    return -eval_expression(ast->get_children()[1]);

                return eval_expression(children[0]);
            }

            if (children[0]->get_token() == "word" || children[0]->get_token() == "byte")
                    return eval_expression(children[1]);

            if (children[0]->get_token() == "sizeof")
            {
                auto label = env.labels.find(children[1]->get_token().value);
                if (label == env.labels.end())
                {
                    error_stack.push_back({"Undefined reference to '" + children[1]->get_token().value + "' label", children[1]->get_token()});
                    return 0;
                }
                if ((*label).second.is_local && current_label().module_name != (*label).second.module_name)
                {
                    error_stack.push_back({"Cannot access to '" + children[1]->get_token().value + "'", children[1]->get_token()});
                    return 0;
                }
                return static_cast<int>((*label).second.size);
            }

            if (children[0]->get_token() == "offset")
            {
                auto var = env.variables.find(children[1]->get_token().value);
                if (var == env.variables.end())
                {
                    error_stack.push_back({"Undefined reference to '" + children[1]->get_token().value + "' variable", children[1]->get_token()});
                    return 0;
                }
                if ((*var).second.label != current_label_name())
                {
                    error_stack.push_back({"Undefined reference to '" + children[1]->get_token().value + "' variable", children[1]->get_token()});
                    return 0;
                }
                return static_cast<int>((*var).second.bp_relative_address);
            }

            int value = eval_expression(children[0]);
            if (children[1]->get_token() == "+")
            {
                return value + eval_expression(children[2]);
            }
            if (children[1]->get_token() == "-")
            {
                return value - eval_expression(children[2]);
            }
            if (children[1]->get_token() == "*")
            {
                return value * eval_expression(children[2]);
            }
            if (children[1]->get_token() == "/")
            {
                return value / eval_expression(children[2]);
            }
            return value;
        }

        std::vector<unsigned char> dump_instruction(Instruction &ins)
        {
            if (ins.instruction == "reserve")
            {
                std::vector<unsigned char> res;
                auto cnt = eval_expression(ins.op1);
                for (auto i = 0; i < cnt; i++)
                    res.push_back(0);
                return res;
            }
            if (ins.instruction == "expression")
            {
                std::vector<unsigned char> res;
                if (ins.addr_mode_size == BYTE_S)
                {
                    res.push_back(eval_expression(ins.op1));
                    return res;
                }
                auto val = eval_expression(ins.op1);
                res.push_back(val & 0xFF00 >> 8);
                res.push_back(val);
                return res;
            }
            if (ins.instruction == "string")
            {
                std::vector<unsigned char> res;
                for (auto &ch : ins.op1->get_token().value)
                    res.push_back(ch);
                return res;
            }
            if (ins.instruction.starts_with("j"))
            {
                if (ins.addressing_mode == IMMEDIATE_BYTE || ins.addressing_mode == IMMEDIATE_WORD)
                {
                    ins.addressing_mode = MEMORY_ABSOULTE;
                }
                else if (ins.addressing_mode == MEMORY_RELATIVE_TO_PC)
                {
                    auto adr = eval_expression(ins.op1);
                    if (is_id(ins.op1))
                        ins.op1 = new AST(Token{std::to_string(adr - current_address).c_str(), NUMBER_LITERAL});
                }
            }
            unsigned char addr = build_addressing_mode(ins);
            unsigned char opc = isa[ins.instruction].first;
            std::vector<unsigned char> args = get_arguments(ins);
            std::vector<unsigned char> res = {addr, opc};
            for (auto ch : args)
            {
                res.push_back(ch);
            }
            return res;
        }

        bool is_id(AST *ast)
        {
            if (ast->get_token().type == IDENTIFIER)
                return true;
            if (ast->get_children().size() != 1)
                return false;
            return is_id(ast->get_children()[0]);
        }

        unsigned char build_addressing_mode(const Instruction &ins)
        {
            switch (ins.addressing_mode)
            {
            case IMPLIED_MODE:
                return 0;
            case IMMEDIATE_BYTE:
                return 0b00'001'000;
            case IMMEDIATE_WORD:
                return 0b00'010'000;
            case REGISTER_ACCESS_MODE:
                return (ins.op1->get_token().value[0] == 'r' ? 1 : 2) | ((ins.op1->get_token().value[1] - '0') << 3);
            case REGISTER_TO_REGISTER_MODE:
                return 0b01'000'000 | ((ins.op1->get_token().value[1] - '0') << 3) | (ins.op2->get_token().value[1] - '0');
            case MEMORY_ABSOULTE:
                return 0b10'000'000;
            case MEMORY_ABSOULTE_INDEXED:
                return 0b10'000'001 | ((ins.indexing.second->get_token().value[1] - '0') << 3);
            case MEMORY_INDIRECT:
                return 0b10'000'010;
            case MEMORY_INDIRECT_INDEXED:
                return 0b10'000'011 | ((ins.indexing.second->get_token().value[1] - '0') << 3);
            case MEMORY_RELATIVE_TO_PC:
                return 0b10'000'100;
            case MEMORY_RELATIVE_TO_PC_WITH_REGISTER:
                return 0b10'000'101 | ((ins.indexing.second->get_token().value[1] - '0') << 3);
            case MEMORY_RELATIVE_TO_BP:
                return 0b10'001'100;
            case MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
                return 0b10'000'110 | ((ins.indexing.second->get_token().value[1] - '0') << 3);
            case IMMEDIATE_TO_MEMORY_ABSOLUTE:
                return ins.addr_mode_size == WORD_S + WORD_S ? 0b1110 : 0b110;
            case IMMEDIATE_TO_MEMORY_ABSOLUTE_INDEXED:
                return 0b111 | ((ins.indexing.second->get_token().value[1] - '0') << 3);
            case IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP:
                return ins.addr_mode_size == WORD_S + BYTE_S ? 0b1011 : 0b11;
            case IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
                return (ins.addr_mode_size == WORD_S ? 0b101 : 0b100) | ((ins.indexing.second->get_token().value[1] - '0') << 3);
            case REGISTER_TO_MEMORY_ABSOULTE:   
                return (ins.op2->get_token().value[0] == 'l' ? 0b11'000'100 : 0b11'000'000) | (ins.op2->get_token().value[1] - '0') << 3;
            case MEMORY_ABSOULTE_TO_REGISTER:
                return (ins.op1->get_token().value[0] == 'l' ? 0b11'000'100 : 0b11'000'000) | (ins.op1->get_token().value[1] - '0') << 3;
            case IMMEDIATE_TO_REGISTER:
                return (ins.op1->get_token().value[0] == 'l' ? 0b11'000'101 : 0b11'000'001) | (ins.op1->get_token().value[1] - '0') << 3;
            case REGISTER_TO_MEMORY_RELATIVE_TO_PC:
                return (ins.op2->get_token().value[0] == 'l' ? 0b11'000'110 : 0b11'000'010) | (ins.op2->get_token().value[1] - '0') << 3;
            case MEMORY_RELATIVE_TO_PC_TO_REGISTER:
                return (ins.op1->get_token().value[0] == 'l' ? 0b11'000'110 : 0b11'000'010) | (ins.op1->get_token().value[1] - '0') << 3;
            case REGISTER_TO_MEMORY_RELATIVE_TO_BP:
                return (ins.op2->get_token().value[0] == 'l' ? 0b11'000'111 : 0b11'000'011) | (ins.op2->get_token().value[1] - '0') << 3;
            case MEMORY_RELATIVE_TO_BP_TO_REGISTER:
                return (ins.op1->get_token().value[0] == 'l' ? 0b11'000'111 : 0b11'000'011) | (ins.op1->get_token().value[1] - '0') << 3;
            }
            return 0;
        }

        std::vector<unsigned char> get_arguments(const Instruction &ins)
        {
            std::vector<unsigned char> res;
            short a;
            switch (ins.addressing_mode)
            {
            case IMPLIED_MODE:
            case REGISTER_ACCESS_MODE:
            case REGISTER_TO_REGISTER_MODE:
                break;
            case IMMEDIATE_BYTE:
            case MEMORY_RELATIVE_TO_PC:
            case MEMORY_RELATIVE_TO_PC_WITH_REGISTER:
            case MEMORY_RELATIVE_TO_BP:
            case MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
            case REGISTER_TO_MEMORY_RELATIVE_TO_PC:
            case REGISTER_TO_MEMORY_RELATIVE_TO_BP:
                res.push_back(eval_expression(ins.op1));
                break;
            case IMMEDIATE_WORD:
            case MEMORY_ABSOULTE:
            case MEMORY_ABSOULTE_INDEXED:
            case MEMORY_INDIRECT:
            case MEMORY_INDIRECT_INDEXED:
            case MEMORY_ABSOULTE_TO_REGISTER:
                a = eval_expression(ins.op1);
                res.push_back(a >> 8);
                res.push_back(a & 0xFF);
                break;
            case IMMEDIATE_TO_MEMORY_ABSOLUTE:
            case IMMEDIATE_TO_MEMORY_ABSOLUTE_INDEXED:
                a = eval_expression(ins.op1);
                res.push_back(a >> 8);
                res.push_back(a & 0xFF);
                if (ins.addr_mode_size == WORD_S + BYTE_S)
                {
                    res.push_back(eval_expression(ins.op2));
                }
                else
                {
                    a = eval_expression(ins.op2);
                    res.push_back(a >> 8);
                    res.push_back(a & 0xFF);
                }
            case IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP:
                res.push_back(static_cast<unsigned char>(eval_expression(ins.indexing.second)) * (ins.indexing.first == '-' ? -1 : 1));
                if (ins.addr_mode_size == WORD_S)
                {
                    res.push_back(eval_expression(ins.op1));
                }
                else
                {
                    a = eval_expression(ins.op1);
                    res.push_back(a >> 8);
                    res.push_back(a & 0xFF);
                }
            case IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
                if (ins.addr_mode_size == BYTE_S)
                {
                    res.push_back(eval_expression(ins.op1));
                }
                else
                {
                    a = eval_expression(ins.op1);
                    res.push_back(a >> 8);
                    res.push_back(a & 0xFF);
                }
            case REGISTER_TO_MEMORY_ABSOULTE:
                a = eval_expression(ins.op2);
                res.push_back(a >> 8);
                res.push_back(a & 0xFF);
                break;
            case IMMEDIATE_TO_REGISTER:
                if (ins.op1->get_token().value[0] == 'l')
                {
                    res.push_back(eval_expression(ins.op2));
                }
                else
                {
                    a = eval_expression(ins.op2);
                    res.push_back(a >> 8);
                    res.push_back(a & 0xFF);
                }
            case MEMORY_RELATIVE_TO_BP_TO_REGISTER:
            case MEMORY_RELATIVE_TO_PC_TO_REGISTER:
                res.push_back(eval_expression(ins.op2));
                break;
            }
            return res;
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
            return dump_instructions();
        }

        inline bool has_errors()
        {
            for (auto &error : error_stack)
            {
                if (!error.is_warning())
                    return true;
            }
            return false;
        }

        inline std::vector<Error> &get_error_stack()
        {
            return error_stack;
        }
    };
}