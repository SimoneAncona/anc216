#include <ast.hh>
#include <vector>
#include <map>
#include <types.hh>
#include <tuple>
#include <string>
#include <isa.hh>

#pragma once

#define BYTE_S 1
#define WORD_S 2

namespace ANC216
{
    struct Label
    {
        size_t address;
        bool is_local;
        std::string module_name;
        std::string section;
    };

    struct Structure
    {
        std::map<std::string, char> fields;
    };

    struct Variable
    {
        size_t bp_relative_address;
        std::string label;
        AST *assign;
    };

    struct Instruction
    {
        AddressingMode addressing_mode;
        AST *op1 = nullptr;
        AST *op2 = nullptr;
        std::pair<char, AST *> indexing;
        std::string instruction;
        size_t addr_mode_size;
        std::string label;
    };

    struct Environment
    {
        std::map<std::string, Label> labels;
        std::map<std::string, Structure> structures;
        std::map<std::string, Variable> variables;
        std::vector<Instruction> instructions;
    };

    class Analyzer
    {
    private:
        AST *ast;
        std::vector<Error> error_stack;
        Environment env;

        size_t current_address;
        size_t bp_relative_address;
        std::string current_label;
        std::string current_section;

        void analyze_command(AST *ast)
        {
            for (auto el : ast->get_children())
            {
                switch (el->get_rule_name())
                {
                case SECTION:
                    analyze_section(el);
                    break;
                case STRUCT_DEF:
                    analyze_structure(el);
                    break;
                case DECLARATION:
                    analyze_declaration(el);
                    break;
                case LABEL:
                    analyze_label(el);
                    break;
                case INSTRUCTION_RULE:
                    analyze_instruction(el);
                    break;
                case EXPRESSION_LIST:
                    analyze_expression_list(el);
                    break;
                case COMMAND:
                    analyze_command(el);
                    break;
                case ORIGIN:
                    analyze_origin(el);
                    break;
                }
            }
        }

        void analyze_origin(AST *ast)
        {
            size_t value = eval_expression(ast->get_children()[1]);
            if (value < current_address)
            {
                error_stack.push_back({"Cannot set address, the address must be grater than or equal to the current address value", ast->get_children()[0]->get_token()});
                return;
            }
            current_address = value;
        }

        void analyze_section(AST *ast)
        {
            current_section = ast->get_children()[1]->get_token().value;
        }

        void analyze_structure(AST *ast)
        {
            env.structures[ast->get_children()[1]->get_token().value] = {};
            analyze_struct_el(ast->get_children()[3], ast->get_children()[1]->get_token().value);
        }

        void analyze_struct_el(AST *ast, const std::string &name)
        {
            env.structures[name].fields[ast->get_children()[0]->get_token().value] = ast->get_children()[2]->get_token().value == "byte" ? BYTE_S : WORD_S;
            if (ast->get_children()[ast->get_children().size() - 1]->get_rule_name() == STRUCT_DEF)
            {
                analyze_struct_el(ast->get_children()[ast->get_children().size() - 1], name);
            }
        }

