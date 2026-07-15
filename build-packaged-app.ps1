param(
  [string]$Destination = (Join-Path $PSScriptRoot "PackagedApp")
)

$ErrorActionPreference = "Stop"
$repoRoot = $PSScriptRoot

$directories = @(
  $Destination,
  (Join-Path $Destination "NewApp"),
  (Join-Path $Destination "SVG"),
  (Join-Path $Destination "QuestionPacks"),
  (Join-Path $Destination "Firmware")
)

foreach ($directory in $directories) {
  New-Item -ItemType Directory -Path $directory -Force | Out-Null
}

$files = @(
  @{ Source = "NewApp/IdM_Trivia2.html"; Target = "NewApp/IdM_Trivia2.html" },
  @{ Source = "SVG/Casco.svg"; Target = "SVG/Casco.svg" },
  @{ Source = "SVG/Tablero.svg"; Target = "SVG/Tablero.svg" },
  @{ Source = "SVG/board-map.json"; Target = "SVG/board-map.json" },
  @{ Source = "QuestionPacks/MujeresCiencia.idmquiz"; Target = "QuestionPacks/MujeresCiencia.idmquiz" },
  @{ Source = "LEDStrip_Animations_Serial_v1g_DiscreteLEDs_Flash6_fix3.ino"; Target = "Firmware/LEDStrip_Animations_Serial_v1g_DiscreteLEDs_Flash6_fix3.ino" },
  @{ Source = "Instructivo Trivia.pdf"; Target = "Instructivo Trivia.pdf" }
)

foreach ($file in $files) {
  $source = Join-Path $repoRoot $file.Source
  $target = Join-Path $Destination $file.Target
  if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw "Required package file is missing: $source"
  }
  Copy-Item -LiteralPath $source -Destination $target -Force
}

Write-Host "Packaged app refreshed at: $Destination"
