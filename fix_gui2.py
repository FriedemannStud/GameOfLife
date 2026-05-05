import re

with open('gui.c', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Truncate at the first EndDrawing(); ... }
end_marker = 'EndDrawing(); // Raylib Anzeigesteuerung: Ende der "Zeichenrunde". Fertig gezeichnetes Bild wird im Fenster angezeigt.\n}'
idx = content.find(end_marker)
if idx != -1:
    content = content[:idx + len(end_marker)] + '\n'

# 2. Extract globals
globals_pattern = r'// --- NEW SHADER PIPELINE GLOBALS ---\nstatic Shader biotopeShader;\nstatic unsigned char \*gpu_data_buffer = NULL;\n'
content = re.sub(globals_pattern, '', content)

# 3. Insert globals before DrawGridAndCells
insert_point = content.find('void DrawGridAndCells')
if insert_point != -1:
    content = content[:insert_point] + '// --- NEW SHADER PIPELINE GLOBALS ---\nstatic Shader biotopeShader;\nstatic unsigned char *gpu_data_buffer = NULL;\n\n' + content[insert_point:]

with open('gui.c', 'w', encoding='utf-8') as f:
    f.write(content)

