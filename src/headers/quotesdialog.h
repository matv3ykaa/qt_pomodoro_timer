#ifndef QUOTESDIALOG_H
#define QUOTESDIALOG_H

#include <QDialog>
#include <QPair>
#include <QString>
#include <QList>

class QLabel;

// Один элемент списка коллекции: иконка статуса + текст цитаты (или заглушка)
class QuoteItem : public QWidget {
    Q_OBJECT
public:
    explicit QuoteItem(const QString &text, bool unlocked, QWidget *parent = nullptr);

protected:
    // Нужен для корректной отрисовки QSS-стилей на кастомном QWidget
    void paintEvent(QPaintEvent *event) override;
};

// Диалоговое окно с прокручиваемым списком всех цитат и счётчиком прогресса
class QuotesDialog : public QDialog {
    Q_OBJECT
public:
    // quotes — список пар (текст цитаты, открыта ли), передаётся из главного окна
    explicit QuotesDialog(const QList<QPair<QString, bool>> &quotes, QWidget *parent = nullptr);

private:
    void setupUI(const QList<QPair<QString, bool>> &quotes);

    QLabel *m_progressLabel; // Заголовок «Открыто X из N цитат»
};

#endif // QUOTESDIALOG_H