        void analyze_declaration(AST *ast)
        {
            if (current_label == "")
            {
                error_stack.push_back({"Cannot use var macro here", ast->get_children()[0]->get_token()});
                return;
            }
            auto children = ast->get_children();
            std::string name = children[1]->get_token().value;
            if (env.variables.find(name) != env.variables.end())
            {
                if (env.variables[name].label == current_label)
                {
                    error_stack.push_back({"Redefinition of \"" + name + "\"", children[1]->get_token()});
                    return;
                }
            }
            size_t size = 1;
            size_t mul = 1;
            size_t i = 3;
            if (children[3]->get_token() == "[")
            {
                i = 5;
                size = eval_expression(children[4]);
                mul = size;
            }
            if (children[i]->get_token().type == IDENTIFIER)
            {
                if (env.structures.find(children[i]->get_token().value) == env.structures.end())
                {
                    error_stack.push_back({"Undefined structure type \"" + children[i]->get_token().value + "\"", children[i]->get_token()});
                    return;
                }
                size_t struct_size = 0;
                auto strct = env.structures[children[i]->get_token().value];
                for (auto el : strct.fields)
                {
                    struct_size += el.second;
                }
                size *= struct_size;
            }
            else
            {
                size *= children[i]->get_token() == "byte" ? BYTE_S : WORD_S;
            }
            i++;
            bool is_assigned = false;
            if (i < children.size())
            {
                if (children[i]->get_token() == "]")
                    i++;
                if (i < children.size())
                {
                    int res = eval_expression(children[i + 1]);
                    if (res >= pow(2, (8 * size)) || res < -pow(2, (8 * size - 1)))
                    {
                        error_stack.push_back({"The value exceeds the size of the variable", children[i]->get_token(), true});
                    }
                    is_assigned = true;
                }
            }
            env.variables[name] = {bp_relative_address, current_label, is_assigned ? children[i + 1] : nullptr};
            bp_relative_address + size;
            Instruction ins;
            if (is_assigned)
            {
                ins.addressing_mode = size == WORD_S ? IMMEDIATE_WORD : IMMEDIATE_BYTE;
                ins.op1 = children[i + 1];
                ins.addr_mode_size = size;
                ins.instruction = "push";
                current_address += size + WORD_S;
                env.instructions.push_back(ins);
                return;
            }
            if (size > WORD_S)
            {
                ins.addressing_mode = REGISTER_ACCESS_MODE;
                ins.op1 = new AST(Token{"r0", REGISTER});
                ins.addr_mode_size = 0;
                ins.instruction = "trsp";
                current_address += WORD_S;
                env.instructions.push_back(ins);
                ins.addressing_mode = IMMEDIATE_TO_REGISTER;
                ins.op1 = new AST(Token{"r0", REGISTER});
                ins.op2 = new AST(Token{std::to_string(size).c_str(), NUMBER_LITERAL});
                ins.addr_mode_size = WORD_S;
                ins.instruction = "add";
                current_address += WORD_S * 2;
                env.instructions.push_back(ins);
                ins.addressing_mode = REGISTER_ACCESS_MODE;
                ins.op1 = new AST(Token{"r0", REGISTER});
                ins.addr_mode_size = 0;
                ins.instruction = "ldsp";
                current_address += WORD_S;
                env.instructions.push_back(ins);
                return;
            }
            ins.addressing_mode = size == WORD_S ? IMMEDIATE_WORD : IMMEDIATE_BYTE;
            ins.op1 = new AST(Token{"0", NUMBER_LITERAL});
            ins.addr_mode_size = size;
            ins.instruction = "push";
            current_address += size + WORD_S;
            env.instructions.push_back(ins);
        }

        void analyze_label(AST *ast)
        {
            bool is_local = false;
            size_t i = 0;
            if (*ast->get_children()[0] == "local")
            {
                is_local = true;
                i++;
            }
            else if (*ast->get_children()[0] == "global")
                i++;
            std::string name = ast->get_children()[i]->get_token().value;
            if (env.labels.find(name) != env.labels.end())
            {
                error_stack.push_back({"Redefinition of \"" + name + "\"", ast->get_children()[i]->get_token()});
                return;
            }
            env.labels[ast->get_children()[i]->get_token().value] = {current_address, is_local, ast->get_children()[i]->get_token().module_name};
            current_label = name;
            bp_relative_address = 0;
        }

