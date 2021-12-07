#include <string>
using namespace std;

class SharedData{
private:
    int line_no;
    string content;
public:
    SharedData();
    void setLine(int);
    void setContent(string);
    int getLine();
    string getContent();
    ~SharedData();
};