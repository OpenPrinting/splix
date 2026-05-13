# SpliX Developer Wiki & Architecture Guide

Welcome to the SpliX Developer Wiki. This guide provides a comprehensive overview of the source code architecture to help developers understand what each code part does. Because SpliX is a reverse-engineered CUPS driver for Samsung's SPL2/SPLc/QPDL proprietary printer protocols, understanding the responsibilities of each module is critical for preventing regressions.

---

## 1. CUPS Entry Points (The Filters)

These files serve as the integration points between the standard CUPS printing subsystem and the internal SpliX engine.

* **`src/pstoqpdl.cpp`**: The PostScript wrapper. This executable intercepts PostScript jobs, applies Color Management System (CMS) profiles (for color matching), and pipes the job into Ghostscript (`gstoraster` or `pstoraster`), which then pipes the resulting raster image into `rastertoqpdl`.
* **`src/rastertoqpdl.cpp`**: The main CUPS filter. It receives standard CUPS raster data on `stdin` (from Ghostscript/macOS filter chains) and outputs raw QPDL (SPL) printer commands on `stdout` directly to the printer device.

---

## 2. Document & Page Management

These modules maintain the state of the print job as it passes through the filter.

* **`src/core.cpp`**: Contains the main execution loop (`Splix::Core::run()`). It reads CUPS page headers, instantiates the `Document` and `Page` objects, manages the thread pool for compression, and coordinates the entire raster-to-QPDL pipeline.
* **`src/document.cpp`**: Represents a single print job. It manages job-level metadata, the total page count, user-selected PPD options (like resolution, color depth, duplexing), and initializes the connection with the printer via `Printer` classes.
* **`src/page.cpp`**: Represents a single page in the document. It manages the raster pixel data received from CUPS, breaking it down into individual horizontal "bands" (chunks of lines) which are then dispatched to worker threads for compression.

---

## 3. Printer & Protocol Output (QPDL)

These files implement the generation of the proprietary printer commands.

* **`src/printer.cpp`**: The abstract interface and implementation for interacting with the hardware. It is responsible for injecting PJL (Printer Job Language) control headers and footers that wrap the QPDL byte-stream.
* **`src/qpdl.cpp`**: The heart of the protocol implementation. It formats the internal objects into the binary byte-stream expected by the printer. It generates the proprietary binary page headers, band headers, color-plane separations, and trailer bytes. **Warning:** Any byte misalignment or incorrect endianness here will cause the printer to reject the job or freeze.

---

## 4. Band Management & Color Separation

Because laser printers don't have enough RAM to process an entire page at once at high resolutions, the page is broken into horizontal strips (bands).

* **`src/band.cpp`**: Represents a single horizontal strip of the page. A `Band` orchestrates multiple `BandPlane` objects.
* **`src/bandplane.cpp`**: Represents a specific color channel (Cyan, Magenta, Yellow, Black) for a single band. This module applies dithering and separates the raw chunky CUPS pixels into planar formats suitable for compression.
* **`src/colors.cpp`**: Handles color conversion (e.g., converting CUPS RGB or CMYK spaces into the specific device-dependent CMYK values needed by Samsung engines).

---

## 5. Compression Algorithms

Samsung printers support several proprietary or standard compression algorithms to reduce the size of the data sent to the printer.

* **`src/compress.cpp`**: The abstract base class (`Compress`) and factory for all compression algorithms. It delegates the actual compression to the specific algorithm implementation based on the printer's capabilities (from the PPD).
* **`src/algo0x0d.cpp` (Algorithm 0x0D)**: A custom Run-Length Encoding (RLE) implementation used by older monochrome printers.
* **`src/algo0x0e.cpp` (Algorithm 0x0E)**: An advanced variation of RLE used by slightly newer printers.
* **`src/algo0x11.cpp` (Algorithm 0x11)**: A proprietary block-based Lempel-Ziv style compression algorithm heavily used by SPL2 color printers (e.g., CLP-300).
* **`src/algo0x13.cpp` (Algorithm 0x13)**: The standard JBIG compression wrapper used for high-end color and modern monochrome printers. It interfaces with `libjbig`.
* **`src/algo0x15.cpp` (Algorithm 0x15)**: Another variation of JBIG compression containing different header/band padding structures (e.g., used by CLP-600 series).

---

## 6. Utilities & Infrastructure

* **`src/cache.cpp`**: Manages a memory cache to avoid thrashing during band generation and compression.
* **`src/ppdfile.cpp`**: Parses the PostScript Printer Description (PPD) file provided by CUPS to extract the user's print settings (paper size, media type, duplexing, tray selection).
* **`src/sp_semaphore.cpp`**: A legacy wrapper that now wraps modern `std::counting_semaphore`. It coordinates the producer-consumer relationship between the CUPS raster reader (producer) and the compression worker threads (consumers).
* **`src/request.cpp`**: Represents an individual compression task requested by the producer to be picked up by the thread pool.

---

## 7. Data Flow Summary

1. CUPS executes `rastertoqpdl` and streams raster data into `stdin`.
2. `core.cpp` reads the CUPS header and initializes `document.cpp`.
3. `printer.cpp` writes the PJL initialization sequence to `stdout`.
4. `core.cpp` begins reading pixel rows, feeding them into `page.cpp`.
5. `page.cpp` buffers the rows until a full `band.cpp` is formed.
6. `band.cpp` separates the colors into `bandplane.cpp`s.
7. `core.cpp` queues the `bandplane.cpp` to be compressed.
8. A worker thread applies the correct `algo0x*.cpp` compression.
9. `qpdl.cpp` formats the compressed data with correct binary headers and writes it to `stdout`.
10. Once all pages are done, `printer.cpp` writes the PJL footer and exits.

---
Created during the Modernization Initiative, 2026.