        void analyze_instruction(AST *ast)
        {
            Instruction insr;
            if (ast->get_children().size() == 1)
                insr = {IMPLIED_MODE, nullptr, nullptr, {}, ast->get_children()[0]->get_token().value, 0};
            else
            {
                insr = get_instruction(ast->get_children()[1]);
                insr.instruction = ast->get_children()[0]->get_token().value;
            }
            bool found = false;
            for (auto adr : isa[insr.instruction].second)
            {
                if (adr == get_family(insr.addressing_mode))
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                error_stack.push_back({"'" + insr.instruction + "' does not support " + addr_to_string(insr.addressing_mode) + " addressing mode", ast->get_children()[0]->get_token()});
                return;
            }
            insr.label = current_label;
            env.instructions.push_back(insr);
            current_address += insr.addr_mode_size + WORD_S;
        }

        std::string addr_to_string(AddressingMode addr)
        {
            switch (addr)
            {
            case IMPLIED_MODE:
                return "implied";
            case IMMEDIATE_BYTE:
                return "immediate single byte";
            case IMMEDIATE_WORD:
                return "immediate word";
            case REGISTER_ACCESS_MODE:
                return "register access";
            case REGISTER_TO_REGISTER_MODE:
                return "register to register";
            case MEMORY_ABSOULTE:
                return "memory absolute";
            case MEMORY_ABSOULTE_INDEXED:
                return "memory absolute indexed";
            case MEMORY_INDIRECT:
                return "memory indirect";
            case MEMORY_INDIRECT_INDEXED:
                return "memory indirect indexed";
            case MEMORY_RELATIVE_TO_PC:
                return "memory relative to PC";
            case MEMORY_RELATIVE_TO_PC_WITH_REGISTER:
                return "memory relative to PC with register";
            case MEMORY_RELATIVE_TO_BP:
                return "memory relative to BP";
            case MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
                return "memory relative to BP with register";
            case IMMEDIATE_TO_MEMORY_ABSOLUTE:
                return "immediate to absolute memory";
            case IMMEDIATE_TO_MEMORY_ABSOLUTE_INDEXED:
                return "immediate to absolute memory indexed";
            case IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP:
                return "immediate to memory relative to BP";
            case IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
                return "immediate to memory relative to BP with register";
            case REGISTER_TO_MEMORY_ABSOULTE:
                return "register to memory absoulte";
            case MEMORY_ABSOULTE_TO_REGISTER:
                return "memory absoulte to register";
            case IMMEDIATE_TO_REGISTER:
                return "immediate to register";
            case REGISTER_TO_MEMORY_RELATIVE_TO_PC:
                return "register to memory relative to PC";
            case MEMORY_RELATIVE_TO_PC_TO_REGISTER:
                return "memory relative to PC to register";
            case REGISTER_TO_MEMORY_RELATIVE_TO_BP:
                return "register to memory relative to BP";
            case MEMORY_RELATIVE_TO_BP_TO_REGISTER:
                return "memory relative to BP to register";
            case LOW_REGISTER_TO_MEMORY_ABSOLUTE:
            case MEMORY_ABSOULTE_TO_LOW_REGISTER:
            case IMMEDIATE_TO_LOW_REGISTER:
            case LOW_REGISTER_TO_MEMORY_RELATIVE_TO_PC:
            case MEMORY_RELATIVE_TO_PC_TO_LOW_REGISTER:
            case LOW_REGISTER_TO_MEMORY_RELATIVE_TO_BP:
            case MEMORY_RELATIVE_TO_BP_TO_LOW_REGISTER:
                return "a";
            }
            return "";
        }

