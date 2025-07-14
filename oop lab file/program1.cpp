#include <iostream>
using namespace std;

class university_orientation
{
private:
    char student_name[20];
    int student_id;
    char branch[10];

public:
    void insert_data()
    {
        cout << "enter student name:-";
        cin >> student_name;

        cout << "enter student id:-";
        cin >> student_id;

        cout << "enter branch:-";
        cin >> branch;
    }

    void display()
    {
        cout << "Student Name: " << student_name << endl;
        cout << "Student ID: " << student_id << endl;
        cout << "Branch: " << branch << endl;
    }
};

int main()
{
    university_orientation u1;
    u1.insert_data();
    u1.display();
    return 0;
}
