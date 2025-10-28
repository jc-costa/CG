# Path Tracing Project – Computer Graphics (2025.2)

## Overview

This project implements the **global illumination algorithm Path Tracing**, as specified in the course project guidelines for *Computer Graphics – 2025.2*.

The implementation was developed from scratch, without relying on external libraries for the Path Tracing operations. Only the functions **putpixel** (for setting a pixel color) and **rand** (for generating uniformly distributed random numbers) or similar were used.

The main test scene is the **Cornell Box**, provided in OBJ format, along with additional **spheres**.

---

## Implemented Variant

In addition to the base implementation, our team implemented **Variant 1**, which adds **quadric surfaces**.

Users can input the coefficients of the quadric equation, including unbounded cases. To handle those, each quadric is associated with a **bounding box**, and **gradients are used as normals**.

---

## File Information

The detailed project specification is available in this repository as a PDF file named:
📄 **`CG-2-2025-Projeto.pdf`**

---

## Requirements

* Implement global illumination using the **Path Tracing** algorithm.
* Do not use pre-existing library functions to perform the Path Tracing itself.
* Use only `putpixel` and `rand` (or equivalent) functions from libraries.
* Include **Cornell Box** and **spheres** in the test scene.

---

## Authors

* Jefferson Costa
* Karla Falcão
Would you like me to make it sound a bit more **professional (for GitHub)** — e.g., adding sections like *How to Run*, *Dependencies*, and *Repository Structure* — or keep it in this concise academic format?
