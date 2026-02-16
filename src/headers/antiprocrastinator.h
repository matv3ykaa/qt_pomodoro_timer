#ifndef ANTIPROCASTINATOR_H
#define ANTIPROCASTINATOR_H

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QList>
#include <QPair>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDir>
#include <QStandardPaths>

class QuotesDialog;

class Antiprocrastinator : public QMainWindow
{
    Q_OBJECT

public:
    explicit Antiprocrastinator(QWidget *parent = nullptr);
    ~Antiprocrastinator() override;

private slots:
    void startTimer();
    void pauseTimer();
    void resetTimer();
    void updateDisplay();   // Вызывается каждую секунду, уменьшает счётчик и обновляет метку
    void timerFinished();   // Сессия завершена: сохраняет в БД и открывает цитату
    void showMotivationalQuote();
    void changeTheme(int index);
    void changeDuration(int minutes);
    void showQuotesCollection();

private:
    bool initDatabase();            // Создаёт таблицы sessions и settings, если их нет
    void loadEnvironmentConfig();   // Читает .env: путь к цитатам, тема, длительность, путь к БД
    void loadQuotes();              // Загружает цитаты из файла (с fallback на встроенный список)
    void loadProgress();            // Восстанавливает количество сессий и открытые цитаты из БД
    void saveProgress();            // Записывает тему и длительность в таблицу settings
    void unlockQuoteForSession(int sessionId);
    void applyTheme(const QString &themeName);  // Переключает тему между темной и светлой
    void setupUI();
    void setupMenuBar();

    // Виджеты интерфейса
    QLabel      *m_timeLabel;           // Отображает оставшееся время в формате MM:SS
    QLabel      *m_sessionCounterLabel; // Показывает, сколько сессий завершено
    QLabel      *m_quoteLabel;          // Область для мотивационных цитат
    QPushButton *m_startButton;
    QPushButton *m_pauseButton;
    QPushButton *m_resetButton;
    QComboBox   *m_themeComboBox;
    QSpinBox    *m_durationSpinBox;

    // Состояние таймера
    QTimer *m_timer;
    QTime   m_remainingTime;
    int     m_pomodoroMinutes = 25;
    bool    m_isRunning = false;

    // Данные о прогрессе
    int m_sessionsCompleted = 0;

    QList<QString>              m_allQuotes;          // Все цитаты из файла
    QList<QPair<QString, bool>> m_quotesWithStatus;   // Текст цитаты, открыта ли
    int m_unlockedCount = 0;                          // Сколько цитат уже доступно пользователю

    // Конфигурация из .env
    QString m_quotesFilePath;
    QString m_defaultTheme;
    int     m_defaultDuration;

    // База данных SQLite
    QSqlDatabase m_db;
    QString      m_dbPath;
};

#endif // ANTIPROCASTINATOR_H