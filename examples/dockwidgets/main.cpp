/*
  This file is part of KDDockWidgets.

  Copyright (C) 2018-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MyWidget.h"
#include "MyMainWindow.h"
#include "MyFrameworkWidgetFactory.h"

#include <kddockwidgets/Config.h>

#include <QStyleFactory>
#include <QApplication>
#include <QDebug>
#include <QCommandLineParser>

using namespace KDDockWidgets;

int main(int argc, char **argv)
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);

    app.setOrganizationName(QStringLiteral("KDAB"));
    app.setApplicationName(QStringLiteral("Test app"));

    QCommandLineParser parser;
    parser.setApplicationDescription("KDDockWidgets example application");
    parser.addHelpOption();

    qApp->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QCommandLineOption customStyle("p", QCoreApplication::translate("main", "Shows how to style framework internals via FrameworkWidgetFactory"));
    parser.addOption(customStyle);

    QCommandLineOption reorderTabsOption("r", QCoreApplication::translate("main", "Support re-ordering tabs with mouse"));
    parser.addOption(reorderTabsOption);

    QCommandLineOption noTitleBars("t", QCoreApplication::translate("main", "Never show titlebars"));
    parser.addOption(noTitleBars);

    QCommandLineOption lazyResizeOption("l", QCoreApplication::translate("main", "Use lazy resize"));
    parser.addOption(lazyResizeOption);

    QCommandLineOption multipleMainWindows("m", QCoreApplication::translate("main", "Shows two multiple main windows"));
    parser.addOption(multipleMainWindows);

    QCommandLineOption incompatibleMainWindows("i", QCoreApplication::translate("main", "Only usable with -m. Make the two main windows incompatible with each other. (Illustrates (MainWindowBase::setAffinityName))"));
    parser.addOption(incompatibleMainWindows);

    QCommandLineOption tabsHaveCloseButton("c", QCoreApplication::translate("main", "Tabs have a close button"));
    parser.addOption(tabsHaveCloseButton);

    QCommandLineOption nonClosableDockWidget("n", QCoreApplication::translate("main", "DockWidget #0 will be non-closable"));
    parser.addOption(nonClosableDockWidget);

    QCommandLineOption relativeRestore("s", QCoreApplication::translate("main", "Don't restore main window geometry, restore dock widgets in relative sizes"));
    parser.addOption(relativeRestore);

    QCommandLineOption doubleClickMaximize("x", QCoreApplication::translate("main", "Double clicking a title bar will maximize a floating window"));
    parser.addOption(doubleClickMaximize);

    QCommandLineOption nonDockable("d", QCoreApplication::translate("main", "DockWidget #9 will be non-dockable"));
    parser.addOption(nonDockable);

    QCommandLineOption maximizeButton("b", QCoreApplication::translate("main", "DockWidgets have maximize/restore buttons instead of float/dock button"));
    parser.addOption(maximizeButton);

    parser.addPositionalArgument("savedlayout", QCoreApplication::translate("main", "loads the specified json file at startup"));

#ifdef KDDOCKWIDGETS_SUPPORTS_NESTED_MAINWINDOWS
    QCommandLineOption dockableMainWindows("j", QCoreApplication::translate("main", "Allow main windows to be docked inside other main windows"));
    parser.addOption(dockableMainWindows);
#endif

    QCommandLineOption maxSizeOption("g", QCoreApplication::translate("main", "Make dock #8 have a max-size of 200x200."));
    parser.addOption(maxSizeOption);

    QCommandLineOption centralFrame("f", QCoreApplication::translate("main", "Persistent central frame"));
#if defined(DOCKS_DEVELOPER_MODE)
    parser.addOption(centralFrame);
#else
    Q_UNUSED(centralFrame)
#endif

    parser.process(app);

    if (parser.isSet(customStyle)) {
        Config::self().setFrameworkWidgetFactory(new CustomWidgetFactory()); // Sets our custom factory

        // Increase the separator size, just for demo
        Config::self().setSeparatorThickness(10);
    }

    MainWindowOptions options = MainWindowOption_None;
#if defined(DOCKS_DEVELOPER_MODE)
    options = parser.isSet(centralFrame) ? MainWindowOption_HasCentralFrame
                                         : MainWindowOption_None;
#endif

    auto flags = KDDockWidgets::Config::self().flags();
    if (parser.isSet(noTitleBars))
        flags |= KDDockWidgets::Config::Flag_HideTitleBarWhenTabsVisible | KDDockWidgets::Config::Flag_AlwaysShowTabs;

    if (parser.isSet(reorderTabsOption))
        flags |= KDDockWidgets::Config::Flag_AllowReorderTabs;

    if (parser.isSet(maximizeButton))
        flags |= KDDockWidgets::Config::Flag_TitleBarHasMaximizeButton;

    if (parser.isSet(lazyResizeOption))
        flags |= KDDockWidgets::Config::Flag_LazyResize;

    if (parser.isSet(tabsHaveCloseButton))
        flags |= KDDockWidgets::Config::Flag_TabsHaveCloseButton;


    if (parser.isSet(doubleClickMaximize))
        flags |= KDDockWidgets::Config::Flag_DoubleClickMaximizes;

    if (parser.isSet(incompatibleMainWindows) && !parser.isSet(multipleMainWindows)) {
        qWarning() << "Error: Argument -i requires -m";
        return 1;
    }

    KDDockWidgets::Config::self().setFlags(flags);

    const bool nonClosableDockWidget0 = parser.isSet(nonClosableDockWidget);
    const bool restoreIsRelative = parser.isSet(relativeRestore);
    const bool nonDockableDockWidget9 = parser.isSet(nonDockable);
    const bool maxSizeForDockWidget8 = parser.isSet(maxSizeOption);
    const bool usesMainWindowsWithAffinity = parser.isSet(multipleMainWindows);

#ifdef KDDOCKWIDGETS_SUPPORTS_NESTED_MAINWINDOWS
    const bool usesDockableMainWindows = parser.isSet(dockableMainWindows);
#else
    const bool usesDockableMainWindows = false;
#endif

    MyMainWindow mainWindow(QStringLiteral("MyMainWindow"), options, nonClosableDockWidget0,
                            nonDockableDockWidget9, restoreIsRelative, maxSizeForDockWidget8);
    mainWindow.setWindowTitle("Main Window 1");
    mainWindow.resize(1200, 1200);
    mainWindow.show();

    if (usesMainWindowsWithAffinity) {
        if (usesDockableMainWindows) {
            qWarning() << "MainWindows with affinity option is incompatible with Dockable Main Windows option";
            return 1;
        }

        // By default a dock widget can dock into any main window.
        // By setting an affinity name we can prevent that. Dock widgets of different affinities are incompatible.
        const QString affinity = parser.isSet(incompatibleMainWindows) ? QStringLiteral("affinity1")
                                                                       : QString();

        auto mainWindow2 = new MyMainWindow(QStringLiteral("MyMainWindow-2"), options,
                                            nonClosableDockWidget0, nonDockableDockWidget9,
                                            restoreIsRelative, maxSizeForDockWidget8, affinity);
        if (affinity.isEmpty())
            mainWindow2->setWindowTitle("Main Window 2");
        else
            mainWindow2->setWindowTitle("Main Window 2 (different affinity)");

        mainWindow2->resize(1200, 1200);
        mainWindow2->show();
    } else if (usesDockableMainWindows) {
        auto mainWindowDockWidget = new DockWidget(QStringLiteral("MyMainWindow-2-DW"));

        const QString affinity = QStringLiteral("Inner-DockWidgets-2");
        auto dockableMainWindow = new MyMainWindow(QStringLiteral("MyMainWindow-2"), options,
                                                   false, false, restoreIsRelative, false, affinity);

        dockableMainWindow->setAffinities({ affinity });

        dockableMainWindow->setStyleSheet(QStringLiteral("background: yellow"));

        dockableMainWindow->setWindowTitle("Dockable Main Window");
        dockableMainWindow->show();
        mainWindowDockWidget->setWidget(dockableMainWindow);
        mainWindowDockWidget->QWidget::show();
        mainWindowDockWidget->QWidget::resize(800, 800);
    }

    const QStringList args = parser.positionalArguments();
    if (args.size() >= 1) {
        const QString sourceJsonFileName = args[0];
        KDDockWidgets::LayoutSaver loader;
        if (!loader.restoreFromFile(sourceJsonFileName)) {
            qWarning() << Q_FUNC_INFO << "Failed to restore from" << sourceJsonFileName;
            return 1;
        }
    }


    return app.exec();
}
