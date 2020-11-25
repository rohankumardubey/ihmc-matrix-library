#include "NativeMatrix.h"
#include <Eigen/Dense>
#include <iostream>
#include <cmath>
#include <cstring>

NativeMatrixImpl::NativeMatrixImpl(int numRows, int numCols) : storage(numRows, numCols), matrix(NULL, numRows, numCols)
{
    updateView(numRows, numCols);
}

void NativeMatrixImpl::resize(int numRows, int numCols)
{
    if(numRows == rows() && numCols == cols())
    {
        return;
    }

    if(numRows * numCols > storage.size())
    {
        storage.resize(numRows, numCols);
    }

    updateView(numRows, numCols);
}

bool NativeMatrixImpl::set(NativeMatrixImpl *a)
{
    resize(a->rows(), a->cols());

    matrix = a->matrix;

    return true;
}

bool NativeMatrixImpl::add(NativeMatrixImpl *a, NativeMatrixImpl *b)
{
    if(a->rows() != b->rows() || a->cols() != b->cols())
    {
        return false;
    }

    resize(a->rows(), a->cols());

    matrix = (a->matrix) + (b->matrix);

    return true;
}

bool NativeMatrixImpl::subtract(NativeMatrixImpl *a, NativeMatrixImpl *b)
{
    if(a->rows() != b->rows() || a->cols() != b->cols())
    {
        return false;
    }
    resize(a->rows(), a->cols());

    matrix = (a->matrix) - (b->matrix);

    return true;
}


bool NativeMatrixImpl::mult(NativeMatrixImpl *a, NativeMatrixImpl *b)
{
    if(a->cols() != b->rows())
    {
        return false;
    }

    resize(a->rows(), b->cols());

    matrix = (a->matrix) * (b->matrix);

    return true;
}

bool NativeMatrixImpl::mult(double scale, NativeMatrixImpl *a, NativeMatrixImpl *b)
{
    if(a->cols() != b->rows())
    {
        return false;
    }

    resize(a->rows(), b->cols());

    matrix = scale * (a->matrix) * (b->matrix);

    return true;
}

bool NativeMatrixImpl::multAdd(NativeMatrixImpl *a, NativeMatrixImpl *b)
{
    if(a->rows() != rows() || b->cols() != cols() || a->cols() != b->rows())
    {
        return false;
    }

    matrix += (a->matrix) * (b->matrix);

    return true;
}

bool NativeMatrixImpl::multTransA(NativeMatrixImpl *a, NativeMatrixImpl *b)
{
    if( a->rows() != b->rows())
    {
        return false;
    }

    resize(a->cols(), b->cols());

    matrix = (a->matrix.transpose()) * (b->matrix);

    return true;
}

bool NativeMatrixImpl::multAddTransA(NativeMatrixImpl *a, NativeMatrixImpl *b)
{
    if(a->cols() != rows() || b->cols() != cols() || a->rows() != b->rows())
    {
        return false;
    }

    matrix += (a->matrix.transpose()) * (b->matrix);

    return true;
}

bool NativeMatrixImpl::multTransB(NativeMatrixImpl *a, NativeMatrixImpl *b)
{
    if(a->cols() != b->cols())
    {
        return false;
    }

    resize(a->rows(), b->rows());

    matrix = (a->matrix) * (b->matrix.transpose());

    return true;
}

bool NativeMatrixImpl::multAddTransB(NativeMatrixImpl *a, NativeMatrixImpl *b)
{
    if(a->rows() != rows() || b->rows() != cols() || a->cols() != b->cols())
    {
        return false;
    }

    matrix += (a->matrix) * (b->matrix.transpose());

    return true;
}

bool NativeMatrixImpl::addBlock(NativeMatrixImpl *a, int destStartRow, int destStartColumn, int srcStartRow, int srcStartColumn, int numberOfRows, int numberOfColumns, double scale)
{
    if(destStartRow < 0 || destStartColumn < 0 || srcStartRow < 0 || srcStartColumn < 0 || numberOfRows < 0 || numberOfColumns < 0)
    {
        return false;
    }

    if(rows() < destStartRow + numberOfRows)
    {
        return false;
    }

    if(cols() < destStartColumn + numberOfColumns)
    {
        return false;
    }

    if(a->rows() < srcStartRow + numberOfRows)
    {
        return false;
    }

    if(a->cols() < srcStartColumn + numberOfColumns)
    {
        return false;
    }

    matrix.block(destStartRow, destStartColumn, numberOfRows, numberOfColumns) += scale * a->matrix.block(srcStartRow, srcStartColumn, numberOfRows, numberOfColumns);
    return true;
}

