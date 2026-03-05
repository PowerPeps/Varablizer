//
// Created by PowerPeps on 12/02/2026.
//

#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>

class Sentence
{
public:
    virtual ~Sentence() = default;
    virtual void resolve() = 0;
};

enum class SentenceType
{
    control,
    defaultFunction,
    ASM,

};

class World
{
protected:
    std::string word;
    SentenceType type;

public:
    explicit World(std::string w, SentenceType t)
        : word(std::move(w)), type(t) {}

    virtual ~World() = default;

    [[nodiscard]] virtual bool match(const std::string& str) const = 0;

    [[nodiscard]] const std::string& getWord() const noexcept
    {
        return word;
    }
};

class Control : public World
{
    public:
        std::basic_regex<> regular{};

};

static const std::unordered_map<std::string, World> Worlds = {
    {"if", World("if", SentenceType::control)},
    {"else", World("else", SentenceType::control)},

    {"do", World("do", SentenceType::control)},
    {"else", World("else", SentenceType::control)},
    {"else", World("else", SentenceType::control)},

    {"pf", World("pf", SentenceType::defaultFunction)},
    {"p", World("p", SentenceType::defaultFunction)},
    {"dd", World("dd", SentenceType::defaultFunction)},

    {"nop", World("nop", SentenceType::ASM)},

    {"", World("", SentenceType::control)},
    {"", World("", SentenceType::control)},
    {"", World("", SentenceType::control)},
    {"", World("", SentenceType::control)},
    {"", World("", SentenceType::control)},

};


class Comment final : public Sentence
{
private:
    std::string value;

    static bool isValid(const std::string& str)
    {
        if (str.size() >= 2 && str.rfind("//", 0) == 0) return true;

        if (str.size() >= 4 &&
            str.rfind("/*", 0) == 0 &&
            str.substr(str.size() - 2) == "*/")
            return true;

        return false;
    }

public:
    explicit Comment(const std::string& str) : value(str)
    {
        if (!isValid(str)) throw std::invalid_argument("Invalid comment format");
    }

    [[nodiscard]] const std::string& str() const noexcept
    {
        return value;
    }

    void resolve() override {}
};

class Function : public Sentence {};

class Variable : public Sentence {};

class NameSpace : public Sentence {};

class Use : public Sentence {};