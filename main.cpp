#include "netsim.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QStyleFactory>
#include <QPalette>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Set Fusion style for consistent appearance across platforms
    a.setStyle(QStyleFactory::create("Fusion"));
    
    // Configure palette for light theme
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(240, 240, 240));  // Light gray background
    palette.setColor(QPalette::WindowText, Qt::black);          // Black text
    a.setPalette(palette);
    
    // Load translation for internationalization
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "NetworkSimulator_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    
    // Create and show main window
    NetSim w;
    w.show();
    
    // Start event loop
    return a.exec();
}