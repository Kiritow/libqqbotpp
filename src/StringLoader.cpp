#include "StringLoader.h"
#include <fstream>
#include <cstdio>
using namespace std;

StringLoader::StringLoader()
{
    _status=-1;
}

StringLoader::StringLoader(const std::string& filename)
{
    _status=-1;
    load(filename);
}

void StringLoader::load(const string& filename)
{
    ifstream ifs(filename);
    if(ifs)
    {
        _m.clear();
        _status=0;
        string numline;
        string content;

        while(getline(ifs,numline))
        {
            getline(ifs,content);
            _m[numline]=content;
        }

        _status=1;
    }
}

bool StringLoader::isReady() const
{
    return _status>0;
}

string StringLoader::get(const string& index)
{
    map<string,string>::iterator iter=_m.find(index);
    if(iter!=_m.end())
    {
        return iter->second;
    }
    else
    {
        return "StringLoaderError: Not Found";
    }
}

string StringLoader::operator[](const string& index)
{
    return get(index);
}
