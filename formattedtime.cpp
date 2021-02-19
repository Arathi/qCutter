#include "formattedtime.h"

FormattedTime::FormattedTime() : hour(0), minute(0), second(0)
{
}

FormattedTime::FormattedTime(QString formatted)
{
    QRegExp regex("([0-9]+):([0-9]+):([0-9\\.]+)");
    int idx = regex.indexIn(formatted);
    if (idx != -1) {
        hour = regex.cap(1).toInt();
        minute = regex.cap(2).toInt();
        second = regex.cap(3).toDouble();
    }
}

int FormattedTime::toMillisecond()
{
    return hour * 3600000 + minute * 60000 + second * 1000;
}

QString FormattedTime::toString()
{
    QString output("");
    output.append(QString::number(hour));
    output.append(":");
    output.append(QString::number(minute));
    output.append(":");
    output.append(QString::number(second));
    return output;
}
