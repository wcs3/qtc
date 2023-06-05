#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define ARR_TYPE uint32_t

typedef struct bst_node_t
{
    ARR_TYPE val;
    bst_node_t *l;
    bst_node_t *r;
} bst_node_t;

void merge(ARR_TYPE *arr, size_t l, size_t m, size_t r, int32_t (*cmp)(ARR_TYPE, ARR_TYPE))
{
    size_t i, j, k;
    size_t Ln = m - l + 1;
    size_t Rn = r - m;

    ARR_TYPE L[Ln], R[Rn];
    memcpy(L, &arr[l], sizeof(L[0]) * Ln);
    memcpy(R, &arr[m + 1], sizeof(R[0]) * Rn);

    i = 0;
    j = 0;
    k = l;

    while (i < Ln && j < Rn)
    {
        if (cmp(L[i], R[j]) >= 0)
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

    while (i < Ln)
    {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < Rn)
    {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void merge_sort(ARR_TYPE *arr, size_t l, size_t r, int32_t (*cmp)(ARR_TYPE, ARR_TYPE))
{
    if (l < r)
    {
        size_t m = l + (r - l) / 2;

        merge_sort(arr, l, m, cmp);
        merge_sort(arr, m + 1, r, cmp);

        merge(arr, l, m, r, cmp);
    }
}

void array_sort(ARR_TYPE *arr, size_t n, int32_t (*cmp)(ARR_TYPE, ARR_TYPE))
{
    merge_sort(arr, 0, n - 1, cmp);
}

void array_reverse(ARR_TYPE *arr, size_t n)
{
    ARR_TYPE *front = arr;
    ARR_TYPE *back = arr[n - 1];
    while (front < back)
    {
        ARR_TYPE tmp = *front;
        *front = *back;
        *back = tmp;
        front++;
        back--;
    }
}

void array_rotate(ARR_TYPE *arr, size_t n, int32_t d)
{
    d = d % n;
    d = (d < 0) ? d + n : d;
    array_reverse(arr, d);
    array_reverse(arr + d, n - d);
    array_reverse(arr, n);
}

size_t inverted_faro_shuffle(ARR_TYPE *arr, size_t first, size_t last)
{
    size_t n = last - first;

    if (n == 1)
    {
        return last;
    }

    size_t m = first + (((n + 1) >> 4) << 1);
}

size_t next_pow_2(size_t n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;

    return n;
}

size_t max_leaves(size_t n)
{
    n = next_pow_2(n) - 1;

    return n / 2;
}

size_t height(size_t n)
{
    size_t h = 0;
    while (n)
    {
        n >>= 1;
        h++;
    }
}

void array_sorted_to_complete_bst(ARR_TYPE *arr, ARR_TYPE *res, size_t level_leaves, size_t n)
{
    size_t p = 0;
    size_t m = max_leaves(n);
    if ((m / 2 - 1) <= (n - m))
    {
        p = m - 1;
    }
    else
    {
        p = n - m / 2;
    }
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
    ARR_TYPE a[] = {0, 1, 2, 3, 4, 5, 6, 7};
    size_t an = sizeof(a) / sizeof(a[0]);

    print_array(a, an);
    printf("\n");

    ARR_TYPE a_bst[an];

    array_sorted_to_bst(a, a_bst, 0, an, 0);
    print_array(a_bst, an);
    printf("\n\n");

    ARR_TYPE b[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    size_t bn = sizeof(b) / sizeof(b[0]);

    print_array(b, bn);
    printf("\n");

    ARR_TYPE b_bst[bn];

    array_sorted_to_bst(b, b_bst, 0, bn, 0);
    print_array(b_bst, bn);
    printf("\n\n");
}