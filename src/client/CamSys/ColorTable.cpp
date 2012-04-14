/*
 * Copyright (c) 2008 Kolja Glogowski
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ColorTable.h"
#include <QtCore/QFile>

#include <iostream>
using std::cout;
using std::endl;

namespace CamSys {

typedef QVector<QRgb> RgbVector;

/*! \class ColorTable
    \brief Color table handling.
 */

/*! \var ColorTable::TableSize
    \brief Total number of colors.
 */

/*! \brief Default constructor.

    Creates a color table using a gray gradient.
 */
ColorTable::ColorTable()
    : RgbVector(TableSize)
{
    for (int i = 0; i < TableSize; ++i)
        (*this)[i] = qRgb(i, i, i);
}

/*! \brief Contructs the color table from a qRgb vector.

    If the vector contains more than 256 entries, only the first 256
    entries will be copied. If the vector contains less than 256 entries
    the remaining entries will be filled with qRgb(0,0,0) values.
 */
ColorTable::ColorTable(const QVector<QRgb> &vec)
    : RgbVector(TableSize)
{
    int count = qMin(int(TableSize), vec.size());

    for (int i = 0; i < count; ++i)
        (*this)[i] = vec[i];

    for (int i = count; i < TableSize; ++i)
        (*this)[i] = qRgb(0, 0, 0);
}

/*! \brief Contructs the color table using a linear gradient. */
ColorTable::ColorTable(float rf, float gf, float bf)
    : RgbVector(TableSize)
{
    for (int i = 0; i < TableSize; ++i)
        (*this)[i] = qRgb(
            static_cast<unsigned char>(rf*i),
            static_cast<unsigned char>(gf*i),
            static_cast<unsigned char>(bf*i));
}

/*! \brief Copy contructor. */
ColorTable::ColorTable(const ColorTable &other)
    : RgbVector(other)
{
}

/*! \brief Assignment operator. */
ColorTable & ColorTable::operator=(const ColorTable &other)
{
    if (this != &other) {
        resize(other.size());
        qCopy(other.constBegin(), other.constEnd(), begin());
    }
    return *this;
}

/*! \brief Create a color table with a black -> white gradient.
 */
ColorTable ColorTable::grayTable()
{
    return ColorTable();
}

/*! \brief Create a color table with a black -> red gradient.
 */
ColorTable ColorTable::redTable()
{
    return ColorTable(1, 0, 0);
}

/*! \brief Create a color table with a black -> green gradient.
 */
ColorTable ColorTable::greenTable()
{
    return ColorTable(0, 1, 0);
}

/*! \brief Create a color table with a black -> blue gradient.
 */
ColorTable ColorTable::blueTable()
{
    return ColorTable(0, 0, 1);
}

/*! \brief Create a color table with a black -> red -> yellow -> white
        gradient.
 */
ColorTable ColorTable::hotTable()
{
    ColorTable ct;
    int r = 0, g = 0, b = 0;

    for (int i = 0; i < 64; ++i) {
        ct[i] = qRgb(r, g, b);
        r += 2;
    }

    for (int i = 64; i < 128; ++i) {
        ct[i] = qRgb(r, g, b);
        g += 1;
        r += 2;
    }
    r = 255;

    for (int i = 128; i < 192; ++i) {
        ct[i] = qRgb(r, g, b);
        g += 2;
    }

    for (int i = 192; i < 256; ++i) {
        ct[i] = qRgb(r, g, b);
        g += 1;
        b += 4;
    }

    return ct;
}

/*! \brief Create a color table with a black -> green -> yellow -> white
        gradient.
 */
ColorTable ColorTable::alienTable()
{
    ColorTable ct;
    int r = 0, g = 0, b = 0;

    for (int i = 0; i < 64; ++i) {
        ct[i] = qRgb(r, g, b);
        g += 2;
    }

    for (int i = 64; i < 128; ++i) {
        ct[i] = qRgb(r, g, b);
        r += 1;
        g += 2;
    }
    g = 255;

    for (int i = 128; i < 192; ++i) {
        ct[i] = qRgb(r, g, b);
        r += 2;
    }

    for (int i = 192; i < 256; ++i) {
        ct[i] = qRgb(r, g, b);
        r += 1;
        b += 4;
    }

    return ct;
}

