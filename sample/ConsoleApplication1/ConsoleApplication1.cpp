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
    OVERLAPPED ov1 = { 0, };
    OVERLAPPED ov2 = { 0, };
    OVERLAPPED ov3 = { 0, };
    UCHAR buffer[100] = { 0, };

    handle = CreateFile(L"\\??\\MYSAMPLE", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    ov1.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (ov1.hEvent == NULL)
        goto exit;

    ov2.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    ov3.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    printf("call to ReadFile1..\n");
    bRet = ReadFile(handle, buffer, 5, &dwRet, &ov1);
    printf("returned from ReadFile1..\n");

    printf("call to ReadFile2..\n");
    bRet = ReadFile(handle, buffer, 5, &dwRet, &ov2);
    printf("returned from ReadFile2..\n");

    printf("call to ReadFile3..\n");
    bRet = ReadFile(handle, buffer, 5, &dwRet, &ov3);
    printf("returned from ReadFile3..\n");

    dwRet = WaitForSingleObject(ov1.hEvent, INFINITE);
    printf("Signaled ov1.hEvent\n");
    dwRet = WaitForSingleObject(ov2.hEvent, INFINITE);
    printf("Signaled ov2.hEvent\n");
    dwRet = WaitForSingleObject(ov3.hEvent, INFINITE);
    printf("Signaled ov3.hEvent\n");

    _getch();

exit:
    if (ov1.hEvent)
        CloseHandle(ov1.hEvent);
    if (ov2.hEvent)
        CloseHandle(ov2.hEvent);
    if (ov3.hEvent)
        CloseHandle(ov3.hEvent);

    if (handle != (HANDLE)-1)
        CloseHandle(handle);

    return 0;
}
