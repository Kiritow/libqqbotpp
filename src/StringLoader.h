#pragma once
#include <string>
#include <map>

class StringLoader
{
public:
    StringLoader();
    StringLoader(const std::string& filename);
    void load(const std::string& filename);
    bool isReady() const;

    std::string get(const std::string& index);
    std::string operator [](const std::string& index);
private:
    std::map<std::string,std::string> _m;
    int _status;
};
