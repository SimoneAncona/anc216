#include <string>
#include <vector>

namespace A216
{
    class AST
    {
    private:
        bool final;
        std::string rule_name;
        std::string value;
        std::vector<AST &> children;

    public:
        AST() = default;
        ~AST() = default;

        inline bool is_final()
        {
            return final;
        }

        inline std::vector<AST &> get_children()
        {
            return children;
        }

        inline std::string get_rule_name()
        {
            return rule_name;
        }

        inline std::string get_value()
        {
            return value;
        }
    };
}