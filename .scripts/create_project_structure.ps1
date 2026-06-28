$folders = @(
    "src\application",
    "src\system",
    "src\drivers\mpu6050",
    "src\drivers\ds18b20",
    "src\drivers\watchdog",
    "src\drivers\led",
    "src\storage\system",
    "src\storage\config",
    "src\storage\calibration",
    "src\storage\baseline",
    "src\monitor\analytics",
    "src\monitor\baseline",
    "src\monitor\alarms",
    "src\communication\serial",
    "src\communication\confirmation",
    "src\communication\progress",
    "src\reports",
    "src\ui\calibration",
    "src\ui\settings",
    "src\ui\baseline",
    "src\config",
    "src\types"
)

$folders | ForEach-Object {
    New-Item -ItemType Directory -Path $_ -Force | Out-Null
}

Write-Host "Estrutura de diretórios criada com sucesso!"