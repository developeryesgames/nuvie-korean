with open('KoreanTranslation.cpp', 'rb') as f:
    content = f.read()

# Replace b"'''" with b"'\''
content = content.replace(b"== '''", b"== '\'")

with open('KoreanTranslation.cpp', 'wb') as f:
    f.write(content)
print("Fixed!")
