// Exemplo de como adicionar suas próprias quádricas
// Edite o arquivo: code/App/Shaders/PathTrace/PathTrace.glsl
// Função: initScene()

// ============================================================================
// EXEMPLOS DE QUÁDRICAS ADICIONAIS
// ============================================================================

// --- TORUS (aproximação) ---
// Um torus perfeito requer uma quártica (grau 4), mas podemos aproximar
// Não é uma quádrica exata, mas este exemplo mostra a estrutura

// --- CONE DUPLO ---
// Equação: x² + y² - z² = 0
quadrics[0] = createQuadric(
    1.0, 1.0, -1.0,          // A=1, B=1, C=-1
    0.0, 0.0, 0.0,           // Sem termos cruzados
    0.0, 0.0, 0.0,           // Sem termos lineares
    0.0,                     // J=0
    vec3(-2.0, -2.0, -3.0),  // Bounding box
    vec3(2.0, 2.0, 3.0),
    3                        // Material: chrome
);

// --- HIPERBOLOIDE DE DUAS FOLHAS ---
// Equação: z²/c² - x²/a² - y²/b² = 1
// Com a=1, b=1, c=1: z² - x² - y² = 1
// Rearranjado: -x² - y² + z² - 1 = 0
quadrics[1] = createQuadric(
    -1.0, -1.0, 1.0,         // A=-1, B=-1, C=1
    0.0, 0.0, 0.0,
    0.0, 0.0, 0.0,
    -1.0,                    // J=-1
    vec3(-3.0, -3.0, -3.0),
    vec3(3.0, 3.0, 3.0),
    7                        // Material: blue glossy
);

// --- PARABOLOIDE HIPERBÓLICO (SELA) ---
// Equação: z = x²/a² - y²/b²
// Com a=1, b=1: z = x² - y²
// Rearranjado: x² - y² - z = 0
quadrics[2] = createQuadric(
    1.0, -1.0, 0.0,          // A=1, B=-1, C=0
    0.0, 0.0, 0.0,
    0.0, 0.0, -1.0,          // I=-1 (termo linear em z)
    0.0,
    vec3(-2.0, -2.0, -4.0),
    vec3(2.0, 2.0, 4.0),
    4                        // Material: gold
);

// --- CILINDRO ELÍPTICO ---
// Equação: x²/a² + y²/b² = 1
// Com a=0.5, b=1.0: 4x² + y² = 1
// Rearranjado: 4x² + y² - 1 = 0
quadrics[3] = createQuadric(
    4.0, 1.0, 0.0,           // A=4, B=1, C=0 (sem z²)
    0.0, 0.0, 0.0,
    0.0, 0.0, 0.0,
    -1.0,
    vec3(-0.5, -1.0, -2.5),
    vec3(0.5, 1.0, 2.5),
    8                        // Material: rough white
);

// --- ESFERA (usando quádrica) ---
// Equação: x² + y² + z² = r²
// Com raio r=1.5: x² + y² + z² = 2.25
// Rearranjado: x² + y² + z² - 2.25 = 0
quadrics[4] = createQuadric(
    1.0, 1.0, 1.0,           // A=1, B=1, C=1
    0.0, 0.0, 0.0,
    0.0, 0.0, 0.0,
    -2.25,                   // J=-r²
    vec3(-1.5, -1.5, -1.5),
    vec3(1.5, 1.5, 1.5),
    6                        // Material: glass
);

// ============================================================================
// COMO CRIAR SUA PRÓPRIA QUÁDRICA
// ============================================================================

/*
PASSO 1: Escreva a equação da superfície desejada
Exemplo: Elipsoide -> x²/4 + y²/9 + z²/16 = 1

PASSO 2: Multiplique para eliminar denominadores
-> 16·9·x²/4 + 16·4·y²/9 + 9·4·z²/16 = 16·9·4
-> 36x² + 16y² + 9z² = 576

PASSO 3: Mova tudo para o lado esquerdo
-> 36x² + 16y² + 9z² - 576 = 0

PASSO 4: Identifique os coeficientes
A = 36  (coeficiente de x²)
B = 16  (coeficiente de y²)
C = 9   (coeficiente de z²)
D = 0   (coeficiente de xy)
E = 0   (coeficiente de xz)
F = 0   (coeficiente de yz)
G = 0   (coeficiente de x)
H = 0   (coeficiente de y)
I = 0   (coeficiente de z)
J = -576 (termo constante)

PASSO 5: Determine a bounding box
Para o elipsoide x²/4 + y²/9 + z²/16 = 1:
- Raio em x: √4 = 2  -> bbox x: [-2, 2]
- Raio em y: √9 = 3  -> bbox y: [-3, 3]
- Raio em z: √4 = 4  -> bbox z: [-4, 4]

PASSO 6: Crie a quádrica
quadrics[índice] = createQuadric(
    36.0, 16.0, 9.0,         // A, B, C
    0.0, 0.0, 0.0,           // D, E, F
    0.0, 0.0, 0.0,           // G, H, I
    -576.0,                  // J
    vec3(-2.0, -3.0, -4.0),  // bboxMin
    vec3(2.0, 3.0, 4.0),     // bboxMax
    materialIndex            // 0-9
);
*/

// ============================================================================
// MATERIAIS DISPONÍVEIS (índices 0-9)
// ============================================================================
/*
0 - White diffuse (branco difuso)
1 - Red diffuse (vermelho difuso)
2 - Green diffuse (verde difuso)
3 - Chrome metal (cromo metálico)
4 - Gold metal (ouro metálico)
5 - Warm light (luz quente emissiva)
6 - Clear glass (vidro transparente)
7 - Blue glossy (azul brilhante)
8 - Rough white (branco áspero)
9 - Bronze (bronze metálico)
*/

// ============================================================================
// DICAS IMPORTANTES
// ============================================================================
/*
1. BOUNDING BOX É ESSENCIAL
   - Sem bbox, superfícies ilimitadas (cilindro, paraboloide) se estendem infinito
   - Teste visual: se não renderizar, aumente a bbox

2. NORMALIZAÇÃO OPCIONAL
   - Pode dividir todos coeficientes pelo mesmo número sem alterar a forma
   - Exemplo: 2x² + 2y² + 2z² - 2 = 0  é o mesmo que  x² + y² + z² - 1 = 0

3. TESTE INCREMENTALMENTE
   - Comece com formas simples (esfera, cilindro)
   - Depois avance para formas complexas

4. POSICIONAMENTO
   - As quádricas aqui estão centradas na origem
   - Para mover, adicione termos lineares G, H, I ou transforme a cena

5. DEBUGGING
   - Se a superfície não aparecer, verifique:
     * Bbox está correta?
     * Câmera está posicionada para ver a superfície?
     * Coeficientes estão corretos?
*/
