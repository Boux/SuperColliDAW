#include "SuperColliderLanguage.h"

#include <string_view>

namespace supercollidaw {

namespace {

using Iterator = TextEditor::Iterator;
using Color = TextEditor::Color;

bool isIdentifierStart(ImWchar c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

bool isIdentifierChar(ImWchar c) { return isIdentifierStart(c) || (c >= '0' && c <= '9'); }

bool isDigit(ImWchar c) { return c >= '0' && c <= '9'; }

bool isHexDigit(ImWchar c) { return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }

bool isPunctuation(ImWchar c) { return std::u32string_view(U"!%&*+-/<=>?@^|~,;:.()[]{}#`").find(c) != std::u32string_view::npos; }

Iterator advance(Iterator i, size_t count) {
    while (count--)
        ++i;
    return i;
}

size_t remaining(Iterator i, Iterator end) { return i < end ? end - i : 0; }

ImWchar peek(Iterator i, Iterator end, size_t offset) { return offset < remaining(i, end) ? *advance(i, offset) : 0; }

Iterator skipWhile(Iterator i, Iterator end, bool (*predicate)(ImWchar)) {
    while (i < end && predicate(*i))
        ++i;
    return i;
}

Iterator identifierAfter(Iterator start, Iterator end) {
    if (!isIdentifierStart(peek(start, end, 0)))
        return start;
    return skipWhile(start, end, isIdentifierChar);
}

Iterator getIdentifier(Iterator start, Iterator end) {
    if (*start >= 'A' && *start <= 'Z')
        return start;
    return identifierAfter(start, end);
}

Iterator hexNumber(Iterator start, Iterator end) {
    const ImWchar x = peek(start, end, 1);
    if (*start != '0' || (x != 'x' && x != 'X'))
        return start;
    const Iterator digitsStart = advance(start, 2);
    const Iterator digitsEnd = skipWhile(digitsStart, end, isHexDigit);
    return digitsEnd == digitsStart ? start : digitsEnd;
}

Iterator exponent(Iterator i, Iterator end) {
    const ImWchar e = peek(i, end, 0);
    if (e != 'e' && e != 'E')
        return i;
    const ImWchar sign = peek(i, end, 1);
    const Iterator digitsStart = advance(i, sign == '+' || sign == '-' ? 2 : 1);
    const Iterator digitsEnd = skipWhile(digitsStart, end, isDigit);
    return digitsEnd == digitsStart ? i : digitsEnd;
}

Iterator piSuffix(Iterator i, Iterator end) {
    const bool isPi = peek(i, end, 0) == 'p' && peek(i, end, 1) == 'i' && !isIdentifierChar(peek(i, end, 2));
    return isPi ? advance(i, 2) : i;
}

Iterator getNumber(Iterator start, Iterator end) {
    const Iterator hex = hexNumber(start, end);
    if (hex != start)
        return hex;
    Iterator i = skipWhile(start, end, isDigit);
    if (i == start)
        return start;
    if (peek(i, end, 0) == '.' && isDigit(peek(i, end, 1)))
        i = skipWhile(advance(i, 1), end, isDigit);
    return piSuffix(exponent(i, end), end);
}

Iterator prefixedIdentifier(Iterator start, Iterator end, ImWchar prefix) {
    if (*start != prefix)
        return start;
    const Iterator nameStart = advance(start, 1);
    const Iterator nameEnd = identifierAfter(nameStart, end);
    return nameEnd == nameStart ? start : nameEnd;
}

Iterator characterLiteral(Iterator start, Iterator end) {
    if (*start != '$' || remaining(start, end) < 2)
        return start;
    const bool escaped = peek(start, end, 1) == '\\' && remaining(start, end) >= 3;
    return advance(start, escaped ? 3 : 2);
}

Iterator className(Iterator start, Iterator end) {
    if (!(*start >= 'A' && *start <= 'Z'))
        return start;
    return skipWhile(start, end, isIdentifierChar);
}

Iterator tokenize(Iterator start, Iterator end, Color& color) {
    const struct {
        Iterator end;
        Color color;
    } candidates[] = {
        { prefixedIdentifier(start, end, '\\'), Color::knownIdentifier },
        { prefixedIdentifier(start, end, '~'), Color::identifier },
        { characterLiteral(start, end), Color::string },
        { className(start, end), Color::declaration },
    };
    for (const auto& candidate : candidates) {
        if (candidate.end == start)
            continue;
        color = candidate.color;
        return candidate.end;
    }
    return start;
}

TextEditor::Language makeLanguage() {
    TextEditor::Language language;
    language.name = "SuperCollider";
    language.singleLineComment = "//";
    language.commentStart = "/*";
    language.commentEnd = "*/";
    language.hasSingleQuotedStrings = true;
    language.hasDoubleQuotedStrings = true;
    language.stringEscape = '\\';
    language.keywords = { "var", "arg", "classvar", "const", "this", "super", "nil", "true", "false", "inf", "pi",
        "thisProcess", "thisThread", "thisFunction", "thisFunctionDef", "thisMethod", "currentEnvironment", "topEnvironment" };
    language.isPunctuation = isPunctuation;
    language.getIdentifier = getIdentifier;
    language.getNumber = getNumber;
    language.customTokenizer = tokenize;
    return language;
}

}

const TextEditor::Language* superColliderLanguage() {
    static const TextEditor::Language language = makeLanguage();
    return &language;
}

}
