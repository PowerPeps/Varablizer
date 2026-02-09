# Artisan - Varablizer Code Generator

Code generation CLI tool for Varablizer project.

## Installation

Build with CMake (automatic with main project):

```bash
cmake --build build --target artisan
```

Executable: `build/artisan.exe` (moved to build root for easy access)

## Commands

### make:opcode

Create a new opcode handler.

**Usage:**
```bash
./artisan make:opcode <NAME> <ADDRESS> <GROUP>
```

**Arguments:**
- `NAME`: Opcode name (e.g., `ADD`, `CUSTOM_OP`)
- `ADDRESS`: Hex address with 0x prefix (e.g., `0x05`, `0x1C`)
- `GROUP`: Group name (e.g., `MATH`, `CONTROL`, `MISC`)

**Examples:**
```bash
# Create a new math operation
./artisan make:opcode MUL2 0x1C MATH

# Create a custom opcode in MISC group
./artisan make:opcode CUSTOM 0x1D MISC

# Create opcode in new group (creates new file)
./artisan make:opcode SYSCALL 0x1E SYSTEM
```

**Features:**
✅ Validates address format (must be 0xNN)
✅ Checks for name conflicts
✅ Checks for address conflicts
✅ Inserts in address order
✅ Creates new group file if needed
✅ Auto-generates handler stub

**Output:**
- Updates `src/execute/opcodes/{group}.h` with new handler
- Handler is auto-discovered by opcode_generator on next build

---

### build:execute

Build the Varablizer execute engine.

**Usage:**
```bash
./artisan build:execute [--clean]
```

**Options:**
- `--clean`: Clean build directory before building

**Examples:**
```bash
# Build the project
./artisan build:execute

# Clean and rebuild
./artisan build:execute --clean
```

---

### execute

Execute a compiled .vbin binary file.

**Usage:**
```bash
./artisan execute <file.vbin>
```

**Arguments:**
- `file.vbin`: Path to the binary file to execute

**Examples:**
```bash
# Execute a binary file
./artisan execute example.vbin

# Execute with relative path
./artisan execute ./programs/test.vbin
```

**Note:** Requires `varablizer.exe` to be built first (run `artisan build:execute`)

## Adding New Commands

1. Create `commands/your_command.h` implementing `Command` interface
2. Register in `main.cpp`:
   ```cpp
   commands["your:command"] = std::make_unique<YourCommand>(project_root);
   ```

## Architecture

```
tools/artisan/
├── main.cpp                    # Entry point + command router
├── commands/
│   ├── command.h               # Base interface
│   └── make_opcode.h           # make:opcode implementation
└── utils/
    └── opcode_scanner.h        # Opcode conflict detection
```

**Extensible design**: easy to add new generators (e.g., `make:test`, `make:validator`, etc.)
