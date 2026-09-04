#include "odf/model/model_registry.hpp"
#include "odf/version.hpp"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    const std::filesystem::path root = argc > 1 ? argv[1] : ODF_DEFAULT_MODEL_ROOT;
    odf::model::ModelRegistry registry;
    const auto status = registry.loadDirectory(root);
    if (!status.ok()) {
        std::cerr << odf::errorCodeName(status.code()) << ": " << status.message() << '\n';
        return 1;
    }
    std::cout << "Object Detection Framework " << odf::kVersionMajor << '.'
              << odf::kVersionMinor << '.' << odf::kVersionPatch << '\n';
    for (const auto& spec : registry.models()) {
        std::cout << spec.id << "\t" << odf::model::modelFamilyName(spec.family) << "\t"
                  << spec.input.width << 'x' << spec.input.height << "\t"
                  << (spec.deploymentValidated ? "validated" : "template") << '\n';
    }
    return 0;
}

