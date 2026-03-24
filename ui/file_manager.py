"""
File Manager
Handles file operations for the IDE
"""

import os
from pathlib import Path
from datetime import datetime


class FileManager:
    """Manages file operations"""
    
    def __init__(self):
        self.recent_files = []
        self.max_recent = 10
        self.load_recent_files()
    
    def create_new_file(self, content="", filename=None):
        """Create a new file"""
        if filename:
            path = Path(filename)
            path.write_text(content, encoding='utf-8')
            self.add_to_recent(str(path))
            return str(path)
        return None
    
    def read_file(self, filepath):
        """Read file content"""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                return f.read()
        except Exception as e:
            raise IOError(f"Cannot read file: {e}")
    
    def write_file(self, filepath, content):
        """Write content to file"""
        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
        except Exception as e:
            raise IOError(f"Cannot write file: {e}")
    
    def add_to_recent(self, filepath):
        """Add file to recent list"""
        filepath = str(filepath)
        if filepath in self.recent_files:
            self.recent_files.remove(filepath)
        self.recent_files.insert(0, filepath)
        self.recent_files = self.recent_files[:self.max_recent]
        self.save_recent_files()
    
    def get_recent_files(self):
        """Get list of recent files"""
        return [f for f in self.recent_files if os.path.exists(f)]
    
    def save_recent_files(self):
        """Save recent files list to config"""
        config_dir = Path.home() / '.compileride'
        config_dir.mkdir(exist_ok=True)
        
        recent_file = config_dir / 'recent.txt'
        with open(recent_file, 'w', encoding='utf-8') as f:
            for filepath in self.recent_files:
                if os.path.exists(filepath):
                    f.write(filepath + '\n')
    
    def load_recent_files(self):
        """Load recent files list from config"""
        config_dir = Path.home() / '.compileride'
        recent_file = config_dir / 'recent.txt'
        
        if recent_file.exists():
            try:
                with open(recent_file, 'r', encoding='utf-8') as f:
                    self.recent_files = [line.strip() for line in f if line.strip()]
            except:
                pass
    
    def get_file_info(self, filepath):
        """Get file information"""
        path = Path(filepath)
        if not path.exists():
            return None
        
        stat = path.stat()
        return {
            'path': str(path),
            'name': path.name,
            'size': stat.st_size,
            'modified': datetime.fromtimestamp(stat.st_mtime),
            'lines': len(open(filepath, 'r', encoding='utf-8').readlines())
        }
    
    def create_backup(self, filepath):
        """Create backup of file"""
        try:
            path = Path(filepath)
            backup_path = path.parent / f"{path.stem}.bak{path.suffix}"
            backup_path.write_text(path.read_text(), encoding='utf-8')
            return str(backup_path)
        except Exception as e:
            raise IOError(f"Cannot create backup: {e}")
