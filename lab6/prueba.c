#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 

char *foo(char *); 

int main(void) { 
    char *a = NULL; 
    char *b = NULL; 

    a = foo("Hola a todos, Saludes "); 
    b = foo("Adios"); 

    printf("Desde el principal: %s %s\n", a, b); 

    free(a); 
    free(b);   

    return 0;
} 

char *foo(char *p) { 
    char *q = (char *)malloc(strlen(p) + 1); 
    if (!q) return NULL;
    strcpy(q, p); 
    printf("Desde foo: la cadena es %s\n", q); 
    return q; 
} 