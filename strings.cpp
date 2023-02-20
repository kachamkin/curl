#include "mainwindow.h"

void rtrim(string& s)
{
    s.erase(find_if(s.rbegin(), s.rend(), [](char ch) {return !isspace(ch); }).base(), s.end());
}

void ltrim(string& s)
{
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](char ch) {return !isspace(ch); }));
}

void toLower(string& s)
{
    transform(s.begin(), s.end(), s.begin(), [](char ch) { return tolower(ch); });
}

void trim(string& s)
{
    ltrim(s);
    rtrim(s);
}
