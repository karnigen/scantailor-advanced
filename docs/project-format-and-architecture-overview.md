# Project Format and Architecture Documentation Overview

Status: initial research overview

## Scope

This document records the first inventory for two topics:

1. Comparison of ScanTailor project format versions 3 and 4.
2. Documentation of the application's architecture and image-processing algorithms.

This is an overview, not a final format specification or algorithm reference.

## Project format: initial findings

ScanTailor project files are XML documents, not JSON documents.

The project file is written by `ProjectWriter` and read by `ProjectReader`. The root element is
`<project>`. It contains the project version, output and layout settings, directories, files,
images, pages, file-name disambiguation settings, and filter settings.

The format version is stored in `<project version="...">` and is defined by `PROJECT_VERSION`
in `version.h.in`. Both the available source copy and the upstream-style source copy currently
define it as version 4.

Historical source tags confirm the version numbering:

- Tag `v1.0.19` defines `PROJECT_VERSION 3`.
- Tag `v1.1.1` also defines `PROJECT_VERSION 3`.
- Tag `v1.2.0` changes `PROJECT_VERSION` from 3 to 4.

Therefore, project files written by releases 1.0.19 and 1.1.1 can be treated as format 3.
The tag search confirms the transition to format 4 at 1.2.0, rather than at 1.1.1.

One format-3 implementation change appears between `v1.0.19` and `v1.1.1`: the 1.1.1 writer
stores paths relative to the project-file directory when possible, and the reader resolves those
paths relative to the project file. The 1.0.19 implementation still stored and consumed the
original path representation. This is a relevant compatibility characteristic even though both
versions retain project format number 3.

The current reader accepts versions from `MIN_READABLE_PROJECT_VERSION` through `PROJECT_VERSION`
(currently 3 through 4). Opening a v3 project does not modify it: the UI reports known defaults
and compatibility changes, and saving upgrades it to format 4 after preserving the original in a
unique backup. Default-parameter profiles use the same readable version range; newly written
profiles use the current version.

This is a compatibility reader and safe-save path, not a general-purpose XML transformation
layer. The key-level document describes the supported defaults and legacy deskew behavior.

For a key-level comparison of format 3 and format 4, see
[ScanTailor project XML keys](scan-tailor-project-keys.md).

## Format 3 vs. 4: what cannot yet be concluded

No tracked `.ScanTailor` project sample in format 3 or 4 was found in the working trees or in
the initial Git file searches.

Consequently, the source review does not prove every structural difference between versions 3 and
4 or cover every real-world project variant.

Before claiming a version 3/4 diff, collect:

- One or more real v3 project files.
- One or more real v4 project files with the same source images and filter settings.
- Source or tags containing the code that introduced project format 4.
- A normalized comparison that removes paths and machine-dependent values.

## Suggested format-comparison method

1. Preserve original files and record their `version`, source application, and source commit/tag.
2. Create a small fixture project and save it with v3 and v4 binaries, if available.
3. Parse both files as XML and compare:
   - root attributes;
   - element and attribute names;
   - path representation;
   - image and page references;
   - filter settings;
   - selected page and layout direction;
   - file-name disambiguation data.
4. Diff normalized XML rather than raw text.
5. Confirm findings against `ProjectReader.cpp`, `ProjectWriter.cpp`, and filter `saveSettings` /
   `loadSettings` implementations.

## Existing architecture anchors

The source tree is divided into the following main layers:

- `foundation`: general-purpose containers, numerics, property sets, XML marshalling, and helpers.
- `math`: geometric and matrix operations.
- `imageproc`: image processing primitives.
- `dewarping`: distortion models and dewarping algorithms.
- `core`: project model, stage sequencing, task processing, caches, UI base classes, and filters.
- `app`: Qt application shell, project and settings dialogs, and main window orchestration.

The processing pipeline is represented by ordered filters:

1. Fix orientation.
2. Split pages.
3. Deskew.
4. Select content.
5. Adjust page layout.
6. Output processing.

The core filter modules implement the shared `AbstractFilter` interface for saving settings,
loading settings, generating tasks, and providing UI and cache behavior.

## Existing processing architecture

The main processing concepts to document are:

- `StageSequence` and `ProjectPages`: define the visible stages and page sequence.
- `AbstractFilter`: defines filter integration points.
- `FilterData`, cache-driven tasks, and worker pools: connect UI and background processing.
- `ProcessingTaskQueue` and `BackgroundTask`: coordinate asynchronous work.
- `PageId`, `ImageId`, `PageInfo`, and `ImageMetadata`: identify and describe inputs.
- `ProjectReader` / `ProjectWriter`: persist project state in XML.
- `OutputGenerator` and related classes: transform processed pages into output images/files.

## Algorithm documentation priorities

Start with a short per-stage document and a single pipeline overview:

1. Page loading and metadata validation.
2. Fix orientation and image trimming.
3. Page splitting and page-layout detection.
4. Deskew and oblique correction.
5. Content selection and zone editing.
6. Margins and page layout.
7. Dewarping.
8. Binarization, despeckling, and output generation.
9. TIFF/JPEG/PNG output and naming.

For each algorithm document, record:

- purpose and user-visible behavior;
- inputs and outputs;
- main entry points;
- relevant classes and source files;
- defaults and apply-to propagation;
- cache and background-task behavior;
- public parameters and units;
- tests or fixtures;
- known limitations and open questions.

## Recommended documentation structure

The initial structure should remain small and evolve into detailed documents:

- `docs/architecture.md`: process, modules, major objects, and runtime flow.
- `docs/project-format.md`: XML format schema, versions, migration, and compatibility.
- `docs/processing-pipeline.md`: stage flow and shared processing infrastructure.
- `docs/algorithms/`: one short document per processing stage.
- `docs/development.md`: build, test, debugging, and validation workflow.

## Next decisions

Choose two inputs before starting the detailed format work:

- Which v3 and v4 project files are available for comparison?
- Which repository branch, tag, or commit corresponds to the v3 and v4 writers?

After that, the first practical deliverables should be:

1. A normalized v3/v4 XML diff.
2. A format-compatibility report.
3. A pipeline/architecture diagram.
4. One detailed algorithm page for the highest-priority stage.
