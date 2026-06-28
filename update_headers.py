import os
import re

# Caminhos dos ficheiros
VERSION_FILE = "src/hal/Version.h"
SRC_DIR = "src"

def get_current_version():
    with open(VERSION_FILE, "r", encoding="utf-8") as f:
        content = f.read()
        # Procura por #define SIGMA_FIRMWARE_VERSION "X.X.X"
        match = re.search(r'#define\s+SIGMA_FIRMWARE_VERSION\s+"([^"]+)"', content)
        if match:
            return match.group(1)
    return None

def update_file_header(file_path, version):
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()
    
    # Substitui a linha da versão mantendo o alinhamento do manual CCS
    pattern = r'//\s+Versao\s+:\s+\d+\.\d+\.\d+\.\d+'
    replacement = f'//  Versao     : {version}'
    
    new_content = re.sub(pattern, replacement, content)
    
    # Se houver alteração, grava o ficheiro
    if new_content != content:
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(new_content)
        print(f"  [SCRIPT] Cabeçalho atualizado: {file_path} -> v{version}")

def main():
    version = get_current_version()
    if not version:
        print("  [SCRIPT] [ERRO] Versão não encontrada em Version.h")
        return

    # Percorre todas as pastas do projeto à procura de ficheiros de código
    for root, _, files in os.walk(SRC_DIR):
        for file in files:
            if file.endswith((".cpp", ".h")):
                update_file_header(os.path.join(root, file), version)

if __name__ == "__main__":
    main()