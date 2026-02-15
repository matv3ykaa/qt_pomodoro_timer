#include "../headers/antiprocrastinator.h"
#include "../headers/quotesdialog.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QApplication>
#include <QStringConverter>
#include <QMenuBar>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QTimer>
#include <QDebug>

Antiprocrastinator::Antiprocrastinator(QWidget *parent)
    : QMainWindow(parent)
    , m_timer(new QTimer(this))
{
    loadEnvironmentConfig();

    if (!initDatabase()) {
        QMessageBox::critical(this, "Ошибка базы данных",
                              "Не удалось инициализировать базу данных прогресса.\n"
                              "Приложение будет работать в режиме только для чтения.");
    }

    loadQuotes();
    setupUI();
    setupMenuBar();
    loadProgress();
    applyTheme(m_defaultTheme);

    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &Antiprocrastinator::updateDisplay);
    connect(m_startButton, &QPushButton::clicked, this, &Antiprocrastinator::startTimer);
    connect(m_pauseButton, &QPushButton::clicked, this, &Antiprocrastinator::pauseTimer);
    connect(m_resetButton, &QPushButton::clicked, this, &Antiprocrastinator::resetTimer);
    connect(m_themeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Antiprocrastinator::changeTheme);
    connect(m_durationSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &Antiprocrastinator::changeDuration);

    resetTimer();
}

Antiprocrastinator::~Antiprocrastinator()
{
    saveProgress();
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase("progress_db");
}

void Antiprocrastinator::loadEnvironmentConfig()
{
    QStringList searchPaths = {
        QApplication::applicationDirPath(),
        QApplication::applicationDirPath() + "/..",
        QDir::currentPath()
    };

    QString envFile;
    for (const QString &path : searchPaths) {
        QString candidate = path + "/.env";
        if (QFile::exists(candidate)) {
            envFile = candidate;
            break;
        }
    }

    m_quotesFilePath = "quotes.txt";
    m_defaultTheme = "light";
    m_defaultDuration = 25;
    m_dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
               + "/antiprocrastinator/progress.db";

    if (!envFile.isEmpty()) {
        QFile file(envFile);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            in.setEncoding(QStringConverter::Utf8);

            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.isEmpty() || line.startsWith("#")) continue;

                QRegularExpression re(R"(^\s*(\w+)\s*=\s*(.+?)\s*$)");
                QRegularExpressionMatch match = re.match(line);
                if (match.hasMatch()) {
                    QString key = match.captured(1);
                    QString value = match.captured(2).trimmed();
                    if (value.startsWith("\"") && value.endsWith("\"")) {
                        value = value.mid(1, value.length() - 2);
                    }
                    if (value.startsWith("'") && value.endsWith("'")) {
                        value = value.mid(1, value.length() - 2);
                    }

                    if (key == "QUOTES_FILE_PATH") m_quotesFilePath = value;
                    else if (key == "DEFAULT_DURATION") m_defaultDuration = value.toInt();
                    else if (key == "DEFAULT_THEME") m_defaultTheme = value;
                    else if (key == "DB_PATH") m_dbPath = value;
                }
            }
            file.close();
        }
    }

    QFileInfo dbFileInfo(m_dbPath);
    if (!dbFileInfo.dir().exists()) {
        dbFileInfo.dir().mkpath(".");
    }
}

bool Antiprocrastinator::initDatabase()
{
    if (QSqlDatabase::contains("progress_db")) {
        QSqlDatabase::removeDatabase("progress_db");
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE", "progress_db");
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        qWarning() << "Не удалось открыть БД:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS sessions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            start_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            duration_minutes INTEGER NOT NULL
        )
    )")) {
        qWarning() << "Ошибка создания таблицы sessions:" << query.lastError();
        return false;
    }

    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        )
    )")) {
        qWarning() << "Ошибка создания таблицы settings:" << query.lastError();
        return false;
    }

    query.prepare("INSERT OR IGNORE INTO settings (key, value) VALUES (:key, :value)");
    query.bindValue(":key", "theme");
    query.bindValue(":value", m_defaultTheme);
    query.exec();

    query.prepare("INSERT OR IGNORE INTO settings (key, value) VALUES (:key, :value)");
    query.bindValue(":key", "duration");
    query.bindValue(":value", QString::number(m_defaultDuration));
    query.exec();

    return true;
}

