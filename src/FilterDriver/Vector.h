#pragma once
#include <ntifs.h>

// Structure to hold the dynamic array
typedef struct _DYNAMIC_UNICODE_STRING_ARRAY {
    UNICODE_STRING** strings;
    ULONG count;
    ULONG capacity;
} DYNAMIC_UNICODE_STRING_ARRAY, * PDYNAMIC_UNICODE_STRING_ARRAY;

// Function to initialize the dynamic array
NTSTATUS InitializeDynamicArray(PDYNAMIC_UNICODE_STRING_ARRAY dynamicArray);

// Function to add a string to the dynamic array
NTSTATUS AddStringToDynamicArray(PDYNAMIC_UNICODE_STRING_ARRAY dynamicArray, PCWSTR string);

// Function to cleanup the dynamic array
VOID CleanupDynamicArray(PDYNAMIC_UNICODE_STRING_ARRAY dynamicArray);