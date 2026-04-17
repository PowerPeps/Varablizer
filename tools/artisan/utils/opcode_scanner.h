#ifndef ARTISAN_OPCODE_SCANNER_H
#define ARTISAN_OPCODE_SCANNER_H

#include <string>
#include <vector>
#include <map>
#include <regex>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace artisan
{
    struct OpcodeEntry
    {
        std::string name;
        std::uint8_t address;
        std::string group;
        std::string file;
    };

    class OpcodeScanner
    {
    public:
        explicit OpcodeScanner(const fs::path& opcodes_dir)
            : opcodes_dir_(opcodes_dir)
        {
            scan();
        }


        bool name_exists(const std::string& name) const
        {
            return opcodes_by_name_.find(name) != opcodes_by_name_.end();
        }


        bool address_exists(std::uint8_t address) const
        {
            return opcodes_by_address_.find(address) != opcodes_by_address_.end();
        }


        const OpcodeEntry* get_by_name(const std::string& name) const
        {
            auto it = opcodes_by_name_.find(name);
            return it != opcodes_by_name_.end() ? &it->second : nullptr;
        }


        const OpcodeEntry* get_by_address(std::uint8_t address) const
        {
            auto it = opcodes_by_address_.find(address);
            return it != opcodes_by_address_.end() ? &it->second : nullptr;
        }


        [[nodiscard]] std::vector<OpcodeEntry> get_by_group(const std::string& group) const
        {
            std::vector<OpcodeEntry> result;
            for (const auto& [addr, entry] : opcodes_by_address_)
            {
                if (entry.group == group)
                    result.push_back(entry);
            }
            return result;
        }

    private:
        void scan()
        {
            if (!fs::exists(opcodes_dir_) || !fs::is_directory(opcodes_dir_))
                return;

            // Regex: /* [[Opcode::NAME, 0xADDR, Group::GROUP]] */
            std::regex pattern(
                R"(/\*\s*\[\[Opcode::(\w+),\s*0x([0-9A-Fa-f]+),\s*Group::(\w+)\]\]\s*\*/)"
            );

            for (const auto& entry : fs::directory_iterator(opcodes_dir_))
            {
                if (entry.path().extension() != ".h")
                    continue;

                std::ifstream file(entry.path());
                if (!file)
                    continue;

                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());

                std::smatch match;
                std::string::const_iterator search_start(content.cbegin());

                while (std::regex_search(search_start, content.cend(), match, pattern))
                {
                    OpcodeEntry opcode;
                    opcode.name = match[1].str();
                    opcode.address = static_cast<std::uint8_t>(
                        std::stoul(match[2].str(), nullptr, 16)
                    );
                    opcode.group = match[3].str();
                    opcode.file = entry.path().filename().string();

                    opcodes_by_name_[opcode.name] = opcode;
                    opcodes_by_address_[opcode.address] = opcode;

                    search_start = match.suffix().first;
                }
            }
        }

        fs::path opcodes_dir_;
        std::map<std::string, OpcodeEntry> opcodes_by_name_;
        std::map<std::uint8_t, OpcodeEntry> opcodes_by_address_;
    };

} // namespace artisan

#endif // ARTISAN_OPCODE_SCANNER_H
