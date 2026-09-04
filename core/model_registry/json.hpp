#pragma once

#include "odf/status.hpp"

#include <map>
#include <string>
#include <vector>

namespace odf::model::internal {

class JsonValue {
public:
    enum class Type { Null, Boolean, Number, String, Array, Object };

    Type type{Type::Null};
    bool boolean{false};
    double number{0.0};
    std::string string;
    std::vector<JsonValue> array;
    std::map<std::string, JsonValue> object;

    [[nodiscard]] const JsonValue* find(const std::string& key) const noexcept;
};

Result<JsonValue> parseJson(const std::string& source);

}  // namespace odf::model::internal

