// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include <conio.h>

int main()
{
    HANDLE handle;
    int Tick;
    char buffer[100] = { 0, };
    DWORD nOfBytesRead;
    BOOL bRet;

    handle = CreateFile(L"\\??\\MYSAMPLE", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    Tick = GetTickCount();

    printf("call to ReadFile..\n");
    bRet = ReadFile(handle, buffer, 5, &nOfBytesRead, NULL);
    printf("returned from ReadFile..\n");

    Tick = GetTickCount() - Tick;
    printf("delay time = &d msec, ReadFile\n", Tick);

    CloseHandle(handle);

    _getch();

    return 0;
}