bool NativeMatrixImpl::multAddBlock(NativeMatrixImpl *a, NativeMatrixImpl *b, int rowStart, int colStart)
{
    if(rowStart < 0 || colStart < 0)
    {
        return false;
    }

    if(a->cols() != b->rows())
    {
        return false;
    }

    if( (rows() - rowStart) < a->rows())
    {
        return false;
    }

    if((cols() - colStart) < b->cols())
    {
        return false;
    }

    matrix.block(rowStart, colStart, a->rows(), b->cols()) += a->matrix * b->matrix;

    return true;

}

bool NativeMatrixImpl::multQuad(NativeMatrixImpl *a, NativeMatrixImpl *b)
{
    if(a->rows() != b->cols() || b->cols() != b->rows())
    {
        return false;
    }

    resize(a->cols(), a->cols());

    matrix = (a->matrix).transpose() * (b->matrix) * (a->matrix);

    return true;
}

bool NativeMatrixImpl::invert(NativeMatrixImpl *a)
{
    if(a->rows() != a->cols())
    {
        return false;
    }

    resize(a->rows(), a->cols());

    matrix = (a->matrix).lu().inverse();

    return true;
}

bool NativeMatrixImpl::solve(NativeMatrixImpl *a, NativeMatrixImpl *b)
{

    if(a->rows() != b->rows() || b->cols() != 1 || a->cols() != a->rows())
    {
        return false;
    }

    resize(a->cols(), 1);

    matrix = (a->matrix).lu().solve((b->matrix));

    return true;

}

bool NativeMatrixImpl::solveCheck(NativeMatrixImpl *a, NativeMatrixImpl *b)
{
    if(a->rows() != b->rows() || b->cols() != 1 || a->cols() != a->rows())
    {
        std::cerr << "NativeMatrix::solveCheck: Invalid dimensions" << std::endl;
        return false;
    }

    resize(a->cols(), 1);

    const Eigen::FullPivLU<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> > fullPivLu = a->matrix.fullPivLu();
    if (fullPivLu.isInvertible())
    {
        matrix = fullPivLu.solve(b->matrix);
        return true;
    }
    else
    {
        matrix.setConstant(nan);
        return false;
    }

}

bool NativeMatrixImpl::insert(NativeMatrixImpl *src, int srcY0, int srcY1, int srcX0, int srcX1, int dstY0, int dstX0)
{
    if(srcY0 < 0 || srcY1 < 0 || srcX0 < 0 || srcX1 < 0 || dstY0 < 0 || dstX0 < 0)
    {
        return false;
    }


    if( srcY1 < srcY0 || srcY0 < 0 || srcY1 > src->rows() )
    {
        return false;
    }
    if( srcX1 < srcX0 || srcX0 < 0 || srcX1 > src->cols() )
    {
        return false;
    }

    int w = srcX1-srcX0;
    int h = srcY1-srcY0;

    if( dstY0+h > rows() )
    {
        return false;
    }
    if( dstX0+w > cols() )
    {
        return false;
    }


    matrix.block(dstY0, dstX0, h, w) = src->matrix.block(srcY0, srcX0, h, w);

    return true;
}

bool NativeMatrixImpl::insert(double *src, int srcRows, int srcCols, int srcY0, int srcY1, int srcX0, int srcX1, int dstY0, int dstX0)
{
    if(src == nullptr)
    {
        return false;
    }

    if(srcY0 < 0 || srcY1 < 0 || srcX0 < 0 || srcX1 < 0 || dstY0 < 0 || dstX0 < 0)
    {
        return false;
    }

    if( srcY1 < srcY0 || srcY0 < 0 || srcY1 > srcRows )
    {
        return false;
    }
    if( srcX1 < srcX0 || srcX0 < 0 || srcX1 > srcCols )
    {
        return false;
    }

    int w = srcX1-srcX0;
    int h = srcY1-srcY0;

    if( dstY0+h > rows() )
    {
        return false;
    }
    if( dstX0+w > cols() )
    {
        return false;
    }

    Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> eigenData(src, srcRows, srcCols);
    matrix.block(dstY0, dstX0, h, w) = eigenData.block(srcY0, srcX0, h, w);

    return true;

}

