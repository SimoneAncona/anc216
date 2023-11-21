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
        size_t get_label_address(const std::string &label)
        {
            return env.labels[label].address;
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
            }

            if (children.size() == 1)
            {
                if (ast->get_token() == "+")
                    return eval_expression(ast->get_children()[1]);

                if (ast->get_token() == "-")
                    return -eval_expression(ast->get_children()[1]);

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

        std::vector<char> dump_instruction(const Instruction &ins)
        {
            if (ins.instruction == "reserve")
            {
                std::vector<char> res;
                auto cnt = eval_expression(ins.op1);
                for (auto i = 0; i < cnt; i++)
                    res.push_back(0);
                return res;
            }
            if (ins.instruction == "expression")
            {
                std::vector<char> res;
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
                std::vector<char> res;
                for (auto &ch : ins.op1->get_token().value)
                    res.push_back(ch);
                return res;
            }
            return {};
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