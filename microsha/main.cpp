#include <iostream>
#include <stdio.h>
//#include <unistd.h>
//#include <errno.h>
//#include <sys/types.h>
#include <vector>
 
using namespace std;

vector<string> parsing(string s, const string& delim){
    size_t pos = 0;
    string token;
    vector<string> ans;
    while ((pos = s.find(delim)) != std::string::npos) {
        token = s.substr(0, pos);
        ans.push_back(token);
        s.erase(0, pos + delim.length());
    }
    if (!s.empty()) { ans.push_back(s); }
    return ans;
}

int main() {
    vector<string> v = parsing("cat < a | cat > b | cat", " ");
    for (size_t i = 0; i < v.size(); i++) {
        cout << v[i] + '\n';
    }
}