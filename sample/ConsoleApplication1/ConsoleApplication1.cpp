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
        printf("Phys 0xF000 (64KB) Mapped Address = 0x%p\n", pVirtualAddress);

        // Access to mapped VA
        int x, y;
        for (y = 0; y < 0x10000 / 16; y++)
        {
            for (x = 0; x < 16; x++)
            {
                printf("%02X ", *(pVirtualAddress + (y * 16 + x)));
            }
            printf("\n");
        }
        printf("\n");

        _getch();
    }

    CloseHandle(handle);

    return 0;
}
