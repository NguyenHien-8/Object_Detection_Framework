#include "model_registry/json.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace odf::model::internal {
namespace {

class Parser {
public:
    explicit Parser(const std::string& source) : source_(source) {}

    Result<JsonValue> parse() {
        skipWhitespace();
        auto value = parseValue(0);
        if (!value.ok()) {
            return value.status();
        }
        skipWhitespace();
        if (position_ != source_.size()) {
            return failure("unexpected trailing data");
        }
        return value;
    }

private:
    static constexpr std::size_t kMaxDepth = 128;

    Result<JsonValue> failure(const std::string& message) const {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "JSON parse error at byte " + std::to_string(position_) +
                                 ": " + message);
    }

    void skipWhitespace() {
        while (position_ < source_.size() &&
               std::isspace(static_cast<unsigned char>(source_[position_])) != 0) {
            ++position_;
        }
    }

    Result<JsonValue> parseValue(std::size_t depth) {
        if (depth > kMaxDepth) {
            return failure("maximum nesting depth exceeded");
        }
        skipWhitespace();
        if (position_ >= source_.size()) {
            return failure("unexpected end of input");
        }
        switch (source_[position_]) {
            case '{': return parseObject(depth + 1U);
            case '[': return parseArray(depth + 1U);
            case '"': return parseStringValue();
            case 't': return parseLiteral("true", true);
            case 'f': return parseLiteral("false", false);
            case 'n': return parseNull();
            default: return parseNumber();
        }
    }

    Result<JsonValue> parseObject(std::size_t depth) {
        ++position_;
        JsonValue result;
        result.type = JsonValue::Type::Object;
        skipWhitespace();
        if (consume('}')) {
            return result;
        }
        while (true) {
            skipWhitespace();
            auto key = parseString();
            if (!key.ok()) {
                return key.status();
            }
            skipWhitespace();
            if (!consume(':')) {
                return failure("expected ':' after object key");
            }
            auto value = parseValue(depth);
            if (!value.ok()) {
                return value.status();
            }
            if (!result.object.emplace(key.takeValue(), value.takeValue()).second) {
                return failure("duplicate object key");
            }
            skipWhitespace();
            if (consume('}')) {
                break;
            }
            if (!consume(',')) {
                return failure("expected ',' or '}' in object");
            }
        }
        return result;
    }

    Result<JsonValue> parseArray(std::size_t depth) {
        ++position_;
        JsonValue result;
        result.type = JsonValue::Type::Array;
        skipWhitespace();
        if (consume(']')) {
            return result;
        }
        while (true) {
            auto value = parseValue(depth);
            if (!value.ok()) {
                return value.status();
            }
            result.array.push_back(value.takeValue());
            skipWhitespace();
            if (consume(']')) {
                break;
            }
            if (!consume(',')) {
                return failure("expected ',' or ']' in array");
            }
        }
        return result;
    }

    Result<std::string> parseString() {
        if (!consume('"')) {
            return Status::error(ErrorCode::InvalidModelConfig,
                                 "JSON parse error at byte " +
                                     std::to_string(position_) + ": expected string");
        }
        std::string result;
        while (position_ < source_.size()) {
            const char current = source_[position_++];
            if (current == '"') {
                return result;
            }
            if (static_cast<unsigned char>(current) < 0x20U) {
                return Status::error(ErrorCode::InvalidModelConfig,
                                     "JSON string contains an unescaped control character");
            }
            if (current != '\\') {
                result.push_back(current);
                continue;
            }
            if (position_ >= source_.size()) {
                return Status::error(ErrorCode::InvalidModelConfig,
                                     "JSON string ends after an escape character");
            }
            const char escaped = source_[position_++];
            switch (escaped) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                default:
                    return Status::error(ErrorCode::InvalidModelConfig,
                                         "JSON contains an unsupported escape sequence");
            }
        }
        return Status::error(ErrorCode::InvalidModelConfig, "unterminated JSON string");
    }

    Result<JsonValue> parseStringValue() {
        auto parsed = parseString();
        if (!parsed.ok()) {
            return parsed.status();
        }
        JsonValue result;
        result.type = JsonValue::Type::String;
        result.string = parsed.takeValue();
        return result;
    }

    Result<JsonValue> parseNumber() {
        const std::size_t begin = position_;
        if (position_ < source_.size() && source_[position_] == '-') {
            ++position_;
        }
        if (position_ >= source_.size()) {
            return failure("invalid number");
        }
        if (source_[position_] == '0') {
            ++position_;
        } else if (std::isdigit(static_cast<unsigned char>(source_[position_])) != 0) {
            while (position_ < source_.size() &&
                   std::isdigit(static_cast<unsigned char>(source_[position_])) != 0) {
                ++position_;
            }
        } else {
            return failure("invalid value");
        }
        if (position_ < source_.size() && source_[position_] == '.') {
            ++position_;
            const auto fractionBegin = position_;
            while (position_ < source_.size() &&
                   std::isdigit(static_cast<unsigned char>(source_[position_])) != 0) {
                ++position_;
            }
            if (fractionBegin == position_) {
                return failure("fraction has no digits");
            }
        }
        if (position_ < source_.size() &&
            (source_[position_] == 'e' || source_[position_] == 'E')) {
            ++position_;
            if (position_ < source_.size() &&
                (source_[position_] == '+' || source_[position_] == '-')) {
                ++position_;
            }
            const auto exponentBegin = position_;
            while (position_ < source_.size() &&
                   std::isdigit(static_cast<unsigned char>(source_[position_])) != 0) {
                ++position_;
            }
            if (exponentBegin == position_) {
                return failure("exponent has no digits");
            }
        }
        const std::string token = source_.substr(begin, position_ - begin);
        char* end = nullptr;
        const double number = std::strtod(token.c_str(), &end);
        if (end != token.c_str() + token.size() || !std::isfinite(number)) {
            return failure("number is invalid or outside the supported range");
        }
        JsonValue result;
        result.type = JsonValue::Type::Number;
        result.number = number;
        return result;
    }

    Result<JsonValue> parseLiteral(const char* literal, bool value) {
        const std::string expected(literal);
        if (source_.compare(position_, expected.size(), expected) != 0) {
            return failure("invalid literal");
        }
        position_ += expected.size();
        JsonValue result;
        result.type = JsonValue::Type::Boolean;
        result.boolean = value;
        return result;
    }

    Result<JsonValue> parseNull() {
        if (source_.compare(position_, 4, "null") != 0) {
            return failure("invalid literal");
        }
        position_ += 4;
        return JsonValue{};
    }

    bool consume(char expected) {
        if (position_ < source_.size() && source_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    const std::string& source_;
    std::size_t position_{0};
};

}  // namespace

const JsonValue* JsonValue::find(const std::string& key) const noexcept {
    if (type != Type::Object) {
        return nullptr;
    }
    const auto iterator = object.find(key);
    return iterator == object.end() ? nullptr : &iterator->second;
}

Result<JsonValue> parseJson(const std::string& source) { return Parser(source).parse(); }

}  // namespace odf::model::internal

