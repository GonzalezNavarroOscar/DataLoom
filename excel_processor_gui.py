# excel_processor_gui.py
import os
import pandas as pd
import tempfile
import subprocess
import flet as ft
import threading

class ExcelProcessorFlet:
    def __init__(self):
        self.input_file = ""
        self.output_file = ""
        self.cpp_executable = "./data_processor"
        self.is_processing = False
        self.validation_errors = []
        self.headers = []
        
    def main(self, page: ft.Page):
        self.page = page
        page.title = "Archive Processor - Flet GUI + C++"
        page.theme_mode = ft.ThemeMode.LIGHT
        page.padding = 20
        page.scroll = ft.ScrollMode.ADAPTIVE
        
        # UI Components
        self.setup_ui()
        
    def setup_ui(self):
        # Title
        title = ft.Text(
            "Archive Processor",
            size=24,
            weight=ft.FontWeight.BOLD,
            text_align=ft.TextAlign.CENTER
        )
        
        # File Selection Section
        self.file_info_text = ft.Text("No file selected", size=14)
        
        file_section = ft.Card(
            content=ft.Container(
                content=ft.Column([
                    ft.Text("File Selection", size=16, weight=ft.FontWeight.BOLD),
                    ft.Row([
                        self.file_info_text,
                        ft.ElevatedButton(
                            "Browse Excel File...",
                            icon=ft.Icons.FOLDER_OPEN,
                            on_click=self.browse_input_file
                        )
                    ], alignment=ft.MainAxisAlignment.SPACE_BETWEEN)
                ]),
                padding=15
            )
        )
        
        # Progress Section
        self.progress_bar = ft.ProgressBar(value=0, width=400)
        self.status_text = ft.Text("Ready to process", size=14)
        
        progress_section = ft.Column([
            self.status_text,
            self.progress_bar
        ])
        
        # Process Button
        self.process_btn = ft.ElevatedButton(
            "Process File",
            icon=ft.Icons.PLAY_ARROW,
            on_click=self.process_file,
            disabled=True
        )
        
        # Results Section
        self.results_text = ft.Text("", size=14, color=ft.Colors.BLUE)
        
        # Process Log Section
        self.process_log = ft.ListView(expand=True, height=300, spacing=5)
        
        process_log_section = ft.Card(
            content=ft.Container(
                content=ft.Column([
                    ft.Text("Validation Process Log", size=16, weight=ft.FontWeight.BOLD),
                    ft.Container(
                        content=self.process_log,
                        border=ft.border.all(1, ft.Colors.OUTLINE),
                        border_radius=5,
                        padding=10,
                        bgcolor=ft.Colors.GREY_50
                    )
                ]),
                padding=15
            )
        )
        
        # Layout
        self.page.add(
            ft.Column([
                title,
                file_section,
                progress_section,
                ft.Row([self.process_btn], alignment=ft.MainAxisAlignment.CENTER),
                self.results_text,
                process_log_section
            ], spacing=20)
        )
    
    def browse_input_file(self, e):
        def get_file_result(e: ft.FilePickerResultEvent):
            if e.files:
                self.input_file = e.files[0].path
                self.file_info_text.value = os.path.basename(self.input_file)
                input_dir = os.path.dirname(self.input_file)
                input_name = os.path.splitext(os.path.basename(self.input_file))[0]
                self.output_file = os.path.join(input_dir, f"processed_{input_name}.xlsx")
                
                self.process_btn.disabled = False
                self.add_log_message(f"Selected file: {os.path.basename(self.input_file)}", ft.Colors.BLUE)
                self.page.update()
        
        file_picker = None
        for control in self.page.overlay:
            if isinstance(control, ft.FilePicker):
                file_picker = control
                break
        
        if file_picker is None:
            file_picker = ft.FilePicker(on_result=get_file_result)
            self.page.overlay.append(file_picker)
            self.page.update()
        
        file_picker.pick_files(
            allowed_extensions=["xlsx", "xls", "csv"],
            dialog_title="Select Excel File"
        )
    
    def process_file(self, e):
        if not os.path.exists(self.cpp_executable):
            self.show_snackbar("Error: C++ processor executable not found!")
            return
        
        self.process_btn.disabled = True
        self.progress_bar.value = 0
        self.is_processing = True
        self.validation_errors = []
        self.process_log.controls.clear()
        self.add_log_message("Starting validation process...", ft.Colors.BLUE)
        
        # Run processing
        thread = threading.Thread(target=self.run_processing)
        thread.daemon = True
        thread.start()
    
    def run_processing(self):
        try:
            self.add_log_message(f"Processing file: {os.path.basename(self.input_file)}", ft.Colors.BLUE)
            self.update_progress(10, "Converting Excel to CSV...")
            self.add_log_message("Converting Excel to CSV format...")
            
            # Convert Excel to CSV
            csv_data = self.excel_to_csv(self.input_file)
            
            # Save to temporary CSV file
            self.update_progress(30, "Preparing data for C++...")
            self.add_log_message("Preparing data for validation...")
            with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
                input_csv_path = f.name
                f.write(csv_data)
            
            # Generate output file names - ONLY ONE OUTPUT FILE
            input_dir = os.path.dirname(self.input_file)
            input_name = os.path.splitext(os.path.basename(self.input_file))[0]
            
            # Only valid output file and log file
            valid_output_path = os.path.join(input_dir, f"output.csv")
            process_log_path = os.path.join(input_dir, f"process_log_{input_name}.txt")
            
            try:
                # C++ processor - now expects 3 arguments (input, valid_output, log_path)
                self.update_progress(50, "Processing with C++...")
                self.add_log_message("Running validation and auto-corrections...", ft.Colors.BLUE)
                
                # Pass only 3 arguments to C++ processor
                cmd = [
                    self.cpp_executable, 
                    input_csv_path,        # Input CSV
                    valid_output_path,     # Valid output CSV
                    process_log_path       # Log file
                ]
                # REMOVED: problematic_output_path from the arguments
                
                # REMOVED: No more processing options to pass
                
                self.add_log_message(f"Executing: {' '.join(cmd)}")
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
                
                if result.returncode != 0:
                    error_msg = f"C++ processing failed: {result.stderr}"
                    self.add_log_message(f"ERROR: {error_msg}", ft.Colors.RED)
                    raise Exception(error_msg)
                
                # Load process log
                self.load_process_log(process_log_path)
                
                self.update_progress(70, "Converting outputs to Excel...")
                self.add_log_message("Converting results to Excel format...")
                
                # Convert valid records to Excel
                if os.path.exists(valid_output_path):
                    with open(valid_output_path, 'r') as f:
                        valid_csv = f.read()
                    valid_excel_path = os.path.join(input_dir, f"output.xlsx")
                    self.csv_to_excel(valid_csv, valid_excel_path)
                    self.valid_output_file = valid_excel_path
                    self.add_log_message(f"Valid records saved: {os.path.basename(valid_excel_path)}", ft.Colors.GREEN)
                    
                    # Show record count
                    df = pd.read_excel(valid_excel_path)
                    total_records = len(df)
                    self.add_log_message(f"Total valid records: {total_records}", ft.Colors.GREEN)
                
                self.update_progress(100, "Processing completed!")
                self.add_log_message("Processing completed successfully!", ft.Colors.GREEN)
                self.processing_finished(True, f"Successfully processed {os.path.basename(self.input_file)}")
                
            finally:
                # Clean up temporary files
                for temp_file in [input_csv_path, process_log_path]:
                    if os.path.exists(temp_file):
                        os.unlink(temp_file)
                # Clean up temporary CSV files
                if os.path.exists(valid_output_path):
                    os.unlink(valid_output_path)
                        
        except Exception as e:
            self.add_log_message(f"Processing failed: {str(e)}", ft.Colors.RED)
            self.processing_finished(False, f"Processing failed: {str(e)}")

    def processing_finished(self, success, message):
        self.process_btn.disabled = False
        self.is_processing = False
        
        if success:
            self.results_text.value = f"{message}"
            self.results_text.color = ft.Colors.GREEN
            
            # Show file locations
            file_info = []
            if hasattr(self, 'valid_output_file') and os.path.exists(self.valid_output_file):
                size = os.path.getsize(self.valid_output_file) / (1024 * 1024)
                file_info.append(f"Valid records: {os.path.basename(self.valid_output_file)} ({size:.2f} MB)")
            
            if hasattr(self, 'problematic_output_file') and os.path.exists(self.problematic_output_file):
                size = os.path.getsize(self.problematic_output_file) / (1024 * 1024)
                file_info.append(f"Problematic records: {os.path.basename(self.problematic_output_file)} ({size:.2f} MB)")
            
            if file_info:
                self.add_log_message("\n".join(file_info))
            
            self.show_snackbar(message, ft.Colors.GREEN)
            
        else:
            self.results_text.value = f"{message}"
            self.results_text.color = ft.Colors.RED
            self.show_snackbar(message, ft.Colors.RED)
        
        self.page.update()

    def load_process_log(self, log_path):
        """Load and display the process log"""
        try:
            if os.path.exists(log_path):
                with open(log_path, 'r', encoding='utf-8') as f:
                    log_lines = f.readlines()
                
                self.add_log_message("Process Log:", ft.Colors.BLUE)
                for line in log_lines:
                    line = line.strip()
                    if line:
                        # Color code based on content
                        if "ERROR" in line or "Error" in line:
                            self.add_log_message(line, ft.Colors.RED)
                        elif "Auto-filled" in line or "Auto-corrected" in line or "Fixed" in line or "Cleaned" in line:
                            self.add_log_message(line, ft.Colors.PURPLE)
                        elif "Warning" in line:
                            self.add_log_message(line, ft.Colors.ORANGE)
                        elif "Success" in line or "Saved" in line:
                            self.add_log_message(line, ft.Colors.GREEN)
                        else:
                            self.add_log_message(line)
            else:
                self.add_log_message("No detailed process log available", ft.Colors.GREY_600)
                
        except Exception as e:
            self.add_log_message(f"Could not load process log: {e}", ft.Colors.RED)

    def add_log_message(self, message, color=None):
        """Add a message to the process log with optional color"""
        if color is None:
            color = ft.Colors.BLACK
        
        # Create text widget
        log_text = ft.Text(message, selectable=True, size=12, color=color)
        
        # Add to log
        self.process_log.controls.append(log_text)
        
        # Auto-scroll to bottom
        self.process_log.scroll_to(offset=-1, duration=100)
        self.page.update()
    
    def excel_to_csv(self, excel_path):
        """Convert Excel file to CSV format for C++ processing"""
        try:
            if excel_path.endswith('.xlsx'):
                df = pd.read_excel(excel_path, engine='openpyxl')
            else:
                df = pd.read_excel(excel_path, engine='xlrd')
            
            # Store headers for error display
            self.headers = df.columns.tolist()
            
            return df.to_csv(index=False)
            
        except Exception as e:
            raise Exception(f"Error reading Excel file: {str(e)}")
    
    def csv_to_excel(self, csv_data, output_path):
        """Convert CSV data back to Excel file"""
        try:
            # Read CSV data
            from io import StringIO
            df = pd.read_csv(StringIO(csv_data))
            
            # Save as Excel
            if output_path.endswith('.xlsx'):
                df.to_excel(output_path, index=False, engine='openpyxl')
            else:
                df.to_excel(output_path, index=False, engine='xlwt')
            
            return True
            
        except Exception as e:
            raise Exception(f"Error writing Excel file: {str(e)}")
    
    def update_progress(self, value, message):
        """Update progress from background thread"""
        self.progress_bar.value = value / 100
        self.status_text.value = message
        self.page.update()
    
    def log_message(self, message):
        """Add message to log from background thread"""
        self.results_text.value = message
        self.page.update()
    
    def show_snackbar(self, message, color=ft.Colors.BLUE):
        """Show snackbar message"""
        snackbar = ft.SnackBar(
            content=ft.Text(message),
            bgcolor=color,
            duration=5000
        )
        self.page.snack_bar = snackbar
        snackbar.open = True
        self.page.update()

def main():
    ft.app(target=ExcelProcessorFlet().main)

if __name__ == "__main__":
    main()