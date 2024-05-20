#include "validators.h"

UInt64Validator::UInt64Validator(quint64 max, QObject* parent)
    : QValidator(parent), m_max(max)
{

}

QValidator::State UInt64Validator::validate(QString& input, int& pos) const
{
    Q_UNUSED(pos);
    if (input.isEmpty())
        return Intermediate;
    bool ok;
    const quint64 value = input.toULongLong(&ok);
    if (ok && value >= 0 && value <= m_max)
        return Acceptable;
    return Invalid;
}

void UInt64Validator::fixup(QString& input) const
{
    bool ok;
    const quint64 value = input.toULongLong(&ok);
    if (!ok || value < 0)
        input = "0";
    else if (value > m_max)
        input = QString::number(m_max);
}

qint64 UInt64Validator::min() const
{
    return 0;
}

qint64 UInt64Validator::max() const
{
    return m_max;
}

Int64Validator::Int64Validator(qint64 min, qint64 max, QObject* parent)
    : QValidator(parent), m_min(min), m_max(max)
{

}

QValidator::State Int64Validator::validate(QString& input, int& pos) const
{
    Q_UNUSED(pos);
    if (input.isEmpty())
        return Intermediate;

    if (input.length() == 1 && input.at(0) == '-')
        return Intermediate;

    bool ok;
    const qint64 value = input.toLongLong(&ok);
    if (ok && value >= m_min && value <= m_max)
        return Acceptable;
    return Invalid;
}

void Int64Validator::fixup(QString& input) const
{
    bool ok;
    const qint64 value = input.toULongLong(&ok);
    if (!ok || value < m_min)
        input = QString::number(m_min);
    else if (value > m_max)
        input = QString::number(m_max);
}

qint64 Int64Validator::min() const
{
    return m_min;
}

qint64 Int64Validator::max() const
{
    return m_max;
}
