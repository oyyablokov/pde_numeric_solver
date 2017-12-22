/*
    Copyright (c) 2017 Oleg Yablokov

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

**/

#include "math_module.h"
#include <memory>
#include <functional>
#include <cmath>

/**
 * // n is the number of unknowns
 *
 * |b0 c0 0 ||x0| |d0|
 * |a1 b1 c1||x1|=|d1|
 * |0  a2 b2||x2| |d2|
 *
 * 1st iteration: b0x0 + c0x1 = d0 -> x0 + (c0/b0)x1 = d0/b0 ->
 *
 * x0 + g0x1 = r0               where g0 = c0/b0        , r0 = d0/b0
 *
 * 2nd iteration:     | a1x0 + b1x1   + c1x2 = d1
 * from 1st it.: -| a1x0 + a1g0x1        = a1r0
 * -----------------------------
 * (b1 - a1g0)x1 + c1x2 = d1 - a1r0
 *
 * x1 + g1x2 = r1               where g1=c1/(b1 - a1g0) , r1 = (d1 - a1r0)/(b1 - a1g0)
 *
 * 3rd iteration:      | a2x1 + b2x2   = d2
 * from 2nd it. : -| a2x1 + a2g1x2 = a2r2
 * -----------------------
 * (b2 - a2g1)x2 = d2 - a2r2
 * x2 = r2                      where                     r2 = (d2 - a2r2)/(b2 - a2g1)
 * Finally we have a triangular matrix:
 * |1  g0 0 ||x0| |r0|
 * |0  1  g1||x1|=|r1|
 * |0  0  1 ||x2| |r2|
 *
 * Condition: ||bi|| > ||ai|| + ||ci||
 *
 * in this version the c matrix reused instead of g
 * and             the d matrix reused instead of r and x matrices to report results
 * Written by Keivan Moradi, 2014
 */
void MathModule::solve_tridiagonal_equation(std::vector<float>& a, std::vector<float>& b, std::vector<float>& c, std::vector<float>& d, int n) {
    n--; // since we start from x0 (not x1)
    c[0] /= b[0];
    d[0] /= b[0];

    for (int i = 1; i < n; i++) {
        c[i] /= b[i] - a[i] * c[i - 1];
        d[i] = (d[i] - a[i] * d[i - 1]) / (b[i] - a[i] * c[i - 1]);
    }

    d[n] = (d[n] - a[n] * d[n - 1]) / (b[n] - a[n] * c[n - 1]);

    for (int i = n; i-- > 0;) {
        d[i] -= c[i] * d[i + 1];
    }
}
