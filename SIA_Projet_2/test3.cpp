#include <string>
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

void vectorPrint(vector<double> v){
    for (double n : v) {
        cout << n << " , ";
    }
    cout << endl;
}

vector<double> getValuesFromLine(string line, char delimiter) {
    /**
     * Parses a line with a delimiter
     */
    vector<double> values;
    while (line.find(delimiter)!=string::npos){
        size_t index = line.find(delimiter);    
        values.push_back(stod(line.substr(0,index)));
        line.erase(0,index+1);
    }
    if (line.size()>0){
        
        values.push_back(stod(line));
    }
    return values;
}

int main() {
    vector<vector<double>> v;
    for (int i = 0; i<3; ++i) {
        vector<double> v2;
        v.push_back(v2);
    }
    cout << v.size() << endl;
    v[0].push_back(1.);
    cout << v[0][0] << endl;

    return 0;
}