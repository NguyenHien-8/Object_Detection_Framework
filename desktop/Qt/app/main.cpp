#include "app/ApplicationBootstrap.hpp"
#include "odf/app/logging_options.hpp"

#include <QApplication>
#include <QMessageBox>

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    std::vector<std::string> arguments(argv, argv + argc);
    auto parsed = odf::app::parseCommandLine(arguments);
    if (!parsed.ok()) {
        QApplication application(argc, argv);
        QMessageBox::critical(nullptr, QStringLiteral("ODF command line"),
                              QString::fromStdString(parsed.status().message()));
        return 2;
    }
    if (parsed.value().help) {
        std::cout << odf::app::commandLineUsage(arguments[0]) << '\n';
        return 0;
    }
    const auto logStatus = odf::app::applyLogLevel(parsed.value().logLevel);
    if (!logStatus.ok()) {
        QApplication application(argc, argv);
        QMessageBox::critical(nullptr, QStringLiteral("ODF command line"),
                              QString::fromStdString(logStatus.message()));
        return 2;
    }

    QApplication application(argc, argv);
    application.setOrganizationName(QStringLiteral("ODF"));
    application.setApplicationName(QStringLiteral("Object Detection Framework"));
    application.setApplicationVersion(QStringLiteral("0.1.0"));
    odf::desktop::ApplicationBootstrap bootstrap(parsed.takeValue());
    return bootstrap.run(application);
}
