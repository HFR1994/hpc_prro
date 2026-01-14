# Raven Roosting Optimization Report

This directory contains a multi-file Typst document formatted to IEEE standards.

## Structure

- `main.typ` - Main document file that imports all sections
- `sections/` - Individual content sections
  - `abstract.typ` - Paper abstract
  - `introduction.typ` - Introduction and background
  - `serial_implementation.typ` - Serial algorithm description with pseudocode
  - `parallel_implementation.typ` - MPI parallel implementation details
  - `optimizations.typ` - Performance optimizations
  - `conclusion.typ` - Conclusions and future work
- `references.bib` - Bibliography file

## Compilation

To compile the document:

```bash
typst compile main.typ
```

This will generate `main.pdf`.

## Requirements

- Typst compiler (latest version)
- IEEE template package: `@preview/ieee:0.2.1`
- CeTZ package for diagrams: `@preview/cetz:0.3.2`

## Customization

Edit the author information in `main.typ` to include your actual details.
