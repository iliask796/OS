#include "SharedData.h"

SharedData::SharedData() {
    line_no=0;
    content[0] = '\0';
}

void SharedData::setLine(int no) {
    line_no=no;
}

void SharedData::setContent(string string1) {
    strcpy(content,string1.c_str());
    //TODO:CHECK IF NEEDED
    content[string1.length()] = '\0';
}

int SharedData::getLine() {
    return line_no;
}

string SharedData::getContent() {
    return content;
}

SharedData::~SharedData() {

}