void Antiprocrastinator::loadQuotes()
{
    QStringList searchPaths = {
        QApplication::applicationDirPath(),
        QApplication::applicationDirPath() + "/../share/antiprocrastinator",
        QApplication::applicationDirPath() + "/../../share/antiprocrastinator",
        QDir::currentPath()
    };

    if (QFileInfo(m_quotesFilePath).isAbsolute()) {
        searchPaths.prepend(m_quotesFilePath);
    } else {
        searchPaths.prepend(QApplication::applicationDirPath() + "/" + m_quotesFilePath);
    }

    QString quotesFile;
    for (const QString &path : searchPaths) {
        QString candidate = QFileInfo(path).isAbsolute() ? path : (QApplication::applicationDirPath() + "/" + path);
        if (QFile::exists(candidate)) {
            quotesFile = candidate;
            break;
        }
    }

    if (quotesFile.isEmpty()) {
        m_allQuotes = {
            "Ты ближе к цели, чем вчера!",
            "Один шаг за раз — и горы сдвинутся.",
            "Прокрастинация — враг мечты. Ты сегодня её победил!",
            "25 минут усилий = час гордости за себя.",
            "Не идеально, но сделано — лучше чем идеально, но не сделано.",
            "Ты уже сделал больше, чем тот, кто даже не начал.",
            "Маленькие шаги ведут к большим результатам.",
            "Сегодняшняя дисциплина — завтрашняя свобода.",
            "Ты сильнее прокрастинации!",
            "Каждая минута продуктивности — инвестиция в будущее себя."
        };
        qWarning() << "Файл цитат не найден, используются встроенные фразы";
        return;
    }

    QFile file(quotesFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty() && !line.startsWith("#")) {
                m_allQuotes.append(line);
            }
        }
        file.close();
        qDebug() << "Загружено цитат:" << m_allQuotes.size();
    } else {
        qWarning() << "Ошибка чтения файла цитат:" << quotesFile;
        m_allQuotes = {
            "Ты ближе к цели, чем вчера!",
            "Один шаг за раз — и горы сдвинутся.",
            "Прокрастинация — враг мечты. Ты сегодня её победил!",
            "25 минут усилий = час гордости за себя.",
            "Не идеально, но сделано — лучше чем идеально, но не сделано.",
            "Ты уже сделал больше, чем тот, кто даже не начал.",
            "Маленькие шаги ведут к большим результатам.",
            "Сегодняшняя дисциплина — завтрашняя свобода.",
            "Ты сильнее прокрастинации!",
            "Каждая минута продуктивности — инвестиция в будущее себя."
        };
    }
}

void Antiprocrastinator::loadProgress()
{
    if (!m_db.isOpen()) {
        m_sessionsCompleted = 0;
        m_pomodoroMinutes = m_defaultDuration;
        m_unlockedCount = 0;
        m_quotesWithStatus.clear();
        for (const QString &quote : m_allQuotes) {
            m_quotesWithStatus.append({quote, false});
        }
        return;
    }

    QSqlQuery query(m_db);
    if (query.exec("SELECT COUNT(*) FROM sessions") && query.next()) {
        m_sessionsCompleted = query.value(0).toInt();
    } else {
        m_sessionsCompleted = 0;
    }

    query.prepare("SELECT value FROM settings WHERE key = 'theme'");
    if (query.exec() && query.next()) {
        m_defaultTheme = query.value(0).toString();
    } else {
        m_defaultTheme = "light";
    }

    query.prepare("SELECT value FROM settings WHERE key = 'duration'");
    if (query.exec() && query.next()) {
        bool ok;
        int duration = query.value(0).toInt(&ok);
        m_pomodoroMinutes = ok ? duration : m_defaultDuration;
    } else {
        m_pomodoroMinutes = m_defaultDuration;
    }

    m_unlockedCount = qMin(m_sessionsCompleted, m_allQuotes.size());

    m_quotesWithStatus.clear();
    for (int i = 0; i < m_allQuotes.size(); ++i) {
        bool unlocked = (i < m_unlockedCount);
        m_quotesWithStatus.append({m_allQuotes[i], unlocked});
    }

    m_sessionCounterLabel->setText(QString("Сессий завершено: %1").arg(m_sessionsCompleted));
    m_durationSpinBox->setValue(m_pomodoroMinutes);

    if (m_unlockedCount > 0) {
        m_quoteLabel->setText(QString("❝%1❞").arg(m_quotesWithStatus[m_unlockedCount - 1].first));
    } else {
        m_quoteLabel->setText("🍅 Начни первую сессию, чтобы открыть цитату!");
    }

    qDebug() << "Прогресс загружен: сессий =" << m_sessionsCompleted
             << ", открыто цитат =" << m_unlockedCount
             << ", всего цитат =" << m_allQuotes.size();
}

