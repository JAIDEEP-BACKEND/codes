#include <iostream>
using namespace std;

// Hostel class declaration
class Hostel {
private:
    int roomNumber;
    string studentName;
    bool isOccupied;

public:
    void setData(int r, string name, bool status); // Function prototype
    void display();                                // Function prototype
};

// Definitions of member functions outside the class
void Hostel::setData(int r, string name, bool status) {
    roomNumber = r;
    studentName = name;
    isOccupied = status;
}

void Hostel::display() {
    cout << "Room Number: " << roomNumber << endl;
    cout << "Student Name: " << studentName << endl;
    cout << "Occupied: " << (isOccupied ? "Yes" : "No") << endl;
}

// Main function
int main() {
    Hostel h1;
    int room;
    string name;
    char choice;

    cout << "Enter Room Number: ";
    cin >> room;
    cin.ignore(); // to ignore leftover newline from cin

    cout << "Enter Student Name: ";
    getline(cin, name);

    cout << "Is the room occupied? (y/n): ";
    cin >> choice;

    bool status = (choice == 'y' || choice == 'Y');

    // Set and display hostel room data
    h1.setData(room, name, status);
    cout << "\nHostel Room Details:\n";
    h1.display();

    return 0;
}
