# Implementação de Quádricas

## Visão Geral

Este projeto implementa superfícies quádricas no path tracer, permitindo renderizar formas como elipsoides, hiperboloides, paraboloides e cilindros através da equação geral da quádrica.

## Equação da Quádrica

A equação geral de uma superfície quádrica em 3D é:

```
Ax² + By² + Cz² + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0
```

Onde:
- **A, B, C**: Coeficientes dos termos quadráticos (x², y², z²)
- **D, E, F**: Coeficientes dos termos cruzados (xy, xz, yz)
- **G, H, I**: Coeficientes dos termos lineares (x, y, z)
- **J**: Termo constante

## Características Implementadas

### 1. Interseção Raio-Quádrica

O raio é definido por: **P(t) = O + t·D**

Onde:
- **O** = origem do raio (ray origin)
- **D** = direção do raio (ray direction)
- **t** = parâmetro do raio

Substituindo na equação da quádrica, obtemos uma equação quadrática:

```
at² + bt + c = 0
```

Resolvida usando a fórmula de Bhaskara.

### 2. Bounding Box

Para superfícies ilimitadas (cilindros, paraboloides, hiperboloides), associamos uma **bounding box** (caixa delimitadora AABB - Axis-Aligned Bounding Box) definida por:

- **bboxMin**: Ponto mínimo (x, y, z)
- **bboxMax**: Ponto máximo (x, y, z)

A interseção só é calculada se o raio interceptar a bounding box primeiro, otimizando o desempenho.

### 3. Cálculo de Normais via Gradiente

A normal em um ponto **P** da superfície é calculada usando o gradiente da função quádrica:

```
∇Q(P) = (∂Q/∂x, ∂Q/∂y, ∂Q/∂z)
```

Onde:
```
∂Q/∂x = 2Ax + Dy + Ez + G
∂Q/∂y = 2By + Dx + Fz + H
∂Q/∂z = 2Cz + Ex + Fy + I
```

O gradiente aponta na direção perpendicular à superfície, fornecendo a normal.

## Exemplos de Quádricas

### Elipsoide

Equação: `x²/a² + y²/b² + z²/c² = 1`

Coeficientes:
```glsl
A = 1/a², B = 1/b², C = 1/c²
D = E = F = G = H = I = 0
J = -1
```

**Exemplo**: Elipsoide com a=0.8, b=1.2, c=0.6
```glsl
A = 1.5625, B = 0.6944, C = 2.7778
J = -1.0
bboxMin = vec3(-0.8, -1.2, -0.6)
bboxMax = vec3(0.8, 1.2, 0.6)
```

### Esfera

Caso especial do elipsoide onde `a = b = c = r`:

```glsl
A = B = C = 1
J = -r²
```

### Hiperboloide de Uma Folha

Equação: `x²/a² + y²/b² - z²/c² = 1`

```glsl
A = 1/a², B = 1/b², C = -1/c²
J = -1
```

**Exemplo**: a=0.5, b=0.5, c=1.0
```glsl
A = 4.0, B = 4.0, C = -1.0
J = -1.0
bboxMin = vec3(-1.0, -1.0, -2.0)
bboxMax = vec3(1.0, 1.0, 2.0)
```

### Paraboloide

Equação: `z = x² + y²`

Rearranjada: `x² + y² - z = 0`

```glsl
A = 1, B = 1, C = 0
I = -1  // termo linear em z
J = 0
```

**Exemplo**:
```glsl
A = 1.0, B = 1.0
I = -1.0
bboxMin = vec3(-1.5, -1.5, 0.0)
bboxMax = vec3(1.5, 1.5, 4.5)
```

### Cilindro

Equação: `x² + y² = r²`

Rearranjada: `x² + y² - r² = 0`

```glsl
A = 1, B = 1, C = 0  // sem termo z²
J = -r²
```

**Exemplo**: Cilindro com raio r=0.6
```glsl
A = 1.0, B = 1.0
J = -0.36
bboxMin = vec3(-0.6, -0.6, -2.0)
bboxMax = vec3(0.6, 0.6, 2.0)
```

### Cone

Equação: `x² + y² - z² = 0`

```glsl
A = 1, B = 1, C = -1
J = 0
```

### Hiperboloide de Duas Folhas

Equação: `z²/c² - x²/a² - y²/b² = 1`

```glsl
A = -1/a², B = -1/b², C = 1/c²
J = -1
```

## Como Adicionar uma Nova Quádrica

No arquivo `PathTrace.glsl`, dentro da função `initScene()`:

```glsl
quadrics[index] = createQuadric(
    A, B, C,           // Coeficientes quadráticos
    D, E, F,           // Termos cruzados
    G, H, I,           // Termos lineares
    J,                 // Constante
    bboxMin,           // vec3 - mínimo da bbox
    bboxMax,           // vec3 - máximo da bbox
    materialIndex      // índice do material
);
```

## Limitações e Considerações

1. **Bounding Box**: Essencial para superfícies ilimitadas. Sem ela, a quádrica se estenderia infinitamente.

2. **Precisão Numérica**: Valores muito pequenos de gradiente podem causar problemas. O código verifica `|∇Q| < ε` antes de usar.

3. **Performance**: A interseção raio-quádrica requer resolver uma equação quadrática, que é mais custosa que raio-esfera mas mais barata que malhas triangulares.

4. **Número Máximo**: Atualmente definido em `#define NUM_QUADRICS 4`. Ajuste conforme necessário.

## Referências

- **Physically Based Rendering** - Matt Pharr, Wenzel Jakob, Greg Humphreys
- **Ray Tracing Gems** - Eric Haines, Tomas Akenine-Möller
- **Computer Graphics: Principles and Practice** - Foley, van Dam, Feiner, Hughes
