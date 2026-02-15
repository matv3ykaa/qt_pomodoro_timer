#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>
#include "headers/antiprocrastinator.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (QStyleFactory::keys().contains("Fusion")) {
        app.setStyle(QStyleFactory::create("Fusion"));
    }

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(245, 247, 250));
    palette.setColor(QPalette::WindowText, QColor(44, 62, 80));
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::AlternateBase, QColor(240, 240, 240));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::black);
    palette.setColor(QPalette::Button, QColor(230, 230, 230));
    palette.setColor(QPalette::ButtonText, QColor(44, 62, 80));
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Highlight, QColor(52, 152, 219));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(palette);

    Antiprocrastinator window;
    window.show();

    return app.exec();
}
