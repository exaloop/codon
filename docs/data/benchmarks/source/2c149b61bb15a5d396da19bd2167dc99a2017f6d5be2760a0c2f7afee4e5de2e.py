# Sparse Matrix-Vector Multiplication (SpMV)
import numpy as np


# Matrix-Vector Multiplication with the matrix given in Compressed Sparse Row
# (CSR) format
def spmv(A_row, A_col, A_val, x):
    y = np.empty(A_row.size - 1, A_val.dtype)

    for i in range(A_row.size - 1):
        cols = A_col[A_row[i]:A_row[i + 1]]
        vals = A_val[A_row[i]:A_row[i + 1]]
        y[i] = vals @ x[cols]

    return y
