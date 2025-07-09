#include<iostream>
using namespace std;
int main ()
{ 
	int array[20],position,i,n,m,value;
	cout<<"enter no of elements";
	cin>>n;
	cout<<"enter elements";
	for(i=0;i<n;i++)
	{
		cin>>array[i];
	}
	cout<<"enter the position you want to enter element";
	cin>>position;
	cout<<"enter the value to insert";
	cin>>value;
	for(i=n-1;i>=position-1;i--)
	{
	array[i+1]=array[i];
	}
	array[position-1]=value;
	cout<<"resulting array";
	for(i=0;i<=n;i++)
	{
		cout<<array[i];
	}
	return 0;
}
