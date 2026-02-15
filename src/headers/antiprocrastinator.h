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
    void updateDisplay();
    void timerFinished();
    void showMotivationalQuote();
    void changeTheme(int index);
    void changeDuration(int minutes);
    void showQuotesCollection();

private:
    bool initDatabase();
    void loadEnvironmentConfig();
    void loadQuotes();
    void loadProgress();
    void saveProgress();
    void unlockQuoteForSession(int sessionId);
    void applyTheme(const QString &themeName);
    void setupUI();
    void setupMenuBar();

    QLabel *m_timeLabel;
    QLabel *m_sessionCounterLabel;
    QLabel *m_quoteLabel;
    QPushButton *m_startButton;
    QPushButton *m_pauseButton;
    QPushButton *m_resetButton;
    QComboBox *m_themeComboBox;
    QSpinBox *m_durationSpinBox;

    QTimer *m_timer;
    QTime m_remainingTime;
    int m_pomodoroMinutes = 25;
    int m_sessionsCompleted = 0;
    bool m_isRunning = false;

    QList<QString> m_allQuotes;
    QList<QPair<QString, bool>> m_quotesWithStatus; // (text, unlocked)
    int m_unlockedCount = 0;

    QString m_quotesFilePath;
    QString m_defaultTheme;
    int m_defaultDuration;

    QSqlDatabase m_db;
    QString m_dbPath;
};

#endif // ANTIPROCASTINATOR_H
