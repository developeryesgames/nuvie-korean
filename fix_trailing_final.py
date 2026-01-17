with open('KoreanTranslation.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

old = """    // Remove trailing whitespace, newlines, and * (continuation marker)
    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\n' || trimmed.back() == '\r' || trimmed.back() == '*'))
        trimmed.pop_back();"""

new = """    // Remove trailing whitespace, newlines, *, and stray quotes (both single and double)
    // The pattern "text."\n*\n"' leaves orphan quotes after the dialogue ends
    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\n' || trimmed.back() == '\r' || trimmed.back() == '*' || trimmed.back() == '\'' || trimmed.back() == '"'))
        trimmed.pop_back();"""

if old in content:
    content = content.replace(old, new)
    with open('KoreanTranslation.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print('Fixed!')
else:
    print('Pattern not found')
