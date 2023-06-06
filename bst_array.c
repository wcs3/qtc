#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define ARR_TYPE uint32_t

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void merge(ARR_TYPE *arr, size_t l, size_t m, size_t r)
{
    size_t i, j, k;
    size_t n1 = m - l + 1;
    size_t n2 = r - m;

    ARR_TYPE L[n1], R[n2];

    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    i = 0;
    j = 0;

    k = l;
    while (i < n1 && j < n2)
    {
        if (L[i] <= R[j])
        {
            arr[k] = L[i];
            i++;
        }
        else
        {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1)
    {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2)
    {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void merge_sort(ARR_TYPE *arr, size_t l, size_t r)
{
    if (l < r)
    {
        size_t m = l + (r - l) / 2;

        merge_sort(arr, l, m);
        merge_sort(arr, m + 1, r);

        merge(arr, l, m, r);
    }
}

void array_sort(ARR_TYPE *arr, size_t n)
{
    merge_sort(arr, 0, n - 1);
}

void array_reverse(ARR_TYPE *arr, size_t n)
{
    ARR_TYPE *front = arr;
    ARR_TYPE *back = &arr[n - 1];
    while (front < back)
    {
        ARR_TYPE tmp = *front;
        *front = *back;
        *back = tmp;
        front++;
        back--;
    }
}

void reverse(void *arr, const size_t elem_size, const size_t elem_cnt)
{
    uint8_t *front = arr;
    uint8_t *back = arr + (elem_cnt - 1) * elem_size;

    while (front < back)
    {
        uint8_t *next_front = front + elem_size;
        while (front < next_front)
        {
            uint8_t tmp = *front;
            *front = *back;
            *back = tmp;

            front++;
            back++;
        }

        back -= elem_size * 2;
    }
}

void rotate(void *arr, const size_t elem_size, const size_t elem_cnt, int32_t d)
{
    d = d % elem_cnt;
    d = (d < 0) ? d + elem_cnt : d;

    reverse(arr, elem_size, d);
    reverse(arr + d * elem_size, elem_size, elem_cnt - d);
    reverse(arr, elem_size, elem_cnt);
}

// Perform faro shuffle inverse on an array. O(n) time, O(n) space
void array_invert_faro_shuffle(ARR_TYPE *arr, size_t n)
{
    ARR_TYPE tmp[n];

    size_t m = ((n + 1) / 4) * 2;
    size_t i = 0;

    // move even indices to front half
    size_t k = 0;
    for (; k < n; ++i)
    {
        tmp[i] = arr[k];
        k += 2;
    }

    // move odd indices to back half
    k = 1;
    for (; k < n; ++i)
    {
        tmp[i] = arr[k];
        k += 2;
    }

    // copy result back to arr
    for (i = 0; i < n; ++i)
    {
        arr[i] = tmp[i];
    }
}

void inverse_faro_shuffle(void *arr, const size_t elem_size, const size_t elem_cnt)
{
    size_t arr_size = elem_size * elem_cnt;
    uint8_t tmp[arr_size];

    size_t i = 0;

    size_t j = 0;
    while (j < arr_size)
    {
        size_t k = i + elem_size;
        while (i < k)
        {
            tmp[i] = ((uint8_t *)arr)[j];
            ++i;
            ++j;
        }
        j += elem_size;
    }

    j = elem_size;
    while (j < arr_size)
    {
        size_t k = i + elem_size;
        while (i < k)
        {
            tmp[i] = ((uint8_t *)arr)[j];
            ++i;
            ++j;
        }
        j += elem_size;
    }

    // copy result back to arr
    for (i = 0; i < arr_size; ++i)
    {
        ((uint8_t *)arr)[i] = tmp[i];
    }
}

void array_to_complete_btree(void *arr, const size_t elem_size, const size_t elem_cnt)
{
    uint8_t h = 1;
    while ((2 << h) - 1 < elem_cnt)
    {
        h++;
    }

    for (uint8_t lvl = h; lvl > 0; lvl--)
    {
        size_t es = MIN((2 << lvl) - 1, elem_cnt);
        size_t lc = MIN(1 << lvl, elem_cnt - ((1 << lvl) - 1));

        inverse_faro_shuffle(arr, elem_size, 2 * lc - 1);
        rotate(arr, elem_size, es, lc);
    }
}

void array_sorted_to_complete_bst(ARR_TYPE *arr, size_t n)
{
    uint8_t h = 1;
    while ((2 << h) - 1 < n)
    {
        h++;
    }

    for (uint8_t lvl = h; lvl > 0; lvl--)
    {
        size_t es = MIN((2 << lvl) - 1, n);
        size_t lc = MIN(1 << lvl, n - ((1 << lvl) - 1));

        inverse_faro_shuffle(arr, sizeof(ARR_TYPE), 2 * lc - 1);
        rotate(arr, sizeof(ARR_TYPE), es, lc);
    }
}

void array_unsorted_to_complete_bst(ARR_TYPE *arr, size_t n)
{
    array_sort(arr, n);
    array_sorted_to_complete_bst(arr, n);
}

bool array_bst_search(ARR_TYPE *arr, size_t n, ARR_TYPE key)
{
    size_t i = 0;
    while (i < n)
    {
        if (key < arr[i])
        {
            i = 2 * i + 1;
        }
        else if (key > arr[i])
        {
            i = 2 * i + 2;
        }
        else
        {
            return true;
        }
    }

    return false;
}

void print_array(ARR_TYPE *arr, size_t n)
{
    printf("[ ");
    for (size_t i = 0; i < n; i++)
    {
        printf("%u ", arr[i]);
    }
    printf("]");
}

int main()
{
    ARR_TYPE a[] = {0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11};
    size_t an = sizeof(a) / sizeof(a[0]);

    print_array(a, an);
    printf("\n");

    array_sorted_to_complete_bst(a, an);
    print_array(a, an);
    printf("\n\n");

    ARR_TYPE b[] = {10, 4, 11, 1, 0, 2, 5, 9, 3, 8, 7};
    size_t bn = sizeof(b) / sizeof(b[0]);

    print_array(b, bn);
    printf("\n");

    array_unsorted_to_complete_bst(b, bn);
    print_array(b, bn);
    printf("\n\n");

    printf("%s\n", array_bst_search(b, bn, 2) ? "FOUND" : "NOT FOUND");
    printf("%s\n", array_bst_search(b, bn, 6) ? "FOUND" : "NOT FOUND");
}