        Instruction get_instruction(AST *ast)
        {
            if (ast->get_children()[0]->get_rule_name() == ADDRESSING_MODE_REALTIVE_BP)
                return get_bp_relative(ast->get_children()[0]);
            if (ast->get_children()[0]->get_rule_name() == ADDRESSING_MODE_MEMORY_TO_REGISTER)
                return get_memory_to_reg(ast->get_children()[0]);
            if (ast->get_children()[0]->get_rule_name() == ADDRESSING_MODE_ABSOLUTE)
                return get_absolute(ast->get_children()[0]);
            if (ast->get_children()[0]->get_rule_name() == EXPRESSION)
                return get_immediate(ast->get_children()[0]);
            if (ast->get_children()[0]->get_rule_name() == ADDRESSING_MODE_INDIRECT)
                return get_indirect(ast->get_children()[0]);
            if (ast->get_children()[0]->get_rule_name() == ADDRESSING_MODE_REGISTER_TO_REGISTER)
                return {REGISTER_TO_REGISTER_MODE, ast->get_children()[0]->get_children()[0], ast->get_children()[0]->get_children()[2], {}, "", 0};
            if (ast->get_rule_name() == ADDRESSING_MODE_REGISTER)
                return {REGISTER_ACCESS_MODE, ast->get_children()[0], nullptr, {}, "", 0};
            if (ast->get_children()[0]->get_rule_name() == ADDRESSING_MODE_REALTIVE_PC)
                return get_relative(ast->get_children()[0]);
            if (ast->get_children()[0]->get_rule_name() == ADDRESSING_MODE_REG_TO_BP)
            {
                check_local_variables(ast->get_children()[0]->get_children()[0]->get_token());
                return {REGISTER_TO_MEMORY_RELATIVE_TO_BP, ast->get_children()[0]->get_children()[0], ast->get_children()[0]->get_children()[2], {}, "", 1};
            }
            if (ast->get_children()[0]->get_rule_name() == ADDRESSING_MODE_RELATIVE_ARRAY)
                return get_array_access(ast->get_children()[0]);
            return {};
        }

        Instruction get_relative(AST *ast)
        {
            Instruction ins;
            ins.addressing_mode = MEMORY_RELATIVE_TO_PC;
            ins.addr_mode_size = BYTE_S;
            if (get_expression_size(ast->get_children()[1]) == WORD_S)
                error_stack.push_back({"The size of the argument exceeds the limit of signed byte", ast->get_children()[0]->get_token(), true});
            return ins;
        }

        Instruction get_indirect(AST *ast)
        {
            Instruction ins;
            ins.addr_mode_size = WORD_S;
            ins.op1 = ast->get_children()[1];
            if (ast->get_children().size() == 3)
            {
                ins.addressing_mode = MEMORY_INDIRECT;
                return ins;
            }
            ins.addressing_mode = MEMORY_INDIRECT_INDEXED;
            ins.indexing = {ast->get_children()[3]->get_token().value[0], ast->get_children()[4]};
            return ins;
        }

        Instruction get_immediate(AST *ast)
        {
            Instruction ins;
            ins.addr_mode_size = get_expression_size(ast);
            if (ins.addr_mode_size == WORD_S)
            {
                ins.addressing_mode = IMMEDIATE_WORD;
                return ins;
            }
            ins.addressing_mode = IMMEDIATE_BYTE;
            return ins;
        }

        Instruction get_absolute(AST *ast)
        {
            Instruction ins;
            ins.op1 = ast->get_children()[1];
            if (ast->get_children().size() == 2)
            {
                ins.addressing_mode = MEMORY_ABSOULTE;
                ins.addr_mode_size = WORD_S;
                return ins;
            }
            size_t i = 3;
            bool indexed = false;
            if (ast->get_children()[2]->get_token().value == "+" || ast->get_children()[2]->get_token().value == "-")
            {
                ins.addressing_mode = MEMORY_ABSOULTE_INDEXED;
                ins.addr_mode_size = WORD_S;
                ins.indexing = {ast->get_children()[2]->get_token().value[0], ast->get_children()[3]};
                if (ast->get_children().size() == 4)
                    return ins;
                i = 5;
                indexed = true;
            }
            if (ast->get_children()[i]->get_rule_name() == EXPRESSION)
            {
                ins.addressing_mode = indexed ? IMMEDIATE_TO_MEMORY_ABSOLUTE_INDEXED : IMMEDIATE_TO_MEMORY_ABSOLUTE;
                ins.addr_mode_size = WORD_S + get_expression_size(ast->get_children()[i]);
                ins.op2 = ast->get_children()[i];
                return ins;
            }
            ins.addressing_mode = REGISTER_TO_MEMORY_ABSOULTE;
            ins.addr_mode_size = WORD_S;
            ins.op2 = ast->get_children()[i];
            return ins;
        }

