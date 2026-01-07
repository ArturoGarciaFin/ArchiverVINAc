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
```

## 🚀 Usage

The general syntax for the program is:

```bash
./vinac <flag> <archive_name.vc> [files...]
```

### 1. Insert Files (Archiving)
You can add files to the archive using two modes:

* **`-ip` (Insert Plain):** Stores the file as-is, without compression.
* **`-ic` (Insert Compressed):** Attempts to compress the file using LZ. If the compressed version is larger than the original, it falls back to plain storage.

```bash
# Example: Insert 'photo.png' into 'backup.vc' using compression
./vinac -ic backup.vc photo.png

# Example: Insert multiple files without compression
./vinac -ip backup.vc file1.txt file2.txt
```

> **Note:** If a file with the same name already exists inside the archive, it will be replaced.

### 2. List Contents
Displays the list of files currently stored inside the archive.

```bash
./vinac -c backup.vc
```

### 3. Extract Files
Extracts files from the archive to the current directory.

* Specify filenames to extract specific members.
* Leave the file list empty to **extract all files**.

```bash
# Extract only 'file1.txt'
./vinac -x backup.vc file1.txt

# Extract everything inside the archive
./vinac -x backup.vc
```

### 4. Remove Files
Permanently deletes specific files from inside the archive.

```bash
./vinac -r backup.vc file_to_remove.txt
```

### 5. Move / Reorder Files
Changes the order of files inside the binary archive. This moves the first file to the position immediately **AFTER** the second file.

```bash
# Moves 'file1' to the position immediately after 'file2'
./vinac -m backup.vc file1 file2
```

## 📝 Credits

* **Author:** Arturo Garcia Fin
* **Compression Logic:** Utilizes an external LZ compression implementation provided for the course.