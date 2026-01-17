with open('KoreanTranslation.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Fix the broken quotes - replace ''' with '\''
content = content.replace("''' || trimmed.back()", "'\'' || trimmed.back()")

with open('KoreanTranslation.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Fixed!")
