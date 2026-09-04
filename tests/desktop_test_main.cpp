#include "bridge/QtImageBridge.hpp"
#include "widgets/ClassFilterPanel.hpp"

#include <QApplication>
#include <QColor>
#include <QSet>

#include <iostream>

int main(int argc, char** argv) {
    QApplication application(argc, argv);
    odf::desktop::ClassFilterPanel filter;
    filter.setClasses({QStringLiteral("person"), QStringLiteral("bus"),
                       QStringLiteral("car")}, true);
    if (filter.selectedClasses().size() != 3) {
        std::cerr << "select-all class state is incorrect\n";
        return 1;
    }
    filter.setSelectedClasses({});
    if (!filter.selectedClasses().isEmpty()) {
        std::cerr << "clear-all class state is incorrect\n";
        return 1;
    }
    filter.setSelectedClasses(QSet<int>{1});
    if (filter.selectedClasses() != QSet<int>{1}) {
        std::cerr << "individual class state is incorrect\n";
        return 1;
    }

    odf::image::Image bgr;
    bgr.width = 1;
    bgr.height = 1;
    bgr.format = odf::image::PixelFormat::Bgr8;
    bgr.pixels = {10, 20, 30};
    auto image = odf::desktop::toQImage(bgr);
    if (!image.ok() || image.value().pixelColor(0, 0) != QColor(30, 20, 10)) {
        std::cerr << "ODF-to-QImage BGR conversion is incorrect\n";
        return 1;
    }
    std::cout << "desktop class-filter and image-bridge tests passed\n";
    return 0;
}

