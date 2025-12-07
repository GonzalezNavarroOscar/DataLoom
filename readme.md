# 📊 Data Loom

A professional-grade data validation and processing tool with a C++ backend for high-performance validation and a modern Python GUI for intuitive interaction.

## ✨ Features

### 🔧 Core Processing
- **High-performance C++ backend** for fast data validation
- **Comprehensive data validation** for ITE student records
- **Automatic error correction** for common data issues
- **Duplicate detection** for CURP and control numbers
- **Cross-field validation** ensuring data consistency
- **Auto-generated emails** based on control numbers
- **Standardized phone number formatting**
- **RFC and CURP validation** with Mexican standards

### 🖥️ User Interface
- **Modern Flet-based GUI** with responsive design
- **Real-time processing feedback** with progress indicators
- **Sample format viewer** for easy reference

### 📁 Data Support
- **CSV file processing** with automatic header detection
- **XLSX file processing** xls too

## 🚀 Quick Start

### Prerequisites

#### 🐍​ **Python 3.8+** with pip 

* **Windows**:
    * Download Python:

        * Go to python.org/downloads

        * Download Python 3.8+ (check "Add Python to PATH" during installation)

    * Verify installation:

    ```bash
    python --version
    pip --version
    ```

* **Linux** (Ubuntu/Debian):
    ```bash
    # Update package list
    sudo apt update

    # Install Python 3.8+ and pip
    sudo apt install python3 python3-pip -y

    # Verify
    python3 --version
    pip3 --version
    ```


#### 🖥️​ **C++ compiler** (g++ on Linux/Mac, Visual Studio on Windows)


* **Windows** (Visual Studio Build Tools):

    * **Go** to Visual Studio Downloads

    * **Download** "Build Tools for Visual Studio"

        * During installation, select:

        * "Desktop development with C++"

        * Include Windows 10/11 SDK

* **Linux** (g++):

    ```bash
    # Ubuntu/Debian
    sudo apt update
    sudo apt install build-essential -y

    # Verify
    g++ --version

    # RHEL/CentOS/Fedora
    sudo dnf install gcc-c++ -y
    # or
    sudo yum install gcc-c++ -y

    ```


#### 🔎​ **Git** (for cloning)

* **Windows**:
    * Download from git-scm.com:

    * Go to git-scm.com/downloads

    * Download Windows version and run installer

    * Recommended settings:

        * Use Git from the command line and also from 3rd-party software

        * Checkout Windows-style, commit Unix-style line endings

        * Use Windows' default console window

    * Verify installation:

        ```bash
        git --version
        ```

* **Linux** (Ubuntu/Debian):

    ```bash
    sudo apt update
    sudo apt install git -y

    # Verify
    git --version
    ```

### Installation of Data Loom

```bash
# 1. Clone the repository
git clone https://github.com/yourusername/data-processor.git
cd /the-place-you-clone-it

# 2. Install Python dependencies
pip install -r requirements.txt

# 3. Build the C++ processor
make

# 4. Run the GUI
python excel_processor_gui.py

```

### Inputing files

1. **Select** the input.xlsx from the same directory as the project.

2. **Click** the "Process file" button.

3. **Read** the logs.

4. **Verify** the output.xlsx file placed on the same directory as the project.

5. **Enjoy** the results.
