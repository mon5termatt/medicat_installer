#!/usr/bin/env python3
"""
Generate a new language section for translations.json

Usage: python generate_translation.py <language_code>
Example: python generate_translation.py de

This script will:
1. Read the English (en) section as a template
2. For each key, show the original English text and prompt for translation
3. Generate a complete language section matching the English structure
4. Save it to a file and optionally merge into translations.json
"""

import json
import sys
from pathlib import Path


def copy_structure_with_translations(en_obj, translations_dict):
    """
    Copy the English structure, replacing values with translations.
    
    Args:
        en_obj: The English object (dict or value)
        translations_dict: Dictionary mapping key paths to translations
    
    Returns:
        Copy of structure with translated values
    """
    if isinstance(en_obj, dict):
        result = {}
        for key, value in en_obj.items():
            if isinstance(value, dict):
                result[key] = copy_structure_with_translations(value, translations_dict)
            else:
                # It's a leaf value - look for translation
                result[key] = value  # Default to English
        return result
    else:
        return en_obj


def collect_translations(en_data, lang_code):
    """
    Iterate through the English structure and collect translations from user.
    
    Returns:
        Dictionary mapping key paths to translated strings
    """
    def walk_structure(obj, path=""):
        """Walk through structure and collect all key paths with values."""
        keys = {}
        
        if isinstance(obj, dict):
            for key, value in obj.items():
                current_path = f"{path}.{key}" if path else key
                
                if isinstance(value, dict):
                    # Recursively get keys from nested dict
                    keys.update(walk_structure(value, current_path))
                else:
                    # Leaf value
                    keys[current_path] = value
        else:
            # Leaf value (shouldn't happen at top level)
            keys[path] = obj
        
        return keys
    
    # Build flat dictionary of all key paths
    all_keys = walk_structure(en_data)
    
    print(f"\n{'='*70}")
    print(f"Generating translations for language code: {lang_code.upper()}")
    print(f"{'='*70}\n")
    print("For each key, you will see the English text.")
    print("Enter the translation, or press Enter to skip (will use English).")
    print("Type 'quit' to stop and save what you've done so far.")
    print(f"\nTotal keys to translate: {len(all_keys)}\n")
    
    total = len(all_keys)
    current = 0
    section = None
    
    for key_path, en_value in sorted(all_keys.items()):
        # Check if we're in a new section
        key_parts = key_path.split('.')
        current_section = key_parts[0] if key_parts else None
        
        if current_section != section:
            section = current_section
            print(f"\n{'='*70}")
            print(f"Section: {section}")
            print(f"{'='*70}")
        
        current += 1
        print(f"\n[{current}/{total}] {key_path}")
        print(f"English: {en_value}")
        
        while True:
            try:
                translation = input(f"Translation ({lang_code}): ").strip()
            except (EOFError, KeyboardInterrupt):
                print("\n\nStopping translation collection...")
                return translations
            
            if translation.lower() == 'quit':
                print("\nStopping translation collection...")
                return translations
            
            if not translation:
                # Use English as fallback
                translation = en_value
                print(f"Using English: {translation}")
            
            translations[key_path] = translation
            break
    
    return translations
    
    return translations


def build_translated_structure(en_data, translations_dict):
    """
    Build the translated structure from flat translations dictionary.
    
    Args:
        en_data: The English data structure (template)
        translations_dict: Dictionary mapping key paths (e.g., "ui.form_title") to translations
    
    Returns:
        Translated structure matching English structure
    """
    def apply_translations(obj, path=""):
        """Recursively apply translations to structure."""
        if isinstance(obj, dict):
            result = {}
            for key, value in obj.items():
                current_path = f"{path}.{key}" if path else key
                
                if isinstance(value, dict):
                    result[key] = apply_translations(value, current_path)
                else:
                    # Leaf value - use translation if available, otherwise English
                    if current_path in translations_dict:
                        result[key] = translations_dict[current_path]
                    else:
                        result[key] = value  # Fallback to English
            return result
        else:
            # Leaf value
            if path in translations_dict:
                return translations_dict[path]
            return obj
    
    return apply_translations(en_data)


def main():
    if len(sys.argv) < 2:
        print("Usage: python generate_translation.py <language_code>")
        print("Example: python generate_translation.py de")
        print("\nThis will generate a German (de) translation section.")
        print("\nLanguage codes should be 2-letter ISO codes (e.g., de, fr, es, it, pt, ja, zh)")
        sys.exit(1)
    
    lang_code = sys.argv[1].lower()
    
    if len(lang_code) != 2:
        print("Error: Language code must be 2 letters (e.g., 'de', 'fr', 'es', 'it')")
        sys.exit(1)
    
    # Load translations.json
    translations_file = Path(__file__).parent / "translations.json"
    
    if not translations_file.exists():
        print(f"Error: {translations_file} not found")
        sys.exit(1)
    
    try:
        with open(translations_file, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except Exception as e:
        print(f"Error loading translations.json: {e}")
        sys.exit(1)
    
    if 'en' not in data:
        print("Error: English (en) section not found in translations.json")
        sys.exit(1)
    
    if lang_code in data:
        response = input(f"Warning: Language '{lang_code}' already exists. Overwrite? (y/N): ").strip().lower()
        if response != 'y':
            print("Cancelled.")
            sys.exit(0)
    
    # Get English data as template
    en_data = data['en']
    
    # Collect translations
    translations_dict = collect_translations(en_data, lang_code)
    
    if not translations_dict:
        print("\nNo translations collected. Exiting.")
        sys.exit(0)
    
    print(f"\n{'='*70}")
    print(f"Collected {len(translations_dict)} translations")
    print(f"{'='*70}")
    
    # Build the translated structure
    print("\nBuilding translation structure...")
    translated_data = build_translated_structure(en_data, translations_dict)
    
    # Save to separate file first
    output_file = Path(__file__).parent / f"translation_{lang_code}.json"
    output_json = json.dumps({lang_code: translated_data}, indent=2, ensure_ascii=False)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(output_json)
    
    print(f"\n{'='*70}")
    print(f"Translation saved to: {output_file}")
    print(f"{'='*70}")
    
    # Ask if user wants to merge it into translations.json
    merge = input("\nMerge into translations.json now? (y/N): ").strip().lower()
    if merge == 'y':
        data[lang_code] = translated_data
        
        # Sort languages (keep 'en' first, then alphabetical)
        sorted_data = {}
        if 'en' in data:
            sorted_data['en'] = data.pop('en')
        for key in sorted(data.keys()):
            sorted_data[key] = data[key]
        data = sorted_data
        
        with open(translations_file, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
        
        print(f"\n✓ Added {lang_code} section to translations.json")
        
        # Validate
        try:
            with open(translations_file, 'r', encoding='utf-8') as f:
                json.load(f)
            print("✓ JSON validation passed")
        except json.JSONDecodeError as e:
            print(f"✗ JSON validation failed: {e}")
            print("Please check the file manually.")
    else:
        print(f"\nTo add this translation later, copy the content from:")
        print(f"  {output_file}")
        print(f"\nAnd merge it into translations.json manually.")


if __name__ == "__main__":
    main()
