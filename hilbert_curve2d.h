#ifndef HILBERT_CURVE2D_H
#define HILBERT_CURVE2D_H
#include <stdint.h>

void d2xy(int n, int d, int &x, int &y);
int i4_power(int i, int j);
void rot(int n, int &x, int &y, int rx, int ry);
int xy2d(int n, int x, int y);
#endif