        bool is_var_expression(AST *ast)
        {
            if (ast->get_children().size() != 1)
                return false;
            if (ast->get_children()[0]->get_token().type == IDENTIFIER)
                return env.variables.find(ast->get_children()[0]->get_token().value) != env.variables.end();
            return is_var_expression(ast->get_children()[0]);
        }

        Instruction get_memory_to_reg(AST *ast)
        {
            Instruction ins;
            ins.op1 = ast->get_children()[0];
            if (ast->get_children()[2]->get_rule_name() == EXPRESSION)
            {
                bool is_var = is_var_expression(ast->get_children()[2]);
                ins.addressing_mode = is_var ? MEMORY_RELATIVE_TO_BP_TO_REGISTER : IMMEDIATE_TO_REGISTER;
                ins.addr_mode_size = is_var ? BYTE_S : ins.op1->get_token().value[0] == 'l' ? BYTE_S
                                                                                            : WORD_S;
                if (!is_var && get_expression_size(ast->get_children()[2]) == WORD_S && ins.addr_mode_size == BYTE_S)
                    error_stack.push_back({"Conversion from word to byte may cause a data loss", ast->get_children()[1]->get_token()});
                ins.indexing = {'+', nullptr};
                ins.op2 = ast->get_children()[2];
                return ins;
            }
            if (ast->get_children()[2]->get_token().type == IDENTIFIER)
            {
                check_local_variables(ast->get_children()[2]->get_token());
                ins.addressing_mode = MEMORY_RELATIVE_TO_BP_TO_REGISTER;
                ins.addr_mode_size = BYTE_S;
                if (get_expression_size(ast->get_children()[4]) == WORD_S)
                    error_stack.push_back({"The size of the argument exceeds the limit of signed byte", ast->get_children()[0]->get_token(), true});
                ins.indexing = {'+', ast->get_children()[4]};
                ins.op2 = ast->get_children()[2];
                return ins;
            }
            if (ast->get_children()[2]->get_token() == "&")
            {
                if (ast->get_children()[3]->get_token() == "bp")
                {
                    ins.addressing_mode = MEMORY_RELATIVE_TO_BP_TO_REGISTER;
                    ins.addr_mode_size = BYTE_S;
                    ins.indexing = {ast->get_children()[4]->get_token().value[0], ast->get_children()[5]};
                    return ins;
                }
                ins.addressing_mode = MEMORY_ABSOULTE_TO_REGISTER;
                ins.addr_mode_size = WORD_S;
                return ins;
            }
            if (ast->get_children()[2]->get_token() == "*")
            {
                ins.addressing_mode = MEMORY_RELATIVE_TO_PC_TO_REGISTER;
                ins.addr_mode_size = BYTE_S;
                if (get_expression_size(ast->get_children()[3]) == WORD_S)
                    error_stack.push_back({"The size of the argument exceeds the limit of signed byte", ast->get_children()[0]->get_token(), true});
                ins.op2 = ast->get_children()[3];
                return ins;
            }
            ins.addressing_mode = REGISTER_ACCESS_MODE;
            ins.addr_mode_size = 0;
            return ins;
        }

        void check_local_variables(const Token &token)
        {
            for (auto &var : env.variables)
            {
                if (var.first == token.value && var.second.label == current_label)
                    return;
            }
            error_stack.push_back({"Undefined variable '" + token.value + "'" + get_similar_var(token.value), token});
        }

