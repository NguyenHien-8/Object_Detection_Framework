#pragma once

#include "odf/app/cli_options.hpp"

class QApplication;

namespace odf::desktop {

class ApplicationBootstrap final {
public:
    explicit ApplicationBootstrap(app::CommandLineOptions commandLine);
    int run(QApplication& application);

private:
    app::CommandLineOptions commandLine_;
};

}  // namespace odf::desktop

