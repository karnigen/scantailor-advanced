# PDF image helpers

These scripts are optional maintainer/user helpers; ScanTailor Advanced does
not invoke external PDF tools.

## Extract embedded images

`extract-images.sh` works on Linux and macOS with Poppler `pdfimages`.
`extract-images.ps1` provides the equivalent command on Windows with
`pdfimages.exe`.

```bash
scripts/pdf/extract-images.sh input.pdf
scripts/pdf/extract-images.sh input.pdf /path/to/output
```

```powershell
.\scripts\pdf\extract-images.ps1 .\input.pdf
.\scripts\pdf\extract-images.ps1 .\input.pdf C:\path\to\output
```

The output directory defaults to the PDF's directory. Poppler extracts the
embedded image streams and keeps their original formats where possible. This
does not compose a full page: a page can contain multiple images, vector
graphics, and text. The accompanying `*-image-info.txt` report from
`pdfimages -list` records each image's dimensions and effective page DPI.
Existing output files are not overwritten.

Install Poppler using the package manager or distribution appropriate for the
platform, and make `pdfimages` available on `PATH`. Alternatively, set
`PDFIMAGES` to its executable path.

## Render complete pages

Use the render helper when the goal is one image per complete PDF page,
including vector graphics and text. It uses Poppler `pdftocairo` to write
LZW-compressed TIFFs. The default resolution is 300 DPI.

```bash
scripts/pdf/render-pages.sh input.pdf
scripts/pdf/render-pages.sh --dpi 300 input.pdf /path/to/output
scripts/pdf/render-pages.sh --first-page 1 --last-page 2 input.pdf /path/to/output
```

```powershell
.\scripts\pdf\render-pages.ps1 .\input.pdf
.\scripts\pdf\render-pages.ps1 .\input.pdf C:\path\to\output -Dpi 300
.\scripts\pdf\render-pages.ps1 .\input.pdf C:\path\to\output -FirstPage 1 -LastPage 2
```

The output directory defaults to the PDF's directory. Existing files are not
overwritten. Set `PDFTOCAIRO` to use a specific `pdftocairo` executable.
Rendering produces raster pages and does not preserve a searchable OCR text
layer.