        std::string get_similar_var(const std::string &id)
        {
            for (auto &var : env.variables)
            {
                if (var.second.label == current_label)
                {
                    if (is_similar(var.first, id))
                        return ". Did you mean '" + var.first + "'?";
                }
            }
            return "";
        }

        bool is_similar(const std::string &s1, const std::string &s2)
        {
            if (!(s1.length() >= s2.length() - 2 && s1.length() <= s2.length() + 2))
                return false;

            size_t l = s1.length() > s2.length() ? s2.length() : s1.length();
            size_t sim = 0;
            for (size_t i = 0, j = 0; i < l; i++)
            {
                if (s1[i] == s2[j])
                {
                    sim++;
                    j++;
                    continue;
                }
                if (j != 0)
                    j++;
            }
            if (sim == 0)
                return false;
            if ((double)sim / l >= 0.75)
                return true;
            return false;
        }

        Instruction get_bp_relative(AST *ast)
        {
            Instruction ins;
            ins.indexing = {ast->get_children()[2]->get_token().value[0], ast->get_children()[3]};
            bool reg_indexed = ast->get_children()[3]->get_rule_name() != EXPRESSION;
            if (ast->get_children().size() == 4)
            {
                ins.addressing_mode = reg_indexed ? MEMORY_RELATIVE_TO_BP_WITH_REGISTER : MEMORY_RELATIVE_TO_BP;
                ins.addr_mode_size = reg_indexed ? 0 : BYTE_S;
                return ins;
            }
            if (ast->get_children()[5]->get_rule_name() == EXPRESSION)
            {
                ins.addressing_mode = reg_indexed ? IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER : IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP;
                ins.addr_mode_size = reg_indexed ? get_expression_size(ast->get_children()[5]) : BYTE_S + get_expression_size(ast->get_children()[5]);
                ins.op1 = ast->get_children()[5];
                return ins;
            }
            ins.addressing_mode = MEMORY_RELATIVE_TO_BP_TO_REGISTER;
            ins.addr_mode_size = BYTE_S;
            ins.op1 = ast->get_children()[5];
            return ins;
        }

        Instruction get_array_access(AST *ast)
        {
            Instruction ins;

            check_local_variables(ast->get_children()[0]->get_token());
            ins.op1 = ast->get_children()[0];
            bool register_indexed = false;
            if (ast->get_children()[2]->get_rule_name() == EXPRESSION)
            {
                if (get_expression_size(ast->get_children()[2]) == WORD_S)
                    error_stack.push_back({"The size of the expression exceeds the size limit. It should be in the range of -128, 127 (or 0, 255)", ast->get_children()[1]->get_token(), true});
                ins.indexing = {'+', ast->get_children()[2]};
            }
            else
            {
                register_indexed = true;
                ins.indexing = {'+', ast->get_children()[2]};
            }

            if (ast->get_children()[5]->get_rule_name() == EXPRESSION)
            {
                ins.addressing_mode = register_indexed ? IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER : IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP;
                ins.addr_mode_size = register_indexed ? get_expression_size(ast->get_children()[5]) : BYTE_S + get_expression_size(ast->get_children()[5]);
                ins.op2 = ast->get_children()[5];
                return ins;
            }
            ins.addressing_mode = REGISTER_TO_MEMORY_RELATIVE_TO_BP;
            ins.addr_mode_size = BYTE_S;
            ins.op2 = ast->get_children()[5];
            return ins;
        }