bool NativeMatrixImpl::extract(int srcY0, int srcY1, int srcX0, int srcX1, double *dst, int dstRows, int dstCols, int dstY0, int dstX0)
{
    if(dst == nullptr)
    {
        return false;
    }

    if(srcY0 < 0 || srcY1 < 0 || srcX0 < 0 || srcX1 < 0 || dstY0 < 0 || dstX0 < 0)
    {
        return false;
    }

    if( srcY1 < srcY0 || srcY0 < 0 || srcY1 > rows() )
    {
        return false;
    }
    if( srcX1 < srcX0 || srcX0 < 0 || srcX1 > cols() )
    {
        return false;
    }

    int w = srcX1-srcX0;
    int h = srcY1-srcY0;

    if( dstY0+h > dstRows )
    {
        return false;
    }
    if( dstX0+w > dstCols )
    {
        return false;
    }

    Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> eigenData(dst, dstRows, dstCols);
    eigenData.block(dstY0, dstX0, h, w) = matrix.block(srcY0, srcX0, h, w);

    return true;

}


bool NativeMatrixImpl::transpose(NativeMatrixImpl *a)
{
    resize(a->cols(), a->rows());

    matrix = a->matrix.transpose();

    return true;
}

bool NativeMatrixImpl::removeRow(int rowToRemove)
{

    if(rowToRemove >= rows() || rowToRemove < 0)
    {
        return false;
    }

    if(rows() <= 1)
    {
        updateView(0, cols());
        return true;
    }

    int oldRows = rows();
    int newRows = oldRows - 1;
    int newCols = cols();

    /*
     * Algorithm based on memmove
     *
     * Very fast compared to eigen directly.
     */

    double* data = storage.data();

    size_t newStride = (size_t)newRows * sizeof(double);


    for (int col = 0; col < newCols - 1; col++)
    {
        double* dst = data + (col * newRows + rowToRemove);
        double* src = data + (col * oldRows + rowToRemove + 1);

        memmove((void*)(dst), (void*)(src), newStride);
    }

    int lastCol = cols() - 1;
    int remaining = newRows - rowToRemove;
    double* dst = data + (lastCol * newRows + rowToRemove);
    double* src = data + (lastCol * oldRows + rowToRemove + 1);
    memmove((void*)(dst), (void*)(src), remaining * sizeof(double));

    updateView(newRows, newCols);
    return true;


}

bool NativeMatrixImpl::removeColumn(int colToRemove)
{
    if(colToRemove >= cols() || colToRemove < 0)
    {
        return false;
    }

    if(cols() <= 1)
    {
        updateView(rows(), 0);
        return true;
    }


    int newRows = rows();
    int oldCols = cols();
    int newCols = oldCols - 1;

    double* data = storage.data();
    double* dst = data + (colToRemove * newRows);
    double* src = data + ( (colToRemove + 1) * newRows);
    size_t size = (newCols - colToRemove) * newRows * sizeof(double);

    memmove(dst, src, size);

    updateView(newRows, newCols);
    return true;
}

void NativeMatrixImpl::zero()
{
    matrix.setZero();
}

bool NativeMatrixImpl::containsNaN()
{
    for(int i = 0; i < matrix.size(); i++)
    {
        if(std::isnan(matrix.data()[i]))
        {
            return true;
        }
    }

    return false;
}

bool NativeMatrixImpl::scale(double scale, NativeMatrixImpl *src)
{
    resize(src->rows(), src->cols());

    matrix = scale * src->matrix;

    return true;
}

bool NativeMatrixImpl::isAprrox(NativeMatrixImpl *other, double precision)
{
    return matrix.isApprox(other->matrix, precision);
}

bool NativeMatrixImpl::set(double *data, int rows, int cols)
{
    if(data == nullptr)
    {
        return false;
    }

    resize(rows, cols);


    Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> eigenData(data, rows, cols);
    matrix = eigenData;

    return true;

}

bool NativeMatrixImpl::get(double *data, int rows, int cols)
{
    if(rows != this->rows() || cols != this->cols())
    {
        return false;
    }

    if(data == nullptr)
    {
        return false;
    }

    Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> eigenData(data, rows, cols);
    eigenData = matrix;

    return true;
}

void NativeMatrixImpl::print()
{
    std::cout << matrix << std::endl;
}





