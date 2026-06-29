import os
import re

# Caminhos dos ficheiros
VERSION_FILE = "src/hal/Version.h"
SRC_DIR = "src"

def get_current_version():
    with open(VERSION_FILE, "r", encoding="utf-8") as f:
        content = f.read()
        match = re.search(
            r'#define\s+SIGMA_FIRMWARE_VERSION\s+"([^"]+)"', content)
        if match:
            return match.group(1)
    return None

def get_current_codename():
    with open(VERSION_FILE, "r", encoding="utf-8") as f:
        content = f.read()
        match = re.search(
            r'#define\s+SIGMA_CODENAME\s+"([^"]+)"', content)
        if match:
            return match.group(1)
    return None

def update_file_header(file_path, version, codename):
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    new_content = content

    # Atualiza a linha Versao
    pattern_ver = r'//\s+Versao\s+:\s+\d+\.\d+\.\d+\.\d+'
    replacement_ver = f'//  Versao     : {version}'
    new_content = re.sub(pattern_ver, replacement_ver, new_content)

    # Atualiza ou insere a linha Codename
    pattern_cod = r'//\s+Codename\s+:\s+.*'
    replacement_cod = f'//  Codename   : {codename}'
    if re.search(pattern_cod, new_content):
        new_content = re.sub(pattern_cod, replacement_cod, new_content)
    else:
        # Insere apos a linha Versao
        new_content = re.sub(
            rf'({replacement_ver})\n',
            rf'\1\n{replacement_cod}\n',
            new_content)

    if new_content != content:
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(new_content)
        print(f"  [SCRIPT] Atualizado: {file_path} -> "
              f"v{version} / {codename}")

def main():
    version = get_current_version()
    codename = get_current_codename()

    if not version:
        print("  [SCRIPT] [ERRO] Versao nao encontrada em Version.h")
        return
    if not codename:
        print("  [SCRIPT] [AVISO] Codename nao encontrado em Version.h")
        codename = ""

    for root, _, files in os.walk(SRC_DIR):
        for file in files:
            if file.endswith((".cpp", ".h")):
                update_file_header(
                    os.path.join(root, file), version, codename)

if __name__ == "__main__":
    main()
