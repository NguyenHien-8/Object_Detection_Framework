#include "app/ApplicationBootstrap.hpp"

#include "controllers/AppController.hpp"
#include "odf/app/model_root.hpp"
#include "services/SettingsService.hpp"
#include "widgets/MainWindow.hpp"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

#include <filesystem>
#include <utility>

namespace odf::desktop {

ApplicationBootstrap::ApplicationBootstrap(app::CommandLineOptions commandLine)
    : commandLine_(std::move(commandLine)) {}

int ApplicationBootstrap::run(QApplication& application) {
    MainWindow window;
    window.show();

    const auto executableDirectory =
        std::filesystem::path(QCoreApplication::applicationDirPath().toStdWString());
    auto modelRoot = app::discoverModelRoot(
        {commandLine_.models, app::modelRootFromEnvironment(), executableDirectory,
         app::compiledInstalledModelRoot()});
    SettingsService settingsService;
    const auto savedSettings = settingsService.load();
    if (!modelRoot.ok() && !commandLine_.models && !app::modelRootFromEnvironment() &&
        !savedSettings.modelRootOverride.isEmpty()) {
        modelRoot = app::discoverModelRoot(
            {std::filesystem::path(savedSettings.modelRootOverride.toStdWString()),
             std::nullopt, executableDirectory, app::compiledInstalledModelRoot()});
    }
    if (!modelRoot.ok()) {
        window.setStatus(QStringLiteral("Model registry unavailable"),
                         QString::fromStdString(modelRoot.status().message()), true);
        const QString selectedRoot = QFileDialog::getExistingDirectory(
            &window, QStringLiteral("Select ODF model registry root"));
        if (!selectedRoot.isEmpty()) {
            modelRoot = app::discoverModelRoot(
                {std::filesystem::path(selectedRoot.toStdWString()), std::nullopt,
                 executableDirectory, app::compiledInstalledModelRoot()});
        }
        if (!modelRoot.ok()) {
            QMessageBox::warning(&window, QStringLiteral("ODF model registry"),
                                 QString::fromStdString(modelRoot.status().message()));
            return application.exec();
        }
    }

    AppController controller(window, modelRoot.takeValue(), commandLine_);
    const auto status = controller.initialize();
    if (!status.ok()) {
        window.setStatus(QStringLiteral("Initialization failed"),
                         QString::fromStdString(status.message()), true);
        QMessageBox::warning(&window, QStringLiteral("ODF initialization"),
                             QString::fromStdString(status.message()));
    }
    bool smokeTimeoutValid = false;
    const int smokeTimeout =
        qEnvironmentVariableIntValue("ODF_DESKTOP_SMOKE_EXIT_MS", &smokeTimeoutValid);
    if (commandLine_.smokeTest || (smokeTimeoutValid && smokeTimeout > 0)) {
        QTimer::singleShot(commandLine_.smokeTest ? 1200 : smokeTimeout,
                           &window, &QWidget::close);
    }
    return application.exec();
}

}  // namespace odf::desktop
