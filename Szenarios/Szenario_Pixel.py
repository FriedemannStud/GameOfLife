
"""
Konvertiert ein Bild in eine Pixelgrafik mit 4x4-Bitmap pro Pixel.
Jeder Pixel wird durch eine 4x4-Matrix mit festem Muster repräsentiert.
Nur Koordinaten mit Wert 1 werden ausgegeben.
"""

from PIL import Image
import sys
import os

# 4x4 Bitmap-Muster (0 = schwarz/aus, 1 = weiß/an)
BITMAP_PATTERN = [
    [0, 0, 0, 0],
    [0, 1, 1, 0],
    [0, 1, 1, 0],
    [0, 0, 0, 0]
]

def scale_image_preserve_aspect(image, target_size=1000):
    """
    Skaliert das Bild verzerrungsfrei auf max. target_size x target_size.
    
    Args:
        image: PIL Image Objekt
        target_size: Maximale Breite/Höhe in Pixeln
    
    Returns:
        Skaliertes PIL Image Objekt
    """
    width, height = image.size
    
    # Berechne Skalierungsfaktor (kleinerer Wert bestimmt die Skalierung)
    scale_factor = min(target_size / width, target_size / height)
    
    # Neue Dimensionen
    new_width = int(width * scale_factor)
    new_height = int(height * scale_factor)
    
    print(f"Original: {width}x{height} -> Skaliert: {new_width}x{new_height}")
    
    # Hochqualitatives Resampling
    return image.resize((new_width, new_height), Image.Resampling.LANCZOS)

def convert_to_bw(image, threshold=128, invert=False):
    """
    Konvertiert Bild zu Schwarz/Weiß.
    
    Args:
        image: PIL Image Objekt
        threshold: Schwellwert für Schwarz/Weiß (0-255)
        invert: Wenn True, werden die Farben invertiert
    
    Returns:
        Schwarz/Weiß PIL Image (1-bit)
    """
    # Zu Graustufen konvertieren
    grayscale = image.convert('L')
    
    # Zu 1-bit Schwarz/Weiß konvertieren
    if invert:
        # Invertierte Logik: weiß wird schwarz, schwarz wird weiß
        bw_image = grayscale.point(lambda x: 0 if x > threshold else 255, mode='1')
    else:
        bw_image = grayscale.point(lambda x: 255 if x > threshold else 0, mode='1')
    
    return bw_image

def expand_to_bitmap(bw_image):
    """
    Expandiert jedes Pixel zu einer 4x4 Bitmap mit festem Muster.
    Gibt nur Koordinaten mit Wert 1 zurück.
    
    Args:
        bw_image: PIL Image (1-bit Schwarz/Weiß)
    
    Returns:
        Liste von (zeile, spalte, wert) Tupeln (nur mit wert=1)
    """
    width, height = bw_image.size
    pixels = bw_image.load()
    
    count = 0
    output_data = []
    output_data.append((1000, 1000, 1000000))
    
    # Durchlaufe jeden Pixel des Eingabebildes
    for y in range(height):
        for x in range(width):
            # Hole Pixelwert (0 = schwarz, 255 = weiß bei mode='1')
            pixel_value = pixels[x, y]
            
            # Nur wenn Pixel weiß ist, verwende das Bitmap-Muster
            if pixel_value:  # Weiß
                # Expandiere zu 4x4 Bitmap
                
                for bitmap_y in range(4):
                    for bitmap_x in range(4):
                        value = BITMAP_PATTERN[bitmap_y][bitmap_x]
                        
                        # Nur Koordinaten mit Wert 1 hinzufügen
                        if value == 1:
                            if count % 2 == 0:
                                value = 2
                            count = count + 1
                            # Berechne absolute Koordinaten
                            abs_y = y * 4 + bitmap_y
                            abs_x = x * 4 + bitmap_x
                                                        
                            output_data.append((abs_y, abs_x, value))
    
    return output_data

def write_output(output_data, output_file):
    """
    Schreibt die Bitmap-Daten in eine Textdatei.
    
    Args:
        output_data: Liste von (zeile, spalte, wert) Tupeln
        output_file: Pfad zur Ausgabedatei
    """
    with open(output_file, 'w', encoding='utf-8') as f:
        for row, col, value in output_data:
            f.write(f"{row} {col} {value}\n")
    
    print(f"Ausgabe geschrieben: {output_file}")
    print(f"Anzahl Zeilen: {len(output_data)}")

def process_image(input_file, output_file, target_size=1000, threshold=128, invert=False):
    """
    Hauptfunktion: Verarbeitet ein Bild und erstellt die Pixelgrafik.
    
    Args:
        input_file: Pfad zum Eingabebild
        output_file: Pfad zur Ausgabedatei
        target_size: Maximale Bildgröße (Standard: 1000)
        threshold: Schwellwert für Schwarz/Weiß (Standard: 128)
        invert: Farben invertieren (Standard: False)
    """
    try:
        # Bild einlesen
        print(f"Lese Bild: {input_file}")
        image = Image.open(input_file)
        
        # Skalieren (verzerrungsfrei)
        scaled_image = scale_image_preserve_aspect(image, target_size)
        
        # Zu Schwarz/Weiß konvertieren
        invert_text = " (invertiert)" if invert else ""
        print(f"Konvertiere zu Schwarz/Weiß (Threshold: {threshold}){invert_text}")
        bw_image = convert_to_bw(scaled_image, threshold, invert)
        
        # Zu 4x4 Bitmap expandieren
        print("Expandiere zu 4x4 Bitmap-Muster...")
        output_data = expand_to_bitmap(bw_image)
        
        # In Datei schreiben
        write_output(output_data, output_file)
        
        print("✓ Erfolgreich abgeschlossen!")
        
    except FileNotFoundError:
        print(f"✗ Fehler: Datei '{input_file}' nicht gefunden!")
        sys.exit(1)
    except Exception as e:
        print(f"✗ Fehler bei der Verarbeitung: {e}")
        sys.exit(1)

def main():
    """
    Hauptprogramm mit Kommandozeilenargumenten.
    """
    if len(sys.argv) < 3:
        print("Usage: python script.py <input_image> <output_file> [target_size] [threshold] [invert]")
        print()
        print("Beispiele:")
        print("  python script.py input.png output.txt")
        print("  python script.py input.jpg output.txt 500")
        print("  python script.py input.bmp output.txt 1000 150")
        print("  python script.py input.png output.txt 1000 128 yes")
        print()
        print("Parameter:")
        print("  input_image  : Eingabebild (PNG, JPG, BMP)")
        print("  output_file  : Ausgabedatei (TXT)")
        print("  target_size  : Max. Größe in Pixeln (Standard: 1000)")
        print("  threshold    : Schwellwert 0-255 (Standard: 128)")
        print("  invert       : Farben invertieren: yes/no (Standard: no)")
        sys.exit(1)
    
    
    input_file = sys.argv[1]
    
    for threshold in range(10, 250, 20):
        target_size = int(sys.argv[3]) if len(sys.argv) > 3 else 1000
        # threshold = int(sys.argv[4]) if len(sys.argv) > 4 else 128
        invert = sys.argv[5].lower() in ['yes', 'y', 'true', '1', 'ja'] if len(sys.argv) > 5 else False
        output_file = sys.argv[2]+"_"+str(threshold)+"_inv"+str(invert)+".bio"
        
        process_image(input_file, output_file, target_size, threshold, invert)

if __name__ == "__main__":
    main()