void Antiprocrastinator::saveProgress()
{
    if (!m_db.isOpen()) return;

    m_db.transaction();

    QSqlQuery query(m_db);
    query.prepare("UPDATE settings SET value = :value WHERE key = 'theme'");
    query.bindValue(":value", m_themeComboBox->currentData().toString());
    if (!query.exec()) {
        m_db.rollback();
        return;
    }

    query.prepare("UPDATE settings SET value = :value WHERE key = 'duration'");
    query.bindValue(":value", QString::number(m_durationSpinBox->value()));
    if (!query.exec()) {
        m_db.rollback();
        return;
    }

    m_db.commit();
}

void Antiprocrastinator::setupUI()
{
    setWindowTitle("Антипрокрастинатор 🍅");
    setMinimumSize(450, 400);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    m_timeLabel = new QLabel("25:00", centralWidget);
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; margin: 10px 0; }");

    m_sessionCounterLabel = new QLabel("Сессий завершено: 0", centralWidget);
    m_sessionCounterLabel->setAlignment(Qt::AlignCenter);
    m_sessionCounterLabel->setStyleSheet("QLabel { font-size: 18px; color: #3498db; margin-bottom: 15px; }");

    m_quoteLabel = new QLabel("🍅 Начни первую сессию, чтобы открыть цитату!", centralWidget);
    m_quoteLabel->setAlignment(Qt::AlignCenter);
    m_quoteLabel->setWordWrap(true);
    m_quoteLabel->setMinimumHeight(70);
    m_quoteLabel->setStyleSheet("QLabel { font-size: 16px; font-style: italic; color: #7f8c8d; padding: 10px; }");

    m_startButton = new QPushButton("▶️ Старт", centralWidget);
    m_pauseButton = new QPushButton("⏸ Пауза", centralWidget);
    m_resetButton = new QPushButton("↻ Сброс", centralWidget);
    m_pauseButton->setEnabled(false);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addWidget(m_resetButton);

    m_themeComboBox = new QComboBox(centralWidget);
    m_themeComboBox->addItem("Светлая", "light");
    m_themeComboBox->addItem("Тёмная", "dark");

    m_durationSpinBox = new QSpinBox(centralWidget);
    m_durationSpinBox->setRange(5, 60);
    m_durationSpinBox->setSuffix(" мин");
    m_durationSpinBox->setValue(m_defaultDuration);

    QFormLayout *settingsLayout = new QFormLayout();
    settingsLayout->addRow("Тема интерфейса:", m_themeComboBox);
    settingsLayout->addRow("Длительность сессии:", m_durationSpinBox);

    QGroupBox *settingsGroup = new QGroupBox("Настройки", centralWidget);
    settingsGroup->setLayout(settingsLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(m_timeLabel);
    mainLayout->addWidget(m_sessionCounterLabel);
    mainLayout->addWidget(m_quoteLabel);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(settingsGroup);
    mainLayout->addStretch();

    centralWidget->setLayout(mainLayout);
}

void Antiprocrastinator::setupMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *quotesMenu = menuBar->addMenu("📚 Цитаты");
    QAction *viewCollectionAction = new QAction("Моя коллекция...", this);
    connect(viewCollectionAction, &QAction::triggered, this, &Antiprocrastinator::showQuotesCollection);
    quotesMenu->addAction(viewCollectionAction);

    QMenu *helpMenu = menuBar->addMenu("❓ Помощь");
    QAction *aboutAction = new QAction("О программе", this);
    connect(aboutAction, &QAction::triggered, this, []() {
        QMessageBox::information(nullptr, "Антипрокрастинатор",
                                 "🍅 Техника Помодоро + коллекционирование мотивационных цитат!\n\n"
                                 "• Каждая завершённая сессия открывает новую цитату\n"
                                 "• Прогресс сохраняется в надёжной базе данных SQLite\n"
                                 "• Настройки хранятся в файле .env\n"
                                 "• Синхронизация гарантируется транзакциями БД");
    });
    helpMenu->addAction(aboutAction);
}

