#pragma once
#include "LinkedList.h"
#include <string>

class DslValidator {
public:
    DslValidator() = default;
    
    bool validateInput(const std::string& input, LinkedList<std::string>& errors, bool& lastDslOk);

private:
    // Helper functions
    static std::string to_lower_copy(const std::string& s);
    static void trim(std::string& s);
    static bool is_identifier(const std::string& s);
    static std::string read_identifier_after(const std::string& lower, const std::string& original, const std::string& keyword);
    static std::string read_value_after(const std::string& lower, const std::string& original, const std::string& keyword);
    static int extract_ints(const std::string& str, int out[], int maxn);
};
