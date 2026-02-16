#include "../headers/quotesdialog.h"
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFont>
#include <QPainter>
#include <QStyleOption>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QApplication>
#include <algorithm>

QuoteItem::QuoteItem(const QString &text, bool unlocked, QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(60);
    setStyleSheet("border: 1px solid #ddd; border-radius: 6px; background: white; margin: 2px 0;");

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 10, 15, 10);

    // Круглый значок в виде зелёной галочки для открытых цитат и серого знака вопроса для закрытых
    auto *iconLabel = new QLabel(this);
    iconLabel->setFixedSize(36, 36);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFont(QFont("Sans", 16, QFont::Bold));

    if (unlocked) {
        iconLabel->setText("✓");
        iconLabel->setStyleSheet("QLabel { color: white; background-color: #27ae60; border-radius: 18px; }");
    } else {
        iconLabel->setText("?");
        iconLabel->setStyleSheet("QLabel { color: white; background-color: #95a5a6; border-radius: 18px; }");
    }

    // Текст цитаты или заглушка не открыта для закрытых позиций
    auto *textLabel = new QLabel(unlocked ? text : "🔒 Эта цитата ещё не открыта", this);
    textLabel->setWordWrap(true);
    textLabel->setFont(QFont("Sans", 12));
    textLabel->setStyleSheet(unlocked ?
                                 "QLabel { color: #2c3e50; font-style: italic; }" :
                                 "QLabel { color: #95a5a6; font-style: italic; }"
                             );

    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    layout->addStretch();
}

// Переопределяем paintEvent, чтобы QSS-стили корректно применялись к кастомному QWidget
void QuoteItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

QuotesDialog::QuotesDialog(const QList<QPair<QString, bool>> &quotes, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Моя коллекция цитат 📚");
    setMinimumSize(450, 500);
    setupUI(quotes);
}

void QuotesDialog::setupUI(const QList<QPair<QString, bool>> &quotes)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // Шапка с прогрессом, то есть сколько цитат уже открыто из общего числа
    m_progressLabel = new QLabel(this);
    m_progressLabel->setAlignment(Qt::AlignCenter);
    m_progressLabel->setFont(QFont("Sans", 18, QFont::Bold));
    int unlocked = std::count_if(quotes.begin(), quotes.end(),
                                 [](const QPair<QString, bool> &q) { return q.second; });
    m_progressLabel->setText(QString("Открыто цитат: %1 из %2")
                                 .arg(unlocked).arg(quotes.size()));
    m_progressLabel->setStyleSheet("QLabel { color: #2980b9; padding: 8px; background-color: #e3f2fd; border-radius: 6px; }");

    // Прокручиваемый список всех цитат
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *scrollWidget = new QWidget(scrollArea);
    auto *scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setSpacing(4);
    scrollLayout->setContentsMargins(0, 0, 0, 0);

    for (int i = 0; i < quotes.size(); ++i) {
        // Нумеруем цитату для удобства навигации по коллекции
        QString displayText = QString("%1. %2").arg(i + 1).arg(quotes[i].first);
        auto *item = new QuoteItem(displayText, quotes[i].second);
        scrollLayout->addWidget(item);
    }
    scrollLayout->addStretch(1);
    scrollWidget->setLayout(scrollLayout);
    scrollArea->setWidget(scrollWidget);

    // Подсказка с процентом прохождения коллекции
    auto *statsLabel = new QLabel(this);
    statsLabel->setWordWrap(true);
    statsLabel->setAlignment(Qt::AlignCenter);
    statsLabel->setStyleSheet("QLabel { color: #7f8c8d; font-size: 13px; margin: 8px 0; }");
    statsLabel->setText(QString(
                            "💡 Каждая завершённая сессия открывает одну новую цитату.\n"
                            "Ты на %1% пути к полной коллекции!"
                            ).arg(qRound(unlocked * 100.0 / qMax(1, quotes.size()))));

    auto *closeButton = new QPushButton("Закрыть", this);
    closeButton->setMinimumHeight(36);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    mainLayout->addWidget(m_progressLabel);
    mainLayout->addWidget(scrollArea, 1);
    mainLayout->addWidget(statsLabel);
    mainLayout->addWidget(closeButton);
}