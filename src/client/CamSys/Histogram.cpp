/*
 * Copyright (c) 2008-2010 Kolja Glogowski
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

#include "Histogram.h"
#include "Image.h"
#include <climits> // for INT_MIN and INT_MAX
#include <cfloat>  // for DBL_MIN and DBL_MAX
#include <cmath>   // for std::sqrt

namespace CamSys {

/*! \class Histogram
    \brief A histogram class.

    This class computes an one-dimensional histograms with fixed
    bin width from unweighted data.

    \note This histogram class is partly inspired by the AIDA IHistogram1D
          interface published at http://aida.freehep.org/doc/v3.2.1/api/

    \see HistogramWidget for a graphical representation of the histogram.
 */

/*!
    \brief Default constructor.

    This constructor creates a histogram with \a bins bins over the
    interval <tt>[\a lowerEdge, \a upperEdge]</tt>. If \a computeStats is
    set to true, the mean value and the RMS of the data will be computed.

    \note
      - The range of each bin is the half open intervall
        <tt>[\a lowerBinEdge, \a upperBinEdge)</tt>, so the bin's lower
        edge is included while its upper edge is excluded. The single
        exception is the last bin which also includes the histogram's
        \a upperEdge.
      - The number of bins is at least 1. If \a bins contains a lesser value,
        a value of 1 is used instead.
      - The \a lowerEdge is alway smaller than \a upperEdge. If the given
        \a lowerEdge is greater than \a upperEdge, both values are switched.
        If the given \a lowerEdge equals \a upperEdge, \a lowerEdge is
        decreased by 0.5 and \a upperEdge is increased by 0.5, resulting
        in a range of width 1.0 centered around the mutual value.

     \see reset(), mean(), rms()
 */
Histogram::Histogram(int bins, double lowerEdge, double upperEdge,
                     bool computeStats)
{
    reset(bins, lowerEdge, upperEdge, computeStats);
}

/*!
    \brief Copy constructor.
 */
Histogram::Histogram(const Histogram &other)
    : m_bins(other.m_bins),
      m_lowerEdge(other.m_lowerEdge),
      m_upperEdge(other.m_upperEdge),
      m_width(other.m_width),
      m_binEntries(other.m_binEntries),
      m_underflow(other.m_underflow),
      m_overflow(other.m_overflow),
      m_computeStats(other.m_computeStats),
      m_sumX(other.m_sumX),
      m_sumX2(other.m_sumX2)
{
}

/*!
    \brief Destructor.
 */
Histogram::~Histogram()
{
}

/*!
    \brief Reset the histogram.

    This methods sets all bin entries to 0 and clears the underflow
    and overflow bin.

    \see reset(int bins, double lowerEdge, double upperEdge)
 */
void Histogram::reset()
{
    Q_ASSERT(m_binEntries.size() == m_bins);

    m_binEntries.fill(0);
    m_underflow = m_overflow = 0;
    m_sumX = m_sumX2 = 0;
}

/*!
    \brief Reset the histogram.

    This method resets the histogram and sets a new number of bins and
    a new lower and upper edge. If \a computeStats is set to true, the mean
    value and the RMS of the data will be computed.

    \see reset(), Histogram(), mean(), rms()
 */
void Histogram::reset(int bins, double lowerEdge, double upperEdge,
                      bool computeStats)
{
    m_bins = (bins > 0) ? bins : 1;

    if (lowerEdge < upperEdge) {
        m_lowerEdge = lowerEdge;
        m_upperEdge = upperEdge;
    }
    else if (lowerEdge > upperEdge) {
        m_lowerEdge = upperEdge;
        m_upperEdge = lowerEdge;
    }
    else {
        m_lowerEdge = lowerEdge - 0.5;
        m_upperEdge = upperEdge + 0.5;
    }

    m_width = m_upperEdge - m_lowerEdge;
    m_binEntries.resize(m_bins);
    m_binEntries.fill(0);
    m_underflow = 0;
    m_overflow = 0;
    m_computeStats = computeStats;
    m_sumX = 0;
    m_sumX2 = 0;

    Q_ASSERT(m_bins > 0);
    Q_ASSERT(m_lowerEdge < m_upperEdge);
}

