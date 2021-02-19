#ifndef TIME_H
#define TIME_H

#include <QString>
#include <QRegExp>

class FormattedTime
{
public:
    FormattedTime();
    FormattedTime(QString formatted);
    // static Time parse(QString formatted);
    int toMillisecond();
    QString toString();

    int hour;
    int minute;
    double second;
};

#endif // TIME_H
