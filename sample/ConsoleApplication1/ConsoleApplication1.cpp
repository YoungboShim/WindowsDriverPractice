// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <stdio.h>

int main()
{
    HANDLE handle;
    BOOL bRet;
    DWORD dwRet;
    UCHAR buffer[100] = { 0, };

    handle = CreateFile(L"\\??\\MYSAMPLE", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    bRet = ReadFile(handle, buffer, 5, &dwRet, NULL);
    if (bRet) printf("Read: dwRet =%d\n", dwRet);
    else printf("Read: FAIL");

    bRet = WriteFile(handle, "HELLO", 5, &dwRet, NULL);
    if (bRet) printf("Write: dwRet =%d\n", dwRet);
    else printf("Read: FAIL");

    bRet = ReadFile(handle, buffer, 100, &dwRet, NULL);
    if (bRet) printf("Read: dwRet =%d\n", dwRet);
    else printf("Read: FAIL");

    CloseHandle(handle);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
