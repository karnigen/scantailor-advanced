param(
  [Parameter(Mandatory = $true, Position = 0)]
  [string]$PdfPath,

  [Parameter(Position = 1)]
  [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'

function Resolve-PopplerTool([string]$ConfiguredPath, [string]$DefaultName) {
  $toolName = if ([string]::IsNullOrWhiteSpace($ConfiguredPath)) { $DefaultName } else { $ConfiguredPath }
  $tool = Get-Command -Name $toolName -ErrorAction SilentlyContinue
  if (-not $tool) {
    throw "Poppler $DefaultName was not found. Install Poppler, add it to PATH, or set the corresponding environment variable."
  }
  if ($tool.Source) { return $tool.Source }
  return $tool.Path
}

if (-not (Test-Path -LiteralPath $PdfPath -PathType Leaf)) {
  throw "PDF file does not exist: $PdfPath"
}
$PdfPath = (Resolve-Path -LiteralPath $PdfPath).Path
$pdfDirectory = [System.IO.Path]::GetDirectoryName($PdfPath)
$fileStem = [System.IO.Path]::GetFileNameWithoutExtension($PdfPath)
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
  $OutputDirectory = $pdfDirectory
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path

$pdfImages = Resolve-PopplerTool $env:PDFIMAGES 'pdfimages.exe'
$tempDirectory = Join-Path $OutputDirectory (".{0}.extract.{1}" -f $fileStem, [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tempDirectory | Out-Null
$movedFiles = [System.Collections.Generic.List[string]]::new()
$committed = $false

try {
  $imageInfoPath = Join-Path $tempDirectory 'image-info.txt'
  $imageInfo = & $pdfImages -list $PdfPath
  if ($LASTEXITCODE -ne 0) {
    throw "pdfimages -list failed with exit code $LASTEXITCODE."
  }
  $imageInfo | Set-Content -LiteralPath $imageInfoPath -Encoding utf8

  $prefix = Join-Path $tempDirectory $fileStem
  & $pdfImages -all $PdfPath $prefix
  if ($LASTEXITCODE -ne 0) {
    throw "pdfimages -all failed with exit code $LASTEXITCODE."
  }

  $images = @(Get-ChildItem -LiteralPath $tempDirectory -File | Where-Object { $_.Name -ne 'image-info.txt' })
  if ($images.Count -eq 0) {
    throw 'No embedded images were found. Use render-pages.ps1 for vector or text-based PDF pages.'
  }

  $infoDestination = Join-Path $OutputDirectory "$fileStem-image-info.txt"
  if (Test-Path -LiteralPath $infoDestination) {
    throw "Refusing to overwrite existing file: $infoDestination"
  }
  foreach ($image in $images) {
    $destination = Join-Path $OutputDirectory $image.Name
    if (Test-Path -LiteralPath $destination) {
      throw "Refusing to overwrite existing file: $destination"
    }
  }

  foreach ($image in $images) {
    $destination = Join-Path $OutputDirectory $image.Name
    Move-Item -LiteralPath $image.FullName -Destination $destination
    $movedFiles.Add($destination)
  }
  Move-Item -LiteralPath $imageInfoPath -Destination $infoDestination
  $movedFiles.Add($infoDestination)
  $committed = $true

  Write-Output "Extracted $($images.Count) embedded image(s) to $OutputDirectory"
  Write-Output "Image dimensions and effective page DPI: $infoDestination"
}
finally {
  if (-not $committed) {
    foreach ($movedFile in $movedFiles) {
      Remove-Item -LiteralPath $movedFile -Force -ErrorAction SilentlyContinue
    }
  }
  Remove-Item -LiteralPath $tempDirectory -Recurse -Force -ErrorAction SilentlyContinue
}
