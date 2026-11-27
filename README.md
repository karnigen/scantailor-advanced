# ScanTailor Advanced

## Description

ScanTailor is an interactive post-processing tool for scanned pages. 
It performs operations such as:
  - [page splitting](https://github.com/scantailor/scantailor/wiki/Split-Pages), 
  - [deskewing](https://github.com/scantailor/scantailor/wiki/Deskew), 
  - [adding/removing borders](https://github.com/scantailor/scantailor/wiki/Page-Layout), 
  - [selecting content](https://github.com/scantailor/scantailor/wiki/Select-Content) 
  - ... and others. 
  
You give it raw scans, and you get pages ready to be printed or assembled into a PDF 
or [DjVu](http://elpa.gnu.org/packages/djvu.html) file. Scanning, optical character recognition, 
and assembling multi-page documents are out of scope of this project.

## Current workflow

Process scans interactively through page splitting, deskewing, content selection, page layout, and output. Automatic results can be reviewed and adjusted page by page, or processed in batches. Scanning, OCR, and assembling multi-page documents are outside the scope of this project.

## Building

For build requirements, package workflows, and local instructions, see [Building and packaging](docs/building.md). For test instructions, see [TESTING.md](TESTING.md).

## Documentation

- [Project and feature history](docs/history.md)
- [Project format and architecture documentation overview](docs/project-format-and-architecture-overview.md)
- [ScanTailor project XML keys: format 3 vs. format 4](docs/scan-tailor-project-keys.md)

## License

This software is licensed under GNU GPLv3, you can read more about it on our [LICENSE](/LICENSE) file.
