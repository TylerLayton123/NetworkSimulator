#include "netsim_classes.h"
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
    
    // Configure palette for light theme with all necessary color roles
    QPalette palette;
    // Main window background
    palette.setColor(QPalette::Window, QColor(240, 240, 240));
    // Window text (for labels, etc.)
    palette.setColor(QPalette::WindowText, Qt::black);
    
    // Button colors (used for menus, toolbars, buttons)
    palette.setColor(QPalette::Button, QColor(240, 240, 240));
    palette.setColor(QPalette::ButtonText, Qt::black);
    
    // Base colors (for text input backgrounds)
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Text, Qt::black);
    
    // Highlight colors (for selection)
    palette.setColor(QPalette::Highlight, QColor(100, 149, 237));  // Cornflower blue
    palette.setColor(QPalette::HighlightedText, Qt::white);
    
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