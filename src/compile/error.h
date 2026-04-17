#ifndef VARABLIZER_COMPILE_ERROR_H
#define VARABLIZER_COMPILE_ERROR_H

#include "source_location.h"
#include <stdexcept>
#include <string>

namespace compile
{
    struct CompileError : std::runtime_error
    {
        SourceLocation location;

        CompileError(const std::string& message, SourceLocation loc)
            : std::runtime_error(loc.to_string() + ": " + message)
            , location(std::move(loc))
        {}
    };

} // namespace compile

#endif // VARABLIZER_COMPILE_ERROR_H
