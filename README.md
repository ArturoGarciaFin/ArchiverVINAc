# VINAc - File Archiver Utility

**VINAc** is a command-line file archiving utility developed in **C**. It functions similarly to standard tools like `tar` or `zip`, allowing users to package multiple files into a single binary `.vc` archive.

The project focuses on **binary file manipulation**, memory management, and data structuring. It incorporates an external **LZ (Lempel-Ziv)** compression module to optimize storage, featuring an intelligent system that decides whether to store files compressed or raw based on efficiency.

> *Adapted from a Programming II course assignment.*

## ⚡ Key Features

* **Binary Archiving:** Custom implementation of file packing and unpacking logic.
* **Smart Compression Integration:** Uses an LZ compression algorithm. The system automatically compares the compressed size vs. the original size; if compression results in a larger file (negative compression), the file is stored in its raw format.
* **File Management:** Supports adding, removing, extracting, and reordering files within the binary archive.
* **Duplicate Handling:** Automatically detects if a file with the same name exists in the archive and updates it (removes the old, adds the new).

## 🛠 Compilation

To compile the project, ensure you have a C compiler (like `gcc`) installed. You can use the provided Makefile:

```bash
make