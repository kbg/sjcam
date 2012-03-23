#ifndef SJCAM_SJCDATA_H
#define SJCAM_SJCDATA_H

#include <QtCore/QByteArray>
#include <QtCore/QVariant>

struct NamedValue
{
    NamedValue() {}
    NamedValue(const QByteArray &name_, const QVariant &value_)
        : name(name_), value(value_) {}
    QByteArray name;
    QVariant value;
};

struct StreamingFrame {
    quint32 x, y;
    quint32 width, height;
    quint32 depth;
    QByteArray jpeg;
};

#endif // SJCAM_SJCDATA_H

