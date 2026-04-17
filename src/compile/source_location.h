#ifndef VARABLIZER_SOURCE_LOCATION_H
#define VARABLIZER_SOURCE_LOCATION_H

#include <cstdint>
#include <string>

namespace compile
{
    struct SourceLocation
    {
        std::string  file;
        std::uint32_t line = 1;
        std::uint32_t col  = 1;

        [[nodiscard]] std::string to_string() const
        {
            return file + ":" + std::to_string(line) + ":" + std::to_string(col);
        }
    };

} // namespace compile

#endif // VARABLIZER_SOURCE_LOCATION_H
