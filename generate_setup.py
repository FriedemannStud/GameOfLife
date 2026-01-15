def get_ggg_pattern():
    # Standard Gosper Glider Gun coordinates
    # Relative to top-left 0,0
    coords = [
        (4, 0), (5, 0), (4, 1), (5, 1),  # Left block
        (4, 10), (5, 10), (6, 10),
        (3, 11), (7, 11),
        (2, 12), (8, 12),
        (2, 13), (8, 13),
        (5, 14),
        (3, 15), (7, 15),
        (4, 16), (5, 16), (6, 16),
        (5, 17),
        (2, 20), (3, 20), (4, 20),
        (2, 21), (3, 21), (4, 21),
        (1, 22), (5, 22),
        (0, 24), (1, 24), (5, 24), (6, 24),
        (2, 34), (3, 34), (2, 35), (3, 35) # Right block
    ]
    return coords

def write_bio_file(filename, rows, cols, population_limit, 
                   blue_off_x=0, blue_off_y=0, 
                   red_off_x=0, red_off_y=0):
    ggg = get_ggg_pattern()
    cells = []
    
    # --- BLUE TEAM (Left Side) ---
    # Base Position: 5, 5
    # Adjusted by blue offsets
    b_start_r = 5 + blue_off_y
    b_start_c = 5 + blue_off_x
    
    for r, c in ggg:
        cells.append((b_start_r + r, b_start_c + c, 2))
        
    # Eater for Blue (moves with blue shift)
    # Base position was approx (85, 85) relative to original placement
    # We apply the same shift so it stays in the glider path
    eater_1_base = [(85, 85), (85, 86), (86, 85), (86, 86)]
    for r, c in eater_1_base:
        if 0 <= r + blue_off_y < rows and 0 <= c + blue_off_x < cols:
            cells.append((r + blue_off_y, c + blue_off_x, 1)) # Red blocks to eat blue gliders

    # --- RED TEAM (Right Side) ---
    # Base Position: 5, Right-Side
    pattern_width = 36
    # Base column calculation
    r_start_r = 5 + red_off_y
    r_start_c = (cols - 5 - pattern_width) + red_off_x
    
    for r, c in ggg:
        # Mirror logic: new_col = width - col
        flipped_c = pattern_width - 1 - c
        cells.append((r_start_r + r, r_start_c + flipped_c, 1))

    # Eater for Red (moves with red shift)
    # Base position was approx (85, 165)
    eater_2_base = [(85, 165), (85, 166), (86, 165), (86, 166)]
    for r, c in eater_2_base:
        if 0 <= r + red_off_y < rows and 0 <= c + red_off_x < cols:
            cells.append((r + red_off_y, c + red_off_x, 1)) # Red blocks to eat red gliders? 
            # Actually doesn't matter which team is the eater, as long as it's a block.
            # But let's keep them uniform.

    # Write file
    with open(filename, 'w') as f:
        f.write(f"{rows} {cols} {population_limit}\n")
        for r, c, team in cells:
            f.write(f"{r} {c} {team}\n")

if __name__ == "__main__":
    # --- EINSTELLUNGEN / JUSTIERUNG ---
    # Hier die Werte ändern, um die Kanonen zu verschieben.
    # Positive X = nach rechts, Positive Y = nach unten
    
    BLUE_IDX_X = 82
    BLUE_IDX_Y = 3
    
    RED_IDX_X = -80
    RED_IDX_Y = 0
    
    # Grid Size
    ROWS = 150
    COLS = 250
    
    write_bio_file("setup.bio", ROWS, COLS, 1000, 
                   BLUE_IDX_X, BLUE_IDX_Y, 
                   RED_IDX_X, RED_IDX_Y)