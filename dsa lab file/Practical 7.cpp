#include <iostream>
#include <stack>
#include <string>
#include <sstream>
#include <cmath>
using namespace std;

int main() {
    string postfix;
    cout << "Enter a postfix expression (tokens separated by spaces): ";
    getline(cin, postfix);

    stack<double> st;
    stringstream ss(postfix);
    string token;

    while (ss >> token) {

        if (isdigit(token[0]) || (token.size() > 1 && token[0] == '-' && isdigit(token[1]))) {
            st.push(stod(token));
        }
        else {
            if (st.size() < 2) {
                cerr << "Error: insufficient operands for operator '" << token << "'\n";
                return 1;
            }
            double a = st.top(); st.pop();
            double b = st.top(); st.pop();
            double ans = 0;

            if (token == "+") ans = b + a;
            else if (token == "-") ans = b - a;
            else if (token == "*") ans = b * a;
            else if (token == "/") {
                if (a == 0) {
                    cerr << "Error: division by zero\n";
                    return 1;
                }
                ans = b / a;
            }
            else if (token == "^") ans = pow(b, a);
            else {
                cerr << "Error: unknown operator '" << token << "'\n";
                return 1;
            }

            st.push(ans);
        }
    }

    if (st.size() != 1) {
        cerr << "Error: invalid postfix expression\n";
        return 1;
    }

    cout << "Result: " << st.top() << "\n";
    return 0;
}