void Antiprocrastinator::applyTheme(const QString &themeName)
{
    QPalette palette;
    if (themeName == "dark") {
        palette.setColor(QPalette::Window, QColor(53, 53, 53));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(35, 35, 35));
        palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(70, 70, 70));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::black);

        m_timeLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: #4fc3f7; margin: 10px 0; }");
        m_sessionCounterLabel->setStyleSheet("QLabel { font-size: 18px; color: #64b5f6; margin-bottom: 15px; }");
        m_quoteLabel->setStyleSheet("QLabel { font-size: 16px; font-style: italic; color: #b0bec5; padding: 10px; min-height: 70px; }");
    } else {
        palette.setColor(QPalette::Window, QColor(245, 247, 250));
        palette.setColor(QPalette::WindowText, QColor(44, 62, 80));
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::AlternateBase, QColor(240, 240, 240));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::black);
        palette.setColor(QPalette::Text, QColor(44, 62, 80));
        palette.setColor(QPalette::Button, QColor(230, 230, 230));
        palette.setColor(QPalette::ButtonText, QColor(44, 62, 80));
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Highlight, QColor(52, 152, 219));
        palette.setColor(QPalette::HighlightedText, Qt::white);

        m_timeLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: #2c3e50; margin: 10px 0; }");
        m_sessionCounterLabel->setStyleSheet("QLabel { font-size: 18px; color: #3498db; margin-bottom: 15px; }");
        m_quoteLabel->setStyleSheet("QLabel { font-size: 16px; font-style: italic; color: #7f8c8d; padding: 10px; min-height: 70px; }");
    }

    qApp->setPalette(palette);
    qApp->setStyleSheet(themeName == "dark" ?
                            "QToolTip { color: #ffffff; background-color: #2a2a2a; border: 1px solid white; padding: 5px; border-radius: 3px; }" :
                            "QToolTip { color: #000000; background-color: #ffffff; border: 1px solid #bdc3c7; padding: 5px; border-radius: 3px; }");

    int index = (themeName == "dark") ? 1 : 0;
    m_themeComboBox->setCurrentIndex(index);
}

void Antiprocrastinator::startTimer()
{
    if (!m_isRunning) {
        if (m_remainingTime <= QTime(0, 0, 0)) {
            m_remainingTime = QTime(0, m_pomodoroMinutes, 0);
        }
        m_timer->start();
        m_isRunning = true;
        m_startButton->setEnabled(false);
        m_pauseButton->setEnabled(true);
        m_startButton->setText("▶️ В работе...");
        m_durationSpinBox->setEnabled(false);
    }
}

void Antiprocrastinator::pauseTimer()
{
    if (m_isRunning) {
        m_timer->stop();
        m_isRunning = false;
        m_startButton->setEnabled(true);
        m_pauseButton->setEnabled(false);
        m_startButton->setText("▶️ Продолжить");
        m_durationSpinBox->setEnabled(true);
    }
}

void Antiprocrastinator::resetTimer()
{
    m_timer->stop();
    m_isRunning = false;
    m_remainingTime = QTime(0, m_pomodoroMinutes, 0);
    updateDisplay();
    m_startButton->setEnabled(true);
    m_pauseButton->setEnabled(false);
    m_startButton->setText("▶️ Старт");
    m_durationSpinBox->setEnabled(true);
}

void Antiprocrastinator::updateDisplay()
{
    if (m_isRunning) {
        m_remainingTime = m_remainingTime.addSecs(-1);
        if (m_remainingTime <= QTime(0, 0, 0)) {
            timerFinished();
            return;
        }
    }

    QString timeText = QString("%1:%2")
                           .arg(m_remainingTime.minute(), 2, 10, QChar('0'))
                           .arg(m_remainingTime.second(), 2, 10, QChar('0'));

    m_timeLabel->setText(timeText);
}

