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

#ifndef CAMSYS_COLORTABLE_H
#define CAMSYS_COLORTABLE_H

#include <QtGui/QRgb>
#include <QtCore/QVector>

namespace CamSys {

//! \addtogroup CamSysGui
//! @{

class ColorTable : public QVector<QRgb>
{
public:
    enum { TableSize = 256 };

    ColorTable();
    ColorTable(const QVector<QRgb> &vec);
    ColorTable(float rf, float gf, float bf);
    ColorTable(const ColorTable &other);
    ColorTable & operator=(const ColorTable &other);

    static ColorTable grayTable();
    static ColorTable redTable();
    static ColorTable greenTable();
    static ColorTable blueTable();
    static ColorTable hotTable();
    static ColorTable alienTable();
    static ColorTable coolTable();
    static ColorTable rgbTable();
    static ColorTable rainbowTable();
    static ColorTable flameTable();
};

//! @} // end of group CamSysGui

} // namespace CamSys

#endif // CAMSYS_COLORTABLE_H
