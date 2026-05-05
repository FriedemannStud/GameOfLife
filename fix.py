with open("gui.c", "r") as f:
    content = f.read()

# Fix the tail: find the LAST 'case STATE_GAME_OVER:' and keep everything up to the first 'EndDrawing();' after it.
game_over_idx = content.rfind("case STATE_GAME_OVER:")
if game_over_idx != -1:
    end_drawing_idx = content.find("EndDrawing();", game_over_idx)
    if end_drawing_idx != -1:
        end_brace_idx = content.find("}", end_drawing_idx)
        content = content[:end_brace_idx + 1] + "\n"

# Fix the missing STATE_PUZZLE if they were placed in the wrong spot
if "case STATE_PUZZLE:" not in content[content.find("switch (state) {") : content.find("case STATE_CONFIG:", content.find("switch (state) {"))]:
    # We need to insert case STATE_PUZZLE
    pass

with open("gui.c", "w") as f:
    f.write(content)
