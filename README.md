# AetherFM

**AetherFM** is a lightweight, modern file manager built entirely with **Pure GTK+ 3.0**. It is designed to be a fast, dependency-free alternative to Thunar, mimicking its core functionality and aesthetic without requiring the full Xfce desktop environment libraries.

## Features

- **Pure GTK+**: Built with standard GTK+ 3.0 and GLib, ensuring broad compatibility and minimal dependencies.
- **Fast Navigation**: Double-click to enter directories, use the "Up" button, or type paths directly.
- **Places Sidebar**: Quick access to your Home directory, file system root, and other mounted drives.
- **Visual Feedback**: Correct file icons based on MIME types.

## Prerequisites

To build AetherFM, you need a C compiler and the GTK+ 3.0 development libraries.

*   **GCC** or **Clang**
*   **CMake** (3.10 or higher)
*   **GTK+ 3.0** (`libgtk-3-dev` on Debian/Ubuntu, `gtk3-devel` on Fedora)

## Installation & Build

1.  **Clone the repository** (if you haven't already):
    ```bash
    git clone https://github.com/yourusername/aetherfm.git
    cd aetherfm
    ```

2.  **Create a build directory**:
    ```bash
    mkdir build
    cd build
    ```

3.  **Configure and Compile**:
    ```bash
    cmake ..
    make
    ```

## Usage

Once built, you can run the file manager directly from the build directory:

```bash
./aetherfm
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is open source and available under the [MIT License](LICENSE).
