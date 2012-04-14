/*
 * Copyright (c) 2009 Kolja Glogowski
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

#include <QtDebug>
#include "ColorRange.h"

#define ASSERT_VALID_RANGE() \
    Q_ASSERT(m_minValue <= m_minColorValue); \
    Q_ASSERT(m_minColorValue <= m_maxColorValue); \
    Q_ASSERT(m_maxColorValue <= m_maxValue);

namespace CamSys {

/*! \class ColorRange
    \brief A class for handling color ranges.

    This class can be used to handle color ranges. It assures that
    \code
    minValue <= minColorValue <= maxColorValue <= maxValue
    \endcode
    is always true.

    When setting \a minValue and \a maxValue, the values of \a minColorValue
    and \a maxColorValue may be changed by the setter methods to fullfill the
    conditions stated above. On the other hand, by setting \a minColorValue
    or \a maxColorValue, the \a minValue and \a maxValue will never change
    automatically.

    In case \a minColorValue equals \a minValue, both values will change and
    stay equal, when \a minValue is being changed. The same applies to
    \a maxColorValue and \a maxValue.
 */

/*! \brief Default constructor.
 */
ColorRange::ColorRange()
    : m_minValue(0),
      m_maxValue(0),
      m_minColorValue(0),
      m_maxColorValue(0)
{
}

/*! \brief Init constructor.

    This constructor sets minColorValue and maxColorValue to the same values
    as specified minValue and maxValue respectively.
 */
ColorRange::ColorRange(double minValue, double maxValue)
    : m_minValue(minValue),
      m_maxValue(maxValue),
      m_minColorValue(minValue),
      m_maxColorValue(maxValue)
{
    Q_ASSERT(minValue <= maxValue);
    if (minValue > maxValue) {
        m_minValue = m_maxValue = m_minColorValue = m_maxColorValue = 0;
        return;
    }
}

/*! \brief Init constructor.
 */
ColorRange::ColorRange(double minValue, double maxValue,
                       double minColorValue, double maxColorValue)
    : m_minValue(minValue),
      m_maxValue(maxValue)
{
    Q_ASSERT(minValue <= maxValue);
    if (minValue > maxValue) {
        m_minValue = m_maxValue = m_minColorValue = m_maxColorValue = 0;
        return;
    }

    setColorSpread(minColorValue, maxColorValue);
}

void ColorRange::setMinValue(double minValue)
{
    if (minValue > m_maxValue) {
        m_minValue = m_maxValue = minValue;
        m_minColorValue = m_maxColorValue = minValue;
        return;
    }

    if (m_minColorValue == m_minValue || m_minColorValue < minValue)
        m_minColorValue = minValue;

    if (m_maxColorValue < minValue)
        m_maxColorValue = minValue;

    m_minValue = minValue;

    ASSERT_VALID_RANGE();
}

void ColorRange::setMaxValue(double maxValue)
{
    if (maxValue < m_minValue) {
        m_minValue = m_maxValue = maxValue;
        m_minColorValue = m_maxColorValue = maxValue;
        return;
    }

    if (m_maxColorValue == m_maxValue || m_maxColorValue > maxValue)
        m_maxColorValue = maxValue;

    if (m_minColorValue > maxValue)
        m_minColorValue = maxValue;

    m_maxValue = maxValue;

    ASSERT_VALID_RANGE();
}

void ColorRange::setMinColorValue(double minColorValue)
{
    if (minColorValue <= m_maxColorValue)
        m_minColorValue =
            (minColorValue >= m_minValue) ? minColorValue : m_minValue;
    else
        m_minColorValue = m_maxColorValue;

    ASSERT_VALID_RANGE();
}

void ColorRange::setMaxColorValue(double maxColorValue)
{
    if (maxColorValue >= m_minColorValue)
        m_maxColorValue =
            (maxColorValue <= m_maxValue) ? maxColorValue : m_maxValue;
    else
        m_maxColorValue = m_minColorValue;

    ASSERT_VALID_RANGE();
}

void ColorRange::setRange(double minValue, double maxValue)
{
    Q_ASSERT(minValue <= maxValue);
    if (minValue > maxValue)
        return;
    
    if (m_minColorValue == m_minValue || m_minColorValue < minValue)
        m_minColorValue = minValue;
    else if (m_minColorValue > maxValue)
        m_minColorValue = maxValue;

    if (m_maxColorValue == m_maxValue || m_maxColorValue > maxValue)
        m_maxColorValue = maxValue;
    else if (m_maxColorValue < minValue)
        m_maxColorValue = minValue;

    m_minValue = minValue;
    m_maxValue = maxValue;

    ASSERT_VALID_RANGE();
}

void ColorRange::setRange(double minValue, double maxValue,
                          double minColorValue, double maxColorValue)
{
    Q_ASSERT(minValue <= maxValue);
    Q_ASSERT(minColorValue <= maxColorValue);
    if (minValue > maxValue || minColorValue > maxColorValue)
        return;

    m_minValue = minValue;
    m_maxValue = maxValue;

    if (minColorValue < minValue)
        m_minColorValue = minValue;
    else if (minColorValue > maxValue)
        m_minColorValue = maxValue;
    else
        m_minColorValue = minColorValue;

    if (maxColorValue > maxValue)
        m_maxColorValue = maxValue;
    else if (maxColorValue < minValue)
        m_maxColorValue = minValue;
    else
        m_maxColorValue = maxColorValue;

    ASSERT_VALID_RANGE();
}

void ColorRange::setColorSpread(double minColorValue, double maxColorValue)
{
    Q_ASSERT(minColorValue <= maxColorValue);
    if (minColorValue > maxColorValue)
        return;

    if (minColorValue < m_minValue)
        m_minColorValue = m_minValue;
    else if (minColorValue > m_maxValue)
        m_minColorValue = m_maxValue;
    else
        m_minColorValue = minColorValue;

    if (maxColorValue > m_maxValue)
        m_maxColorValue = m_maxValue;
    else if (maxColorValue < m_minValue)
        m_maxColorValue = m_minValue;
    else
        m_maxColorValue = maxColorValue;

    ASSERT_VALID_RANGE();
}

double ColorRange::minColorRatio() const
{
    if (width() <= 0) return 0.0;
    return (m_minColorValue - m_minValue) / width();
}

void ColorRange::setMinColorRatio(double minColorRatio)
{
    setMinColorValue(m_minValue + minColorRatio * width());
}

double ColorRange::maxColorRatio() const
{
    if (width() <= 0) return 1.0;
    return (m_maxColorValue - m_minValue) / width();
}

void ColorRange::setMaxColorRatio(double maxColorRatio)
{
    setMaxColorValue(m_minValue + maxColorRatio * width());
}

void ColorRange::setColorSpreadRatio(double minColorRatio,
                                     double maxColorRatio)
{
    Q_ASSERT(minColorRatio <= maxColorRatio);
    setColorSpread(m_minValue + minColorRatio * width(),
                   m_minValue + maxColorRatio * width());
}


} // namespace CamSys