        void analyze_expression_list(AST *ast)
        {
            Instruction ins;
            switch (ast->get_children()[0]->get_token().type)
            {
            case STRING_LITERAL:
                ins.instruction = "string";
                ins.op1 = ast->get_children()[0];
                ins.addr_mode_size = ast->get_children()[0]->get_token().value.length() - 2;
                current_address += ins.addr_mode_size;
                break;
            case KEYWORD:
                ins.instruction = "reserve";
                ins.op1 = ast->get_children()[1];
                ins.addr_mode_size = eval_expression(ast->get_children()[1]);
                current_address += ins.addr_mode_size;
                break;
            default:
                ins.instruction = "expression";
                ins.op1 = ast->get_children()[0];
                ins.addr_mode_size = get_expression_size(ast->get_children()[0]);
                current_address += ins.addr_mode_size;
                break;
            }
            ins.label = current_label;
            env.instructions.push_back(ins);
            if (ast->get_children()[ast->get_children().size() - 1]->get_rule_name() == EXPRESSION_LIST)
                analyze_expression_list(ast->get_children()[ast->get_children().size() - 1]);
        }

        char get_expression_size(AST *ast)
        {
            auto children = ast->get_children();
            if (children.size() == 0)
            {
                if (ast->get_token().type == IDENTIFIER)
                    return WORD_S;

                if (ast->get_token() == "$")
                    return WORD_S;

                if (ast->get_token().type == NUMBER_LITERAL)
                {
                    auto value = std::stoi(ast->get_token().value, 0);
                    if (value > 65'536 || value < -32'768)
                    {
                        error_stack.push_back({"The size of the literal number exceeds the bit limit", ast->get_token()});
                        return WORD_S;
                    }
                    if (value > 255 || value < -128)
                        return WORD_S;
                    return BYTE_S;
                }
            }
            if (children.size() == 1)
            {
                if (ast->get_token() == "byte")
                {
                    if (get_expression_size(children[0]) > 255 || get_expression_size(children[0]) < -128)
                    {
                        error_stack.push_back({"Byte casting may cause data loss", ast->get_token(), true});
                    }
                    return BYTE_S;
                }
                if (ast->get_token() == "word")
                {
                    get_expression_size(children[0]);
                    return WORD_S;
                }
                if (ast->get_token() == "+" || ast->get_token() == "-")
                    return get_expression_size(children[0]);
                return get_expression_size(children[0]);
            }
            if (children.size() == 2)
            {
                if (children[0]->get_token() == "sizeof")
                    return WORD_S;
                return WORD_S;
            }
            int v1 = eval_expression(children[0]);
            int v2 = eval_expression(children[2]);
            int res;
            if (children[1]->get_token() == "+")
                res = v1 + v2;
            else if (children[1]->get_token() == "-")
                res = v1 - v2;
            else if (children[1]->get_token() == "*")
                res = v1 * v2;
            else
                res = v1 / v2;
            if (res > 65'356 || res < -32'768)
            {
                error_stack.push_back({"The size of the literal number exceeds the bit limit", children[1]->get_token()});
                return WORD_S;
            }
            if (res > 255 || res < -128)
                return WORD_S;
            return BYTE_S;
        }

        int eval_expression(AST *ast)
        {
            auto children = ast->get_children();
            if (ast->get_token() == "sizeof")
            {
                error_stack.push_back({"Cannot evaluate expression, cannot resolve the size of the label", ast->get_children()[0]->get_token()});
                return -1;
            }
            if (ast->get_token().type == IDENTIFIER)
            {
                error_stack.push_back({"Cannot evaluate expression, cannot resolve the address of \"" + ast->get_children()[0]->get_token().value + "\"", ast->get_children()[0]->get_token()});
                return -1;
            }
            if (ast->get_token().type == TYPE)
            {
                error_stack.push_back({"Unexpected type in expression", ast->get_children()[0]->get_token()});
                return -1;
            }
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

                return eval_expression(children[0]);
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

    public:
        Analyzer(AST *ast)
        {
            this->ast = ast;
            current_address = 0;
            current_label = "";
        }

        inline std::vector<char> assemble()
        {
            analyze_command(this->ast);
            if (!error_stack.empty())
                return {};

            return {};
        }

        inline std::vector<Error> &get_error_stack()
        {
            return error_stack;
        }

        inline Environment &get_environment()
        {
            return env;
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
    };
}