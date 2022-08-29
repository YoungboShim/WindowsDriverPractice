// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include <conio.h>

int main()
{
    HANDLE handle = (HANDLE)-1;
    BOOL bRet = FALSE;
    DWORD dwRet;
    unsigned char* pVirtualAddress;

    handle = CreateFile(L"\\??\\MYSAMPLE", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    bRet = ReadFile(handle, &pVirtualAddress, sizeof(unsigned char*), &dwRet, NULL);

    if (bRet == TRUE)
    {
        printf("Mapped Address = 0x%p\n", pVirtualAddress);
        _getch();
    }

    CloseHandle(handle);

    return 0;
}
