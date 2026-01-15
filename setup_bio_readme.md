# How to create a .bio file

The `.bio` file format is a simple text format that defines the game board configuration and the starting positions of living cells.

Here is a guide on how to manually create such a file:

## File Structure

The file consists of two parts:
1.  **The Header**: Defines the board dimensions and the population limit.
2.  **The Data Lines**: Each subsequent line places exactly one living cell on the board.

---

### 1. The Header (First Line)
The first line must contain exactly three numbers, separated by spaces:
`[Number of Rows] [Number of Columns] [Max Population Limit]`

*   **Number of Rows**: The height of the board (Y-axis).
*   **Number of Columns**: The width of the board (X-axis).
*   **Max Initial Population Limit**: A limit used in the game rules.

**Example:**
`50 50 100`
*(Means: A 50x50 grid with an initial population limit of 100 units.)*

---

### 2. The Data Lines (Cell Positions)
Every line below the header represents a single living cell.
The format is:
`[Row] [Column] [Team-ID]`

*   **Row**: The row number (starts at 0 for the top row). Must be smaller than "Number of Rows" specified in the header.
*   **Column**: The column number (starts at 0 for the left column). Must be smaller than "Number of Columns" specified in the header.
*   **Team-ID**: Determines which team the cell belongs to.
    *   `1` = **Team Red**
    *   `2` = **Team Blue**

*(Note: Empty cells do not need to be listed; they are automatically considered "dead".)*

---

## Complete Example

Here is a very small, complete example file (e.g., `mini_test.bio`) for a 10x10 grid:

```text
10 10 20
2 2 2
2 3 2
6 6 1
6 7 1
```

**Explanation of the example:**
*   `10 10 20`: The field is 10x10 in size.
*   `2 2 2`: A blue cell in the lower right corner.
*   `2 3 2`: Another blue cell next to it.
*   `6 6 1`: A red cell in the upper left area. 
*   `6 7 1`: Another red cell next to it. 

You can write this format using any simple text editor (like Notepad, TextEdit, VS Code) and save the file with the `.bio` extension.

Be carefull! Blue cells are placed in the left side of the grid. Red cells are located in the right side of the grid.