#include "SharedData.h"

SharedData::SharedData() {
    line_no=0;
    content.clear();
}

void SharedData::setLine(int no) {
    line_no=no;
}

void SharedData::setContent(string string1) {
    content.assign(string1);
}

int SharedData::getLine() {
    return line_no;
}

string SharedData::getContent() {
    return content;
}

SharedData::~SharedData() {

}