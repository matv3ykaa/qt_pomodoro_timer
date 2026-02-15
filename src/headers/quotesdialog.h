#ifndef QUOTESDIALOG_H
#define QUOTESDIALOG_H

#include <QDialog>
#include <QPair>
#include <QString>
#include <QList>

class QLabel;

class QuoteItem : public QWidget {
    Q_OBJECT
public:
    explicit QuoteItem(const QString &text, bool unlocked, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

class QuotesDialog : public QDialog {
    Q_OBJECT
public:
    explicit QuotesDialog(const QList<QPair<QString, bool>> &quotes, QWidget *parent = nullptr);

private:
    void setupUI(const QList<QPair<QString, bool>> &quotes);
    QLabel *m_progressLabel;
};

#endif // QUOTESDIALOG_H
