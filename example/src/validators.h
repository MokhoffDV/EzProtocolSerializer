#ifndef VALIDATORS_H
#define VALIDATORS_H

#include <QValidator>
#include <QString>

class UInt64Validator : public QValidator
{
public:
    UInt64Validator(quint64 max, QObject* parent = nullptr);

    State validate(QString& input, int& pos) const override;
    void fixup(QString& input) const override;
    qint64 min() const;
    qint64 max() const;
private:
    quint64 m_max;
};

class Int64Validator : public QValidator
{
public:
    Int64Validator(qint64 min, qint64 max, QObject* parent = nullptr);

    State validate(QString& input, int& pos) const override;
    void fixup(QString& input) const override;
    qint64 min() const;
    qint64 max() const;

private:
    qint64 m_min;
    qint64 m_max;
};

#endif // VALIDATORS_H