/*!
    \brief Get the total number of bins.
 */
int Histogram::bins() const
{
    return m_bins;
}

/*!
    \brief Get the lower edge of the histogram range.

    \note This endpoint is included in the first bin.
 */
double Histogram::lowerEdge() const
{
    return m_lowerEdge;
}

/*!
    \brief Get the upper edge of the histogram range.

    \note This endpoint is included in the last bin.
 */
double Histogram::upperEdge() const
{
    return m_upperEdge;
}

/*!
    \brief Returns the width of each bin.
 */
double Histogram::binWidth() const
{
    return m_width / m_bins;
}

/*!
    \brief Get the bin index corresponding the given \a x coordinate.

    \note If \a x is below lowerEdge() or above upperEdge(), the returned
          index will be -1 or bins() respectively.
 */
int Histogram::binIndex(double x) const
{
    if (x < m_lowerEdge)
        return -1;
    else if (x > m_upperEdge)
        return m_bins;

    const int idx = int(((x - m_lowerEdge) / m_width) * m_bins);
    return (idx < m_bins) ? idx : (idx - 1);
}

/*!
    \brief Get the lower edge of the bin coordinate corresponding to
        the given index \a i.

    \note If the index \a i is below 0, the returned value is DBL_MIN. If
          the index is greater than or equal to bins(), the returned value
          is the upper edge of the last in-range bin (i.e. upperEdge()),
          even though this value is included in the last in-range bin.
 */
double Histogram::binLowerEdge(int i) const
{
    if (i < 0)
        return DBL_MIN;
    else if (i >= m_bins)
        return m_upperEdge;

    return m_lowerEdge + (m_width * i) / m_bins;
}

/*!
    \brief Get the upper edge of the bin coordinate corresponding to
        the given index \a i.

    \note If the index \a i is below 0, the returned value is the lower edge
        of the first in-range bin (i.e. lowerEdge()). If the index is greater
        than or equal to bins(), the returned value is DBL_MAX.
 */
double Histogram::binUpperEdge(int i) const
{
    if (i < 0)
        return m_lowerEdge;
    else if (i >= m_bins)
        return DBL_MAX;

    return m_lowerEdge + (m_width * (i + 1)) / m_bins;
}

/*!
    \brief Get the number of all entries contained in the in-range bins.

    \see allEntries()
 */
int Histogram::entries() const
{
    int sum = 0;
    for (int i = 0; i < m_bins; ++i)
        sum += m_binEntries[i];
    return sum;
}

/*!
    \brief Get the total number of entries, including all in-range bins
        aswell as the underflow and overflow bin.

    \see entries()
 */
int Histogram::allEntries() const
{
    return entries() + m_underflow + m_overflow;
}

/*!
    \brief Get the minimum number of entries in the in-range bins.

    \note The overflow and underflow bins are excluded from this operation.
 */
int Histogram::minBinEntries() const
{
    int min = INT_MAX;
    for (int i = 0; i < m_bins; ++i)
        if (min > m_binEntries[i])
            min = m_binEntries[i];
    return min;
}

/*!
    \brief Get the maximum number of entries in the in-range bins.

    \note The overflow and underflow bins are excluded from this operation.
 */
int Histogram::maxBinEntries() const
{
    int max = INT_MIN;
    for (int i = 0; i < m_bins; ++i)
        if (max < m_binEntries[i])
            max = m_binEntries[i];
    return max;
}

/*!
    \brief Get the number of entries contained in bin \a i.

    \note If the index \a i is smaller than 0 or \a i is greater than
        or equal to bins(), the returned number are the entries of the
        underflow or overflow bin respectively.
 */
int Histogram::binEntries(int i) const
{
    if (i < 0)
        return m_underflow;
    else if (i >= m_bins)
        return m_overflow;
    else
        return m_binEntries[i];
}

/*!
    \brief Get the number of entries in the underflow bin.
 */
int Histogram::underflowEntries() const
{
    return m_underflow;
}

/*!
    \brief Get the number of entries in the overflow bin.
 */
