/*
 * File:   main.c
 * Author: Ben L
 *
 * Created on September 5, 2018, 8:37 PM
 */

#include "main.h"
/*
 *
 */
int calcVals() {
    int8_t a = 0b00100110;
    int8_t b = 0b11011010;
    int8_t c = 0b00000111;
    int8_t d = 0b01000000;

    printf("Initial values: \n");
    printf("a = %d \n", a);
    printf("b = %d \n", b);
    printf("c = %d \n", c);
    printf("d = %d \n", d);

    printf("\nNew Values: \n");
    printf("a - b = %d, \n",sub(a, b, 3, 4, 3, 4));
    printf("a * c = %d, \n",mult(a, c, 3, 4, 6, 1));
    printf("b / d = %d, \n",divis(b, d, 3, 4, 0, 8));

    return (EXIT_SUCCESS);
}

int8_t twosCompliment(int8_t x) {
    x = x^(0b11111111);
    x += (int8_t)1;
    return x;
}

int8_t sub(int16_t a, int16_t b, int qna, int qra, int qnb, int qrb) {
    int8_t ans;
    b = twosCompliment(b);
    ans = (a + b)>>(qra/2);
    return ans;
 }

int16_t mult(int8_t a, int8_t b, int8_t qna, int8_t qra, int8_t qnb, int8_t qrb) {
    int16_t ans;
    int shift;
    ans = (a*b)>>((qra+qrb)-qnb);
    return ans;
}

int16_t divis(int8_t a, int8_t b, int8_t qna, int8_t qra, int8_t qnb, int8_t qrb) {
    int8_t ans;
    ans = (a<<(qra+qna-qnb))/b;
    return ans;
}
