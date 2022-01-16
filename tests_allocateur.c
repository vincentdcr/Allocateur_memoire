#include <stdio.h>
#include "mem.h"
#include "common.h"

void test1() {  // Testing max allocation
    mem_init(get_memory_adr(), get_memory_size());
    void *result;
    size_t estimate = 819200;
	while ((result = mem_alloc(estimate)) == NULL) {
		estimate--;
                if(!estimate) {
                        printf("Allocation failed\n");
                }
	}
	if (mem_alloc(1) != NULL) {
        printf("Error Test1 : Remaining room for allocation after max allocation\n");
    }
}

void test2() {  // Testing free and alloc with mem_fit_first
    mem_init(get_memory_adr(), get_memory_size());
    int tab_alloc[5] = {64, 128, 53, 95, 64};
    void* tab_free[5];
    for (int i=0; i<4; i++){
        tab_free[i] = mem_alloc(tab_alloc[i]);
    }
    for (int i=0; i<4; i++){
        *(int *)tab_free[i] = tab_alloc[i];
        mem_free(tab_free[i]);
    }
}

void test3() {  // Testing mem_fit_best and mem_fit_worst
    mem_init(get_memory_adr(), get_memory_size());

    mem_fit(&mem_fit_best);

    int tab_alloc[5] = {64, 1280, 8562, 178000, 64};
    void* tab_free[5];
    for (int i=0; i<4; i++){
        tab_free[i] = mem_alloc(tab_alloc[i]);
    }
    for (int i=0; i<4; i++){
        *(int *)tab_free[i] = tab_alloc[i];
        mem_free(tab_free[i]);
    }

    mem_fit(&mem_fit_worst);

    for (int i=0; i<4; i++){
        tab_free[i] = mem_alloc(tab_alloc[i]);
    }
    for (int i=0; i<4; i++){
        *(int *)tab_free[i] = tab_alloc[i];
        mem_free(tab_free[i]);
    }
}

int main() {
    printf("===============\nTEST 1\n");
    test1();
    printf("PASSED\n\n");
    printf("===============\nTEST 2\n");
    test2();
    printf("PASSED\n\n");
    printf("===============\nTEST 3\n");
    test3();
    printf("PASSED\n\n");
    printf("All tests successfully passed\n");
    return 0;
}