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
- **Python 3.8+** with pip
- **C++ compiler** (g++ on Linux/Mac, Visual Studio on Windows)
- **Git** (for cloning)

### Installation

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

