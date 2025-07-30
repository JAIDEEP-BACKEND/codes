
#include<iostream>
#include<stack>
using namespace std;
int main(){
    stack<int>s;
    s.push(10);
    s.push(20);
    s.push(30);
    cout<<"Top element is:-"<<s.top();
  s.pop();
    cout<<"Top after popping"<<s.top();
    return 0;
}
