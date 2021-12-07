#include <string>
#include <cstring>
using namespace std;

#define lineSize 100

class SharedData{
private:
    int line_no;
    char content[lineSize+1];
public:
    SharedData();
    void setLine(int);
    void setContent(string);
    int getLine();
    string getContent();
    ~SharedData();
};