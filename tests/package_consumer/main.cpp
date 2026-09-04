#include "odf/model/model_registry.hpp"
#include "odf/version.hpp"

int main() {
    odf::model::ModelRegistry registry;
    return odf::kVersionMajor == 0 && registry.size() == 0U ? 0 : 1;
}

