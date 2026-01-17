with open('KoreanTranslation.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Find and fix ALL trailing quote removal lines
count = 0
for i, line in enumerate(lines):
    if "trimmed.back() == '*'))" in line and "trimmed.back() == '\"'" not in line:
        print(f"Found at line {i+1}: {line.strip()}")
        # This is the line to fix - add quote removal
        lines[i] = line.replace(
            "trimmed.back() == '*'))",
            "trimmed.back() == '*' || trimmed.back() == '\'' || trimmed.back() == '\"'))"
        )
        # Also fix the comment on previous line
        if "continuation marker" in lines[i-1]:
            lines[i-1] = lines[i-1].replace(
                "// Remove trailing whitespace, newlines, and * (continuation marker)",
                "// Remove trailing whitespace, newlines, *, and stray quotes"
            )
        print(f"Fixed to: {lines[i].strip()}")
        count += 1

with open('KoreanTranslation.cpp', 'w', encoding='utf-8') as f:
    f.writelines(lines)
print(f"Done! Fixed {count} occurrences")
