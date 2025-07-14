#include <iostream>
using namespace std;

class Student {
public:
    void showName() {
        cout << "This is my class." << endl;
    }
};

int main() {
    Student s;
    s.showName();
    return 0;
}
