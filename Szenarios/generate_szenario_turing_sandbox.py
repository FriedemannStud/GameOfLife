import sys

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

def get_block_pattern():
    # 2x2 Block (Eater)
    return [(0,0), (0,1), (1,0), (1,1)]

def transform_and_place(pattern, r_offset, c_offset, rotation=0, mirror=False):
    """
    Rotiert und spiegelt ein Muster und verschiebt es an die Zielposition.
    rotation: 0, 90, 180, 270 (im Uhrzeigersinn)
    mirror: True = Horizontal spiegeln (c -> -c) vor der Rotation
    """
    transformed = []
    for r, c in pattern:
        curr_r, curr_c = r, c
        
        # 1. Mirror (Horizontal Flip: c -> -c)
        # Dies spiegelt um die Y-Achse des lokalen Ursprungs (0,0)
        if mirror:
            curr_c = -curr_c
            
        # 2. Rotate (around 0,0)
        # 90 deg clockwise: (r, c) -> (c, -r)
        # 180: (r, c) -> (-r, -c)
        # 270: (r, c) -> (-c, r)
        if rotation == 90:
            curr_r, curr_c = curr_c, -curr_r
        elif rotation == 180:
            curr_r, curr_c = -curr_r, -curr_c
        elif rotation == 270:
            curr_r, curr_c = -curr_c, curr_r
            
        # 3. Translate
        final_r = curr_r + r_offset
        final_c = curr_c + c_offset
        
        transformed.append((final_r, final_c))
    return transformed

def write_bio_file(filename, rows, cols, population_limit, structures):
    all_cells = []
    
    for s in structures:
        stype = s.get('type', 'GGG')
        team = s.get('team', 1)
        r_off = s.get('y', 0)
        c_off = s.get('x', 0)
        rot = s.get('rotation', 0)
        mir = s.get('mirror', False)
        
        if stype == 'GGG':
            pattern = get_ggg_pattern()
        elif stype == 'BLOCK':
            pattern = get_block_pattern()
        else:
            print(f"Warnung: Unbekannter Typ {stype}")
            continue
            
        placed_cells = transform_and_place(pattern, r_off, c_off, rot, mir)
        
        for r, c in placed_cells:
            # Nur Zellen innerhalb des Gitters hinzufügen
            if 0 <= r < rows and 0 <= c < cols:
                all_cells.append((r, c, team))
    
    # Write file
    with open(filename, 'w') as f:
        f.write(f"{rows} {cols} {population_limit}\n")
        for r, c, team in all_cells:
            f.write(f"{r} {c} {team}\n")

if __name__ == "__main__":
    # --- EINSTELLUNGEN / JUSTIERUNG ---
    
    # Grid Size
    ROWS = 500
    COLS = 500
    POPULATION_LIMIT = 1000
    
    # Definition der zu platzierenden Strukturen
    # Struktur: "GGG" (Kanone), "BLOCK" (Eater/Hindernis)
    # Team: 1 (Rot), 2 (Blau)
    # x, y: Position (Spalte, Zeile) des Ankers (meist oben-links vom Muster)
    # rotation: 0, 90, 180, 270 (im Uhrzeigersinn)
    # mirror: True/False (Spiegelung an der lokalen Y-Achse, c -> -c)
    
    # Hinweis zur Positionierung:
    # Da Mirror (c -> -c) die Koordinaten negiert, muss die X-Position entsprechend 
    # angepasst werden, wenn man spiegeln will.
    # Beispiel: Ein Muster von x=0 bis x=35 wird durch Mirror zu x=0 bis x=-35.
    # Um es an die gleiche Stelle zu legen, muss man x um die Breite (35) erhöhen.
    
    PLACEMENTS = [
        # --- BLAUES TEAM (Links) ---
        # GGG (Kanone)
        # Original: (5, 5) + Offsets (3, 82) -> y=8, x=87
        {'type': 'GGG', 'team': 2, 'x': 87, 'y': 8, 'rotation': 0, 'mirror': False},
        
        # Eater (Block)
        # Original: (85, 85) + Offsets (3, 82) -> y=88, x=167
        # {'type': 'BLOCK', 'team': 1, 'x': 167, 'y': 88, 'rotation': 0, 'mirror': False},

        # --- ROTES TEAM (Rechts) ---
        # GGG (Kanone) - Gespiegelt
        # Original: y=5, x=129. Mirror war an rechter Kante.
        # Mit Mirror=True wird x=0..35 zu x=0..-35.
        # Um den Bereich 129..164 abzudecken (wie original), muss der Anker bei 164 liegen.
        # 164 + (-35) = 129.
        {'type': 'GGG', 'team': 1, 'x': 84, 'y': 55, 'rotation': 180, 'mirror': True},
        {'type': 'GGG', 'team': 1, 'x': 180, 'y': 8, 'rotation': 0, 'mirror': False},
        
        # Eater (Block)
        # Original: y=85, x=85.
        # {'type': 'BLOCK', 'team': 1, 'x': 85, 'y': 85, 'rotation': 0, 'mirror': False}
        
        
        
        # Team 1 nach oben: mal 0, Team 2 nach unten: mal 0
        {'type': 'GGG', 'team': 2, 'x': (87)+0, 'y': (8)+120, 'rotation': 0, 'mirror': False},
        {'type': 'GGG', 'team': 1, 'x': (87)+0, 'y': (30)+120, 'rotation': 180, 'mirror': True},
        # Team 1 nach oben: mal 1/5, Team 2 nach unten: mal 1/10
        {'type': 'GGG', 'team': 2, 'x': (87)+0, 'y': (8)+180, 'rotation': 0, 'mirror': False},
        {'type': 'GGG', 'team': 1, 'x': (84)+0, 'y': (55)+180, 'rotation': 180, 'mirror': True},
        # Team 1 nach oben: mal 0, Team 2 nach unten: mal 2/3
        {'type': 'GGG', 'team': 2, 'x': (87)+0, 'y': (8)+300, 'rotation': 0, 'mirror': False},
        {'type': 'GGG', 'team': 1, 'x': (89)+0, 'y': (41)+300, 'rotation': 180, 'mirror': True},
    ]
    
    write_bio_file("setup.bio", ROWS, COLS, POPULATION_LIMIT, PLACEMENTS)