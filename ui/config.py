"""
IDE Configuration
Settings and preferences for CompilerIDE
"""

import json
from pathlib import Path


class Config:
    """Configuration management for the IDE"""
    
    DEFAULT_CONFIG = {
        'window': {
            'width': 1400,
            'height': 900,
            'maximized': False,
        },
        'editor': {
            'font': 'Courier New',
            'font_size': 11,
            'tab_width': 4,
            'use_spaces': True,
            'line_numbers': True,
            'syntax_highlight': True,
            'auto_indent': True,
            'show_whitespace': False,
        },
        'compiler': {
            'path': r'd:\compiler\bin\compiler.exe',
            'timeout': 30,
            'auto_save': True,
        },
        'theme': {
            'name': 'light',
            'background': '#FFFFFF',
            'foreground': '#000000',
            'keyword_color': '#0066CC',
            'string_color': '#006600',
            'comment_color': '#999999',
            'number_color': '#CC0000',
        },
        'recent_files': [],
        'auto_build': False,
    }
    
    def __init__(self):
        self.config_dir = Path.home() / '.compileride'
        self.config_file = self.config_dir / 'config.json'
        self.config = self.DEFAULT_CONFIG.copy()
        self.load()
        
    def load(self):
        """Load configuration from file"""
        if self.config_file.exists():
            try:
                with open(self.config_file, 'r') as f:
                    loaded = json.load(f)
                    # Merge with defaults
                    self.config.update(loaded)
            except Exception as e:
                print(f"Error loading config: {e}")
                self.config = self.DEFAULT_CONFIG.copy()
        
    def save(self):
        """Save configuration to file"""
        try:
            self.config_dir.mkdir(parents=True, exist_ok=True)
            with open(self.config_file, 'w') as f:
                json.dump(self.config, f, indent=2)
        except Exception as e:
            print(f"Error saving config: {e}")
    
    def get(self, key, default=None):
        """Get configuration value"""
        keys = key.split('.')
        value = self.config
        for k in keys:
            if isinstance(value, dict):
                value = value.get(k)
            else:
                return default
        return value if value is not None else default
    
    def set(self, key, value):
        """Set configuration value"""
        keys = key.split('.')
        config = self.config
        for k in keys[:-1]:
            if k not in config:
                config[k] = {}
            config = config[k]
        config[keys[-1]] = value
        self.save()
    
    def reset(self):
        """Reset to default configuration"""
        self.config = self.DEFAULT_CONFIG.copy()
        self.save()


# Global config instance
_config = None

def get_config():
    """Get global config instance"""
    global _config
    if _config is None:
        _config = Config()
    return _config