/*! \brief Create a color table with a black -> blue -> cyan -> white
        gradient.
 */
ColorTable ColorTable::coolTable()
{
    ColorTable ct;
    int r = 0, g = 0, b = 0;

    for (int i = 0; i < 64; ++i) {
        ct[i] = qRgb(r, g, b);
        b += 2;
    }

    for (int i = 64; i < 128; ++i) {
        ct[i] = qRgb(r, g, b);
        g += 1;
        b += 2;
    }
    b = 255;

    for (int i = 128; i < 192; ++i) {
        ct[i] = qRgb(r, g, b);
        g += 2;
    }

    for (int i = 192; i < 256; ++i) {
        ct[i] = qRgb(r, g, b);
        g += 1;
        r += 4;
    }

    return ct;
}

/*! \brief Create a color table with a blue -> green -> red gradient.
 */
ColorTable ColorTable::rgbTable()
{
    ColorTable ct;
    int r = 0, g = 0, b = 255;

    for (int i = 0; i < 64; ++i)
    {
        ct[i] = qRgb(r, g, b);
        g += 4;
    }
    g = 255; 

    for (int i = 64; i < 128; ++i)
    {
        ct[i] = qRgb(r, g, b);
        b -= 4;
    }
    b = 0;

    for (int i = 128; i < 192; ++i)
    {
        ct[i] = qRgb(r, g, b);
        r += 4;
    }
    r = 255;

    for (int i = 192; i < 256; ++i)
    {
        ct[i] = qRgb(r, g, b);
        g -= 4;
    }

    return ct;
}

/*! \brief Create a color table with a magenta -> blue -> cyan -> green ->
        yellow -> red gradient.
 */
ColorTable ColorTable::rainbowTable()
{
    ColorTable ct;
    int r = 255, g = 0, b = 255;

    for (int i = 0; i < 51; ++i)
    {
        ct[i] = qRgb(r, g, b);
        r -= 5;
    }
    r = 0;

    for (int i = 51; i < 102; ++i)
    {
        ct[i] = qRgb(r, g, b);
        g += 5;
    }
    g = 255;

    for (int i = 102; i < 153; ++i)
    {
        ct[i] = qRgb(r, g, b);
        b -= 5;
    }
    b = 0;

    for (int i = 153; i < 204; ++i)
    {
        ct[i] = qRgb(r, g, b);
        r += 5;
    }
    r = 255;

    for (int i = 204; i < 256; ++i)
    {
        ct[i] = qRgb(r, g, b);
        g -= 5;
    }

    return ct;
}

/*! \brief Create a color table with a black -> blue -> red -> yellow ->
        white gradient.
 */
ColorTable ColorTable::flameTable()
{
    ColorTable ct;
    int r = 0, g = 0, b = 0;

    for (int i = 0; i < 32; ++i)
    {
        ct[i] = qRgb(r, g, b);
        b += 4;
    }

    for (int i = 32; i < 64; ++i)
    {
        ct[i] = qRgb(r, g, b);
        b += 4;
        r += 2;
    }
    b = 255;

    for (int i = 64; i < 128; ++i)
    {
        ct[i] = qRgb(r, g, b);
        r += 3;
        b -= 4;
    }
    r = 255;
    b = 0;

    for (int i = 128; i < 192; ++i)
    {
        ct[i] = qRgb(r, g, b);
        g += 4;
    }
    g = 255;
    b = 0;

    for (int i = 192; i < 256; ++i)
    {
        ct[i] = qRgb(r, g, b);
        b += 4;
    }
    b = 255;

    return ct;
}

} // namespace CamSys

