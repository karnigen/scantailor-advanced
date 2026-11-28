param(
  [Parameter(Mandatory = $true, Position = 0)]
  [string]$PdfPath,

  [Parameter(Position = 1)]
  [string]$OutputDirectory,

  [ValidateRange(1, 2400)]
  [int]$Dpi = 300,

  [ValidateRange(0, 99999)]
  [int]$FirstPage = 0,

  [ValidateRange(0, 99999)]
  [int]$LastPage = 0
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
if ($FirstPage -gt 0 -and $LastPage -gt 0 -and $FirstPage -gt $LastPage) {
  throw 'FirstPage must not be greater than LastPage.'
}
$PdfPath = (Resolve-Path -LiteralPath $PdfPath).Path
$pdfDirectory = [System.IO.Path]::GetDirectoryName($PdfPath)
$fileStem = [System.IO.Path]::GetFileNameWithoutExtension($PdfPath)
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
  $OutputDirectory = $pdfDirectory
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path

$pdfToCairo = Resolve-PopplerTool $env:PDFTOCAIRO 'pdftocairo.exe'
$tempDirectory = Join-Path $OutputDirectory (".{0}.render.{1}" -f $fileStem, [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tempDirectory | Out-Null
$movedFiles = [System.Collections.Generic.List[string]]::new()
$committed = $false

try {
  $arguments = @('-tiff', '-tiffcompression', 'lzw', '-r', "$Dpi")
  if ($FirstPage -gt 0) { $arguments += @('-f', "$FirstPage") }
  if ($LastPage -gt 0) { $arguments += @('-l', "$LastPage") }
  $arguments += @($PdfPath, (Join-Path $tempDirectory $fileStem))
  & $pdfToCairo @arguments
  if ($LASTEXITCODE -ne 0) {
    throw "pdftocairo failed with exit code $LASTEXITCODE."
  }

  $images = @(Get-ChildItem -LiteralPath $tempDirectory -File | Where-Object { $_.Extension -in '.tif', '.tiff' })
  if ($images.Count -eq 0) {
    throw 'Poppler did not produce any TIFF pages.'
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
  $committed = $true
  Write-Output "Rendered $($images.Count) page(s) at $Dpi DPI to $OutputDirectory"
}
finally {
  if (-not $committed) {
    foreach ($movedFile in $movedFiles) {
      Remove-Item -LiteralPath $movedFile -Force -ErrorAction SilentlyContinue
    }
  }
  Remove-Item -LiteralPath $tempDirectory -Recurse -Force -ErrorAction SilentlyContinue
}
