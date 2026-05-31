import os
from pathlib import Path
import re
import sys
import logging  

logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
command_headers_dir = 'include/ChatCommands/Commands'
generated_file_path = 'src/GeneratedCommands.cpp'
class_or_struct_pattern = re.compile(r'(class|struct)\s+(\w+)')
register_cmd_pattern = re.compile(r'REGISTER_CMD\((\w+),.*\)')

def generate_includes():
    generated_commands_path = Path(generated_file_path)
    if not generated_commands_path.is_file():
        logging.error(f"{generated_file_path} doesn't exist. "
                      "Note: this file should be inside the MainServer project (in src)")
        sys.exit(1)

    if not os.path.isdir(command_headers_dir):
        logging.error(f"{command_headers_dir} not found or is not a directory. "
                      "Note: Commands should be written inside the include/ChatCommands/Commands folder")
        sys.exit(1)

    headers = sorted(
        f for f in os.listdir(command_headers_dir)
        if os.path.isfile(os.path.join(command_headers_dir, f)) and f.endswith('.h')
    )

    new_content = "".join(
        f'#include "../{command_headers_dir}/{header}"\n' for header in headers
    )

    existing_content = generated_commands_path.read_text() if generated_commands_path.is_file() else None
    if existing_content == new_content:
        logging.info(f"{generated_file_path} already up to date ({len(headers)} headers)")
        return

    generated_commands_path.write_text(new_content)
    logging.info(f"Generated includes for {len(headers)} headers in {generated_file_path}")

def ensure_commands_registration():
    headers = [
        f for f in os.listdir(command_headers_dir)
        if os.path.isfile(os.path.join(command_headers_dir, f)) and f.endswith('.h')
    ]
    class_map = {}  
    register_commands = set()  
    
    for current_header in headers:
        file = os.path.join(command_headers_dir, current_header)
        with open(file) as f:
            lines = list(f)
        
        for i, line in enumerate(lines):
            for match in re.finditer(class_or_struct_pattern, line):
                class_name = match.group(2)
                skip = i > 0 and "//NO_CHECK" in lines[i - 1].strip()
                class_map[class_name] = skip
            for match in re.finditer(register_cmd_pattern, line):
                register_commands.add(match.group(1))

    errors_found = False
    for class_name, skip in class_map.items():
        if not skip and class_name not in register_commands:
            logging.error(f"Command {class_name} has not been registered with REGISTER_CMD and is not marked //NO_CHECK")
            errors_found = True
    
    if errors_found:
        sys.exit(1)

generate_includes()
ensure_commands_registration()
