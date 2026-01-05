#!/usr/bin/env python3
"""
Validate translations.json file for:
1. Valid JSON syntax
2. Structure consistency across languages
3. Key completeness (all keys present in all languages)
4. Other issues
"""

import json
import sys
from pathlib import Path
from collections import defaultdict

def validate_translations(file_path):
    """Validate the translations.json file."""
    errors = []
    warnings = []
    
    # Read and parse JSON
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        errors.append(f"JSON Syntax Error: {e}")
        return errors, warnings
    except Exception as e:
        errors.append(f"File Error: {e}")
        return errors, warnings
    
    print("OK: JSON syntax is valid")
    
    # Get all languages
    languages = list(data.keys())
    print(f"OK: Found {len(languages)} language(s): {', '.join(languages)}")
    
    # Define expected top-level sections
    expected_sections = {'ui', 'status', 'messages', 'ventoy_warning', 'ventoy_not_detected', 'titles', 'log', 'errors'}
    
    # Build a map of all keys in each language
    language_keys = {}
    for lang in languages:
        if not isinstance(data[lang], dict):
            errors.append(f"Language '{lang}' is not an object")
            continue
        
        lang_sections = set(data[lang].keys())
        language_keys[lang] = lang_sections
        
        # Check for missing sections
        missing_sections = expected_sections - lang_sections
        if missing_sections:
            warnings.append(f"Language '{lang}' missing sections: {', '.join(sorted(missing_sections))}")
        
        # Check for extra sections
        extra_sections = lang_sections - expected_sections
        if extra_sections:
            warnings.append(f"Language '{lang}' has extra sections: {', '.join(sorted(extra_sections))}")
    
    # Use English as the reference structure
    if 'en' not in languages:
        errors.append("English (en) language not found - cannot validate structure")
        return errors, warnings
    
    # Get all keys from English (reference language)
    ref_lang = 'en'
    
    def get_keys(obj, prefix=""):
        """Recursively get all keys from nested dictionary."""
        keys = []
        if isinstance(obj, dict):
            for key, value in obj.items():
                full_key = f"{prefix}.{key}" if prefix else key
                keys.append(full_key)
                if isinstance(value, dict):
                    keys.extend(get_keys(value, full_key))
        return keys
    
    ref_structure_keys = get_keys(data[ref_lang])
    ref_structure_keys_set = set(ref_structure_keys)
    
    print(f"OK: Reference language '{ref_lang}' has {len(ref_structure_keys)} keys")
    
    # Compare all languages with reference
    for lang in languages:
        if lang == ref_lang:
            continue
        
        # Get missing sections for this language (already reported above)
        lang_sections = set(data[lang].keys())
        missing_sections_for_lang = expected_sections - lang_sections
        
        lang_keys = get_keys(data[lang])
        lang_keys_set = set(lang_keys)
        
        # Find missing keys
        missing_keys = ref_structure_keys_set - lang_keys_set
        
        # Filter out keys that belong to entirely missing sections
        # If a section is missing entirely, don't report its keys (they're already covered by the section warning)
        missing_keys_filtered = {
            key for key in missing_keys 
            if not any(key.startswith(missing_section + '.') for missing_section in missing_sections_for_lang)
        }
        
        if missing_keys_filtered:
            missing_list = sorted(list(missing_keys_filtered))
            missing_display = ', '.join(missing_list[:10])
            if len(missing_keys_filtered) > 10:
                missing_display += f" ... and {len(missing_keys_filtered) - 10} more"
            warnings.append(f"Language '{lang}' missing {len(missing_keys_filtered)} key(s): {missing_display}")
        
        # Find extra keys
        extra_keys = lang_keys_set - ref_structure_keys_set
        if extra_keys:
            extra_list = sorted(list(extra_keys))
            extra_display = ', '.join(extra_list[:10])
            if len(extra_keys) > 10:
                extra_display += f" ... and {len(extra_keys) - 10} more"
            warnings.append(f"Language '{lang}' has {len(extra_keys)} extra key(s): {extra_display}")
    
    # Check for escape sequence issues (like \ n instead of \n)
    def check_escape_sequences(obj, path=""):
        """Check for malformed escape sequences in strings."""
        issues = []
        if isinstance(obj, dict):
            for key, value in obj.items():
                current_path = f"{path}.{key}" if path else key
                if isinstance(value, dict):
                    issues.extend(check_escape_sequences(value, current_path))
                elif isinstance(value, str):
                    # Check for backslash-space sequences that should be newlines
                    if '\\ ' in value or '\\ n' in value:
                        issues.append(f"Possible malformed escape in {current_path}: contains '\\ ' or '\\ n'")
        return issues
    
    escape_issues = check_escape_sequences(data)
    if escape_issues:
        warnings.extend(escape_issues)
    
    return errors, warnings

def main():
    file_path = Path(__file__).parent / "translations.json"
    
    if not file_path.exists():
        print(f"ERROR: {file_path} not found", file=sys.stderr)
        sys.exit(1)
    
    print(f"Validating {file_path}...")
    print()
    
    errors, warnings = validate_translations(file_path)
    
    print()
    
    if warnings:
        print(f"WARNING: Found {len(warnings)} warning(s):")
        for warning in warnings:
            print(f"  - {warning}")
        print()
    
    if errors:
        print(f"ERROR: Found {len(errors)} error(s):")
        for error in errors:
            print(f"  - {error}")
        print()
        sys.exit(1)
    
    if not warnings and not errors:
        print("All validations passed!")
        sys.exit(0)
    else:
        print("Validation completed with warnings (non-critical)")
        sys.exit(0)

if __name__ == "__main__":
    main()