int Histogram::overflowEntries() const
{
    return m_overflow;
}

/*!
    \brief Get the mean value of the histogram.

    The mean value is calculatet by:
        \f[ \overline{x} = \frac{1}{n} \sum_{i=1}^n x_i \f]

    \note To use this method, \a computeStats must have been true, when the
          constructor or the reset() method was called.

    \see Histogram(), reset()
 */
double Histogram::mean() const
{
    int n = entries();
    return (n != 0) ? m_sumX / n : 0;
}

/*!
    \brief Get the root mean square of the histogram.

    The root mean square is calculated by:
        \f[ x_{rms} = \sqrt{ \frac{1}{n} \sum_{i=1}^n x_i^2 } \f]

    \note To use this method, \a computeStats must have been true, when the
          constructor or the reset() method was called.

    \see Histogram(), reset()
 */
double Histogram::rms() const
{
    int n = entries();
    return (n != 0) ? std::sqrt(m_sumX2 / n) : 0;
}

/*!
    \brief Fill the histogram with the given \a x value.
 */
void Histogram::fill(double x)
{
    if (x < m_lowerEdge) {
        m_underflow += 1;
        return;
    }

    if (x > m_upperEdge) {
        m_overflow += 1;
        return;
    }

    const int idx = int(((x - m_lowerEdge) / m_width) * m_bins);
    m_binEntries[(idx < m_bins) ? idx : (idx - 1)] += 1;

    if (m_computeStats) {
        m_sumX += x;
        m_sumX2 += x * x;
    }
}


/*!
    \brief Fill the histogram with the pixel values from the given image.
 */
void Histogram::fill(const Image *image)
{
    Q_ASSERT(image);

    switch (image->format())
    {
    case Image::Uint8:
        fillByLine<Image::FormatInfo<Image::Uint8>::type>(image);
        return;
    case Image::Int8:
        fillByLine<Image::FormatInfo<Image::Int8>::type>(image);
        return;
    case Image::Uint16:
        fillByLine<Image::FormatInfo<Image::Uint16>::type>(image);
        return;
    case Image::Int16:
        fillByLine<Image::FormatInfo<Image::Int16>::type>(image);
        return;
    case Image::Uint32:
        fillByLine<Image::FormatInfo<Image::Uint32>::type>(image);
        return;
    case Image::Int32:
        fillByLine<Image::FormatInfo<Image::Int32>::type>(image);
        return;
    case Image::Float32:
        fillByLine<Image::FormatInfo<Image::Float32>::type>(image);
        return;
    case Image::Float64:
        fillByLine<Image::FormatInfo<Image::Float64>::type>(image);
        return;
    }

    Q_ASSERT(false);
}

/*! \fn void Histogram::fill<T>(const T *data, int count)
    \brief Fill the histogram from the array \a data of the given \a size.

\note The template argument must be convertable to double, i.e. it must
    support the following conversion:
    \code
    T x;
    double d = double(x);
    \endcode
 */

/*!
    \brief Get a copy of all in-range bin entries.

    The vector returned, includes all in-range bins entries. To get the
    entries of the underflow and overflow bins use the underflowEntries()
    or overflowEntries().
 */
QVector<int> Histogram::binEntries() const
{
    return m_binEntries;
}

/*!
    \brief Assignement operator.
 */
Histogram & Histogram::operator = (const Histogram &other)
{
    if (this != &other)
    {
        m_bins = other.m_bins;
        m_lowerEdge = other.m_lowerEdge;
        m_upperEdge = other.m_upperEdge;
        m_width = other.m_width;
        m_binEntries = other.m_binEntries;
        m_underflow = other.m_underflow;
        m_overflow = other.m_overflow;
        m_computeStats = other.m_computeStats;
        m_sumX = other.m_sumX;
        m_sumX2 = other.m_sumX2;
    }

    return *this;
}

/*!
    \brief Helper function used by fill(const Image *image)
 */
template <typename T>
inline void Histogram::fillByLine(const Image *image)
{
    const int width = image->width();
    const int height = image->height();
    for (int i = 0; i < height; ++i)
        fill(image->scanLine<T>(i), width);
}

} // namespace CamSys
