#include <iostream>
using namespace std;

class Student {
public:
    void showName() {
        cout << "My name is Ali." << endl;
    }
};

int main() {
    Student s;
    s.showName();
    return 0;
}
