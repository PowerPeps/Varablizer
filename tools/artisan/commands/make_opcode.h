#ifndef ARTISAN_MAKE_OPCODE_H
#define ARTISAN_MAKE_OPCODE_H

#include "command.h"
#include "../utils/opcode_scanner.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace artisan
{
    class MakeOpcodeCommand : public Command
    {
    public:
        explicit MakeOpcodeCommand(const fs::path& project_root)
            : project_root_(project_root)
            , opcodes_dir_(project_root / "src" / "execute" / "opcodes")
        {}

        [[nodiscard]] std::string name() const override
        {
            return "make:opcode";
        }

        [[nodiscard]] std::string description() const override
        {
            return "Create a new opcode handler";
        }

        [[nodiscard]] std::string usage() const override
        {
            return "artisan make:opcode <NAME> <ADDRESS> <GROUP>\n"
                   "  NAME    : Opcode name (e.g., ADD, CUSTOM_OP)\n"
                   "  ADDRESS : Hex address (e.g., 0x05, 0x1C)\n"
                   "  GROUP   : Group name (e.g., MATH, CONTROL, MISC)";
        }

        int execute(const std::vector<std::string>& args) override
        {
            if (args.size() != 3)
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m Invalid number of arguments\n\n";
                std::cerr << "Usage: " << usage() << "\n";
                return 1;
            }

            const std::string& name = args[0];
            const std::string& address_str = args[1];
            const std::string& group = args[2];

            if (!validate_address_format(address_str))
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m Address must be in format 0xNN\n";
                return 1;
            }

            std::uint8_t address = parse_address(address_str);

            OpcodeScanner scanner(opcodes_dir_);

            if (scanner.name_exists(name))
            {
                auto* existing = scanner.get_by_name(name);
                std::cerr << "\033[1;31m[ERROR]\033[0m Opcode name '" << name << "' already exists\n";
                std::cerr << "   Found in: " << existing->file << " at address 0x"
                         << std::hex << std::uppercase << (int)existing->address << "\n";
                return 1;
            }

            if (scanner.address_exists(address))
            {
                auto* existing = scanner.get_by_address(address);
                std::cerr << "\033[1;31m[ERROR]\033[0m Address 0x" << std::hex << std::uppercase
                         << (int)address << " already used by opcode '" << existing->name << "'\n";
                std::cerr << "   Found in: " << existing->file << "\n";
                return 1;
            }

            std::string group_lower = to_lowercase(group);
            fs::path target_file = opcodes_dir_ / (group_lower + ".h");

            std::string opcode_code = generate_opcode_code(name, address, group);

            if (fs::exists(target_file))
            {
                insert_into_existing_file(target_file, opcode_code, address, scanner, group);
            }
            else
            {
                create_new_file(target_file, opcode_code, group);
            }

            std::cout << "\033[1;32m[OK]\033[0m Opcode '" << name << "' created successfully!\n";
            std::cout << "   File: " << target_file.filename() << "\n";
            std::cout << "   Address: 0x" << std::hex << std::uppercase << (int)address << "\n";
            std::cout << "   Group: " << group << "\n";
            std::cout << "\nNext steps:\n";
            std::cout << "   1. Implement the handler body in " << target_file.filename() << "\n";
            std::cout << "   2. Rebuild the project (the opcode will be auto-registered)\n";
            std::cout.flush();

            return 0;
        }

    private:
        static bool validate_address_format(const std::string& addr)
        {
            if (addr.size() < 3 || addr.substr(0, 2) != "0x")
                return false;

            for (size_t i = 2; i < addr.size(); ++i)
            {
                if (!std::isxdigit(addr[i]))
                    return false;
            }
            return true;
        }

        static std::uint8_t parse_address(const std::string& addr)
        {
            return static_cast<std::uint8_t>(std::stoul(addr, nullptr, 16));
        }

        static std::string to_lowercase(const std::string& str)
        {
            std::string result = str;
            std::transform(result.begin(), result.end(), result.begin(),
                          [](unsigned char c) { return std::tolower(c); });
            return result;
        }

        static std::string generate_opcode_code(const std::string& name, std::uint8_t address,
                                        const std::string& group)
        {
            std::ostringstream oss;
            oss << "/* [[Opcode::" << name << ", 0x" << std::hex << std::uppercase
                << (int)address << ", Group::" << group << "]] */\n";
            oss << "static void h_" << to_lowercase(name) << "(assembly& vm) noexcept\n";
            oss << "{\n";
            oss << "    // TODO: Implement " << name << " opcode\n";
            oss << "    (void)vm;\n";
            oss << "}\n";
            return oss.str();
        }

        static void insert_into_existing_file(const fs::path& file, const std::string& code,
                                       std::uint8_t new_address, const OpcodeScanner& scanner,
                                       const std::string& group)
        {
            std::ifstream in(file);
            std::string content((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
            in.close();

            auto group_opcodes = scanner.get_by_group(group);
            std::sort(group_opcodes.begin(), group_opcodes.end(),
                     [](const auto& a, const auto& b) { return a.address < b.address; });

            // Find insertion point (before first opcode with higher address)
            std::regex pattern(R"(/\*\s*\[\[Opcode::(\w+),\s*0x([0-9A-Fa-f]+),\s*Group::(\w+)\]\]\s*\*/\s*static\s+void\s+h_\w+\(.*?\)\s*noexcept\s*\{(?:[^{}]|\{[^}]*\})*\})");

            std::smatch match;
            std::string::const_iterator search_start(content.cbegin());
            size_t insert_pos = std::string::npos;
            size_t last_opcode_end = 0;

            while (std::regex_search(search_start, content.cend(), match, pattern))
            {
                std::uint8_t addr = static_cast<std::uint8_t>(
                    std::stoul(match[2].str(), nullptr, 16)
                );
                std::string grp = match[3].str();

                if (grp == group)
                {
                    last_opcode_end = match.position(0) + match.length(0);

                    if (addr > new_address && insert_pos == std::string::npos)
                    {
                        insert_pos = match.position(0);
                        break;
                    }
                }

                search_start = match.suffix().first;
            }

            if (insert_pos == std::string::npos && last_opcode_end > 0)
            {
                insert_pos = last_opcode_end;
                while (insert_pos < content.size() && std::isspace(content[insert_pos])) insert_pos++;
            }

            if (insert_pos == std::string::npos)
            {
                size_t endif_pos = content.rfind("#endif");
                if (endif_pos != std::string::npos)
                {
                    while (endif_pos > 0 && content[endif_pos - 1] != '\n') endif_pos--;
                    insert_pos = endif_pos;
                }
            }

            if (insert_pos != std::string::npos)
            {
                content.insert(insert_pos, "\n" + code);
            }
            else
            {
                size_t pos = content.rfind("#endif");
                if (pos != std::string::npos)
                    content.insert(pos, code + "\n");
            }

            std::ofstream out(file);
            out << content;
        }

        static void create_new_file(const fs::path& file, const std::string& code,
                            const std::string& group)
        {
            std::string group_upper = group;
            std::transform(group_upper.begin(), group_upper.end(), group_upper.begin(),
                          [](unsigned char c) { return std::toupper(c); });

            std::ofstream out(file);
            out << "// Opcode handlers for " << group_upper << " group\n";
            out << "// This file is meant to be included INSIDE assembly.h in the private section\n\n";
            out << "#if ENABLE_" << group_upper << "\n\n";
            out << code << "\n";
            out << "#endif // ENABLE_" << group_upper << "\n";

            std::cout << "\033[1;36m[INFO]\033[0m Created new file: " << file.filename() << "\n";
            std::cout << "\033[1;33m[WARNING]\033[0m Remember to:\n";
            std::cout << "   1. Add #include \"opcodes/" << file.filename().string()
                     << "\" in assembly.h\n";
            std::cout << "   2. Add ENABLE_" << group_upper << " to opcodes_config.h\n";
            std::cout << "   3. Add the file to CMakeLists.txt OPCODE_HEADERS\n";
        }

        fs::path project_root_;
        fs::path opcodes_dir_;
    };

} // namespace artisan

#endif // ARTISAN_MAKE_OPCODE_H
