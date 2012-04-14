/*
 * Copyright (c) 2009, 2010 Kolja Glogowski
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

#ifndef CAMSYS_COLOR_RANGE_H
#define CAMSYS_COLOR_RANGE_H

namespace CamSys {

//! \addtogroup CamSysGui
//! @{

class ColorRange
{
public:
    ColorRange();
    ColorRange(double minValue, double maxValue);
    ColorRange(double minValue, double maxValue,
               double minColorValue, double maxColorValue);

    double minValue() const;
    void setMinValue(double minValue);

    double maxValue() const;
    void setMaxValue(double maxValue);

    double minColorValue() const;
    void setMinColorValue(double minColorValue);

    double maxColorValue() const;
    void setMaxColorValue(double maxColorValue);

    void setRange(double minValue, double maxValue);
    void setRange(double minValue, double maxValue,
                  double minColorValue, double maxColorValue);
    void setColorSpread(double minColorValue, double maxColorValue);

    double minColorRatio() const;
    void setMinColorRatio(double minColorRatio);

    double maxColorRatio() const;
    void setMaxColorRatio(double maxColorRatio);

    void setColorSpreadRatio(double minColorRatio, double maxColorRatio);

    double width() const;
    double colorSpreadWidth() const;

    bool operator==(const ColorRange &other) const;
    bool operator!=(const ColorRange &other) const;

private:
    double m_minValue;
    double m_maxValue;
    double m_minColorValue;
    double m_maxColorValue;
};

//! @} // end of group CamSysGui

inline double ColorRange::minValue() const { return m_minValue; }
inline double ColorRange::maxValue() const { return m_maxValue; }
inline double ColorRange::minColorValue() const { return m_minColorValue; }
inline double ColorRange::maxColorValue() const { return m_maxColorValue; }
inline double ColorRange::width() const { return m_maxValue - m_minValue; }
inline double ColorRange::colorSpreadWidth() const {
    return m_maxColorValue - m_minColorValue;
}
inline bool ColorRange::operator==(const ColorRange &other) const {
    return (m_minValue == other.m_minValue) &&
           (m_maxValue == other.m_maxValue) &&
           (m_minColorValue == other.m_minColorValue) &&
           (m_maxColorValue == other.m_maxColorValue);
}
inline bool ColorRange::operator!=(const ColorRange &other) const
{
    return (m_minValue != other.m_minValue) ||
           (m_maxValue != other.m_maxValue) ||
           (m_minColorValue != other.m_minColorValue) ||
           (m_maxColorValue != other.m_maxColorValue);
}

} // namespace CamSys

#endif // CAMSYS_COLOR_RANGE_H
