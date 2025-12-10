# User Guide - Interactive Quadric System

## How to Edit Quadrics

The system allows users to interactively input quadric coefficients during program execution.

### Editing Methods

#### 1. Quick Editor (Alt + Number)

Press **Alt + [1-8]** to quickly edit a specific quadric:

- **Alt+1**: Edit Quadric 0 (Default ellipsoid)
- **Alt+2**: Edit Quadric 1 (Default hyperboloid)
- **Alt+3**: Edit Quadric 2 (Default paraboloid)
- **Alt+4**: Edit Quadric 3 (Default cylinder)
- **Alt+5 to Alt+8**: Edit Quadrics 4-7 (initially empty)

The editor will open in the terminal where you can enter the coefficients.

#### 2. Editor Menu (Ctrl+Q)

Press **Ctrl+Q** to open the quadric editor menu with options:
- Choose which quadric to edit
- View instructions
- Add new quadric

#### 3. List Quadrics (Ctrl+L)

Press **Ctrl+L** to list all active quadrics with their coefficients.

### Editing Process

When you open the editor for a quadric, you will be prompted to enter:

```
Enter A (x² coefficient) [current_value]: 
Enter B (y² coefficient) [current_value]: 
Enter C (z² coefficient) [current_value]: 
Enter D (xy coefficient) [current_value]: 
Enter E (xz coefficient) [current_value]: 
Enter F (yz coefficient) [current_value]: 
Enter G (x coefficient) [current_value]: 
Enter H (y coefficient) [current_value]: 
Enter I (z coefficient) [current_value]: 
Enter J (constant) [current_value]: 

--- Bounding Box ---
Enter bbox min X [current_value]: 
Enter bbox min Y [current_value]: 
Enter bbox min Z [current_value]: 
Enter bbox max X [current_value]: 
Enter bbox max Y [current_value]: 
Enter bbox max Z [current_value]: 

Enter material index (0-9) [current_value]: 
```

**Tip**: Press **Enter** without typing anything to keep the current value.

## Practical Examples

### Example 1: Create a Sphere

Equation: `x² + y² + z² = r²`

For a sphere with radius 1.5:

1. Press **Alt+5** (to edit Quadric 4, empty)
2. Enter the values:
   ```
   A: 1
   B: 1
   C: 1
   D: 0
   E: 0
   F: 0
   G: 0
   H: 0
   I: 0
   J: -2.25    (J = -r²)
   
   bbox min: -1.5, -1.5, -1.5
   bbox max: 1.5, 1.5, 1.5
   material: 6 (glass)
   ```

### Example 2: Create a Cone

Equation: `x² + y² - z² = 0`

1. Press **Alt+6**
2. Enter the values:
   ```
   A: 1
   B: 1
   C: -1
   D: 0
   E: 0
   F: 0
   G: 0
   H: 0
   I: 0
   J: 0
   
   bbox min: -2, -2, -3
   bbox max: 2, 2, 3
   material: 3 (chrome)
   ```

### Example 3: Create a Hyperbolic Paraboloid (Saddle)

Equation: `z = x² - y²` → Rearrange: `x² - y² - z = 0`

1. Press **Alt+7**
2. Enter the values:
   ```
   A: 1
   B: -1
   C: 0
   D: 0
   E: 0
   F: 0
   G: 0
   H: 0
   I: -1
   J: 0
   
   bbox min: -2, -2, -4
   bbox max: 2, 2, 4
   material: 4 (gold)
   ```

## Available Materials

When choosing the material index, use:

| Index | Material | Description |
|-------|----------|-------------|
| 0 | White Diffuse | White diffuse (walls) |
| 1 | Red Diffuse | Red diffuse |
| 2 | Green Diffuse | Green diffuse |
| 3 | Chrome Metal | Chrome metal (mirror-like) |
| 4 | Gold Metal | Gold metal |
| 5 | Warm Light | Warm emissive light |
| 6 | Clear Glass | Transparent glass |
| 7 | Blue Glossy | Glossy blue |
| 8 | Rough White | Rough white |
| 9 | Bronze | Bronze metal |

## Important Tips

### 1. Bounding Box is Essential
- Always define an appropriate bounding box
- For bounded surfaces (spheres, ellipsoids), the bbox should encompass the entire shape
- For unbounded surfaces (cylinders, cones, paraboloids), the bbox defines visible limits

### 2. Verify the Equation
Before entering coefficients:
1. Write the desired equation
2. Rearrange to the format: `Ax² + By² + Cz² + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0`
3. Identify each coefficient

### 3. Test Incrementally
- Start with simple shapes (sphere, cylinder)
- Test visualization before creating complex shapes
- Use **Ctrl+L** to verify values after editing

### 4. Visualize Changes
After editing a quadric:
- The scene will automatically restart
- Use camera controls (Right mouse + WASD) to view the new shape
- Adjust camera with Q/E (up/down)

### 5. Debugging
If the quadric doesn't appear:
- ✓ Check if the bounding box is correct
- ✓ Confirm the camera is positioned to see the shape
- ✓ Use **Ctrl+L** to verify coefficients were saved correctly
- ✓ Try temporarily increasing the bounding box

## Common Equation Conversions

### Sphere
- Standard form: `x² + y² + z² = r²`
- Coefficients: A=1, B=1, C=1, J=-r²

### Ellipsoid
- Standard form: `x²/a² + y²/b² + z²/c² = 1`
- Multiply: `(1/a²)x² + (1/b²)y² + (1/c²)z² - 1 = 0`
- Coefficients: A=1/a², B=1/b², C=1/c², J=-1

### Cylinder (Z-axis)
- Standard form: `x² + y² = r²`
- Coefficients: A=1, B=1, C=0, J=-r²

### Cone (Z-axis)
- Standard form: `x² + y² = z²`
- Rearrange: `x² + y² - z² = 0`
- Coefficients: A=1, B=1, C=-1, J=0

### Paraboloid
- Standard form: `z = x² + y²`
- Rearrange: `x² + y² - z = 0`
- Coefficients: A=1, B=1, I=-1

### Hyperboloid (1 sheet)
- Standard form: `x²/a² + y²/b² - z²/c² = 1`
- Coefficients: A=1/a², B=1/b², C=-1/c², J=-1

### Hyperboloid (2 sheets)
- Standard form: `z²/c² - x²/a² - y²/b² = 1`
- Coefficients: A=-1/a², B=-1/b², C=1/c², J=-1

## Complete Controls

| Key | Action |
|-----|--------|
| **Right mouse + WASD** | Move camera |
| **Q / E** | Move up / down |
| **Shift** | Move faster |
| **R** | Reload shaders |
| **T** | Cycle tonemapper |
| **+ / -** | Adjust exposure |
| **↑ / ↓** | Adjust max bounces |
| **F** | Toggle depth of field |
| **Ctrl+Q** | Quadric editor menu |
| **Ctrl+L** | List all quadrics |
| **Alt+[1-8]** | Edit quadric N |
| **ESC** | Quit |

## Saving Your Quadrics

Currently, quadrics are configured at runtime and are not saved to file. To make your configurations permanent:

1. After creating the desired quadric, use **Ctrl+L** to view the coefficients
2. Copy the values
3. (Optional) Add them to the code in `Main.cpp` in the `InitializeDefaultQuadrics()` function

## Limitations

- Maximum of 8 simultaneous quadrics (configurable in `MAX_QUADRICS`)
- Quadrics are always centered or positioned via linear terms G, H, I
- For more complex transformations (rotation), modify coefficients D, E, F
