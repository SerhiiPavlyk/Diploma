#include "Vector.h"

#define INITIAL_ARRAY_SIZE 25

NTSTATUS InitializeDynamicArray(PDYNAMIC_UNICODE_STRING_ARRAY dynamicArray)
{
    dynamicArray->count = 0;
    dynamicArray->capacity = INITIAL_ARRAY_SIZE;
    dynamicArray->strings = ExAllocatePoolWithTag(NonPagedPool, dynamicArray->capacity * sizeof(UNICODE_STRING*), 'Tag');
    if (!dynamicArray->strings)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(dynamicArray->strings, dynamicArray->capacity * sizeof(UNICODE_STRING*));
    return STATUS_SUCCESS;
}

NTSTATUS AddStringToDynamicArray(PDYNAMIC_UNICODE_STRING_ARRAY dynamicArray, PCWSTR string)
{
    if (dynamicArray->count == dynamicArray->capacity)
    {
        // Need to resize the array
        ULONG newCapacity = dynamicArray->capacity * 2;
        UNICODE_STRING** newStrings = ExAllocatePoolWithTag(NonPagedPool, newCapacity * sizeof(UNICODE_STRING*), 'Tag');
        if (!newStrings)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory(newStrings, dynamicArray->strings, dynamicArray->count * sizeof(UNICODE_STRING*));
        ExFreePool(dynamicArray->strings);
        dynamicArray->strings = newStrings;
        dynamicArray->capacity = newCapacity;
    }

    // Allocate memory for the new string
    dynamicArray->strings[dynamicArray->count] = ExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING), 'Tag');
    if (!dynamicArray->strings[dynamicArray->count])
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize the string
    RtlInitUnicodeString(dynamicArray->strings[dynamicArray->count], string);
    dynamicArray->count++;
    return STATUS_SUCCESS;
}

VOID CleanupDynamicArray(PDYNAMIC_UNICODE_STRING_ARRAY dynamicArray)
{
    for (ULONG i = 0; i < dynamicArray->count; ++i)
    {
        ExFreePool(dynamicArray->strings[i]);
    }
    ExFreePool(dynamicArray->strings);
    RtlZeroMemory(dynamicArray, sizeof(DYNAMIC_UNICODE_STRING_ARRAY));
}