void Antiprocrastinator::timerFinished()
{
    m_timer->stop();
    m_isRunning = false;

    if (m_db.isOpen()) {
        m_db.transaction();

        QSqlQuery query(m_db);
        query.prepare("INSERT INTO sessions (duration_minutes) VALUES (:duration)");
        query.bindValue(":duration", m_pomodoroMinutes);
        if (!query.exec()) {
            m_db.rollback();
            QMessageBox::warning(this, "Ошибка", "Не удалось сохранить сессию. Прогресс может быть потерян.");
            return;
        }

        m_sessionsCompleted++;
        m_sessionCounterLabel->setText(QString("Сессий завершено: %1").arg(m_sessionsCompleted));

        if (m_unlockedCount < m_allQuotes.size()) {
            m_quotesWithStatus[m_unlockedCount].second = true;
            m_unlockedCount++;
        }

        if (!m_db.commit()) {
            QMessageBox::warning(this, "Ошибка", "Не удалось сохранить прогресс. Попробуйте ещё раз.");
            return;
        }

        qDebug() << "Сессия сохранена, открыта цитата #" << m_unlockedCount;
    } else {
        m_sessionsCompleted++;
        m_sessionCounterLabel->setText(QString("Сессий завершено: %1").arg(m_sessionsCompleted));
        if (m_unlockedCount < m_allQuotes.size()) {
            m_quotesWithStatus[m_unlockedCount].second = true;
            m_unlockedCount++;
        }
    }

    showMotivationalQuote();

    m_remainingTime = QTime(0, m_pomodoroMinutes, 0);
    updateDisplay();
    m_startButton->setEnabled(true);
    m_pauseButton->setEnabled(false);
    m_startButton->setText("▶️ Новый цикл");
    m_durationSpinBox->setEnabled(true);

    saveProgress();
}

void Antiprocrastinator::showMotivationalQuote()
{
    if (m_unlockedCount == 0) return;

    QString quote = m_quotesWithStatus[m_unlockedCount - 1].first;

    m_quoteLabel->setText("✨ Открыта новая цитата!");
    m_quoteLabel->setStyleSheet(
        "QLabel { font-size: 18px; font-weight: bold; color: #e67e22; "
        "background-color: #fff3cd; border-radius: 8px; padding: 10px; }"
        );

    QTimer::singleShot(1200, this, [this, quote]() {
        m_quoteLabel->setText(QString("❝%1❞").arg(quote));
        QString style = qApp->palette().color(QPalette::Base).lightness() < 128 ?
                            "QLabel { font-size: 18px; font-weight: bold; font-style: italic; color: #64b5f6; "
                            "background-color: #2a3b4d; border-radius: 8px; padding: 10px; }" :
                            "QLabel { font-size: 18px; font-weight: bold; font-style: italic; color: #27ae60; "
                            "background-color: #e8f5e9; border-radius: 8px; padding: 10px; }";
        m_quoteLabel->setStyleSheet(style);

        QTimer::singleShot(2500, this, [this]() {
            QString normalStyle = qApp->palette().color(QPalette::Base).lightness() < 128 ?
                                      "QLabel { font-size: 16px; font-style: italic; color: #b0bec5; padding: 10px; min-height: 70px; }" :
                                      "QLabel { font-size: 16px; font-style: italic; color: #7f8c8d; padding: 10px; min-height: 70px; }";
            m_quoteLabel->setStyleSheet(normalStyle);
        });
    });

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("🏆 Цитата открыта!");
    msgBox.setText(QString("Ты завершил %1 сессий и открыл %2 из %3 цитат!")
                       .arg(m_sessionsCompleted)
                       .arg(m_unlockedCount)
                       .arg(m_allQuotes.size()));
    msgBox.setInformativeText(QString("❝%1❞").arg(quote));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

void Antiprocrastinator::showQuotesCollection()
{
    QuotesDialog dialog(m_quotesWithStatus, this);
    dialog.exec();
}

void Antiprocrastinator::changeTheme(int index)
{
    Q_UNUSED(index);
    QString theme = m_themeComboBox->currentData().toString();
    applyTheme(theme);
    saveProgress();
}

void Antiprocrastinator::changeDuration(int minutes)
{
    m_pomodoroMinutes = minutes;
    if (!m_isRunning) {
        m_remainingTime = QTime(0, minutes, 0);
        updateDisplay();
    }
    saveProgress();
}
