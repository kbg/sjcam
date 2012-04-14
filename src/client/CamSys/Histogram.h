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

#ifndef CAMSYS_HISTOGRAM_H
#define CAMSYS_HISTOGRAM_H

#include <QtCore/QVector>

namespace CamSys {

class Image;

//! \addtogroup CamSysCore
//! @{

class Histogram
{
public:
    explicit Histogram(int bins = 10, double lowerEdge = 0.0,
        double upperEdge = 1.0, bool computeStats = false);

    Histogram(const Histogram &other);
    virtual ~Histogram();

    void reset();
    void reset(int bins, double lowerEdge, double upperEdge,
               bool computeStats = false);

    int bins() const;
    double lowerEdge() const;
    double upperEdge() const;
    double binWidth() const;
    int binIndex(double x) const;
    double binLowerEdge(int i) const;
    double binUpperEdge(int i) const;

    int entries() const;
    int allEntries() const;
    int minBinEntries() const;
    int maxBinEntries() const;
    int binEntries(int i) const;
    int underflowEntries() const;
    int overflowEntries() const;

    double mean() const;
    double rms() const;

    void fill(double x);
    void fill(const Image *image);

    template <typename T>
    void fill(const T *data, int count);

    QVector<int> binEntries() const;
    Histogram & operator = (const Histogram &other);

private:
    template <typename T>
    void fillByLine(const Image *image);

private:
    int m_bins;
    double m_lowerEdge, m_upperEdge, m_width;
    QVector<int> m_binEntries;
    int m_underflow, m_overflow;
    bool m_computeStats;
    double m_sumX, m_sumX2;
};

//! @} // end of group CamSysCore

template <typename T>
inline void Histogram::fill(const T *data, int count)
{
    const bool computeStats = m_computeStats;
    for (int i = 0; i < count; i++)
    {
        const double x = double(data[i]);
        if (x < m_lowerEdge) {
            m_underflow += 1;
        } else if (x > m_upperEdge) {
            m_overflow += 1;
        } else {
            const int idx = int(((x - m_lowerEdge) / m_width) * m_bins);
            m_binEntries[(idx < m_bins) ? idx : (idx - 1)] += 1;
            if (computeStats) {
                m_sumX += x;
                m_sumX2 += x * x;
            }
        }
    }
}

} // namespace CamSys

#endif // CAMSYS_HISTOGRAM_H
