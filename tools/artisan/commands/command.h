#ifndef ARTISAN_COMMAND_H
#define ARTISAN_COMMAND_H

#include <string>
#include <vector>

namespace artisan
{
    class Command
    {
    public:
        virtual ~Command() = default;

        virtual int execute(const std::vector<std::string>& args) = 0;

        [[nodiscard]] virtual std::string name() const = 0;

        [[nodiscard]] virtual std::string description() const = 0;

        [[nodiscard]] virtual std::string usage() const = 0;
    };

} // namespace artisan

#endif // ARTISAN_COMMAND_H
