#pragma once

#include <tuple>
#include <sstream>

#include <mini/ini.h>
#include <modUtils.hpp>

using namespace ModUtils;

mINI::INIStructure ini;

// use on string literals to allow them to be used in the readConfig function
// "my string here"_s
inline std::string operator"" _s(const char* str, std::size_t) {
    return std::string(str);
}

bool hasSectionAndKey(std::string section, std::string key){
    if (ini.has(section)) {
        if (ini[section].has(key)){
            return true;
        }
    }
    return false;
}

template <typename T>
T fromString(const std::string& str) {
    std::istringstream iss(str);
    T value;
    iss >> value;
    if (iss.fail()) {
        throw std::exception("Failed to parse Data from string.");
    }
    return value;
}

template <typename T>
std::string toString(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

template <typename T>
void getConfigVal(T& value, std::string section, std::string key) {
    try {
        if (hasSectionAndKey(section, key)){
            value = fromString<T>(ini[section][key]);
        } else {
            ini[section][key] = toString<T>(value);
        }
    } catch (std::exception e){
        ini[section][key] = toString<T>(value);
    }
}

#define LVALUE_AND_RVALUE_PAIRS \
    replacePairs(std::string&, std::string&) \
    replacePairs(std::string&, std::string&&) \
    replacePairs(std::string&&, std::string&) \
    replacePairs(std::string&&, std::string&&) \

#define replacePairs(field1, field2) \
    template <typename T, typename... Rest> \
    void getConfigVals(std::tuple<T&, field1, field2> t,Rest&&... rest) { \
        T& value = std::get<0>(t); \
        std::string section = std::get<1>(t); \
        std::string key = std::get<2>(t); \
        getConfigVal(value,section,key); \
        if constexpr (sizeof...(rest) > 0) { \
            getConfigVals(std::forward<Rest>(rest)...); \
        } \
    }
LVALUE_AND_RVALUE_PAIRS

//function to set up ini file
//pass any number of forwarded tuples
//for example: std::forward_as_tuple(splitDamageFix,section,"split damage fix"_s)
template <typename... Args>
void readConfig(Args&&... args) {
    mINI::INIFile config(GetCurrentModPath() + "/" + GetCurrentModName() + ".ini");
    config.read(ini);
    getConfigVals(std::forward<Args>(args)...);
    config.write(ini,true);
}