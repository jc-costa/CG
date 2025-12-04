# Guia de Uso - Sistema Interativo de Quádricas

## Como Editar Quádricas

O sistema permite que o usuário entre com os coeficientes das quádricas de forma interativa durante a execução do programa.

### Métodos de Edição

#### 1. Editor Rápido (Alt + Número)

Pressione **Alt + [1-8]** para editar rapidamente uma quádrica específica:

- **Alt+1**: Edita Quadric 0 (Elipsoide padrão)
- **Alt+2**: Edita Quadric 1 (Hiperboloide padrão)
- **Alt+3**: Edita Quadric 2 (Paraboloide padrão)
- **Alt+4**: Edita Quadric 3 (Cilindro padrão)
- **Alt+5 a Alt+8**: Edita Quadrics 4-7 (vazias inicialmente)

O editor será aberto no terminal onde você pode inserir os coeficientes.

#### 2. Menu do Editor (Ctrl+Q)

Pressione **Ctrl+Q** para abrir o menu do editor de quádricas com opções:
- Escolher qual quádrica editar
- Ver instruções
- Adicionar nova quádrica

#### 3. Listar Quádricas (Ctrl+L)

Pressione **Ctrl+L** para listar todas as quádricas ativas com seus coeficientes.

### Processo de Edição

Quando você abre o editor para uma quádrica, será solicitado a entrar com:

```
Enter A (x² coefficient) [valor_atual]: 
Enter B (y² coefficient) [valor_atual]: 
Enter C (z² coefficient) [valor_atual]: 
Enter D (xy coefficient) [valor_atual]: 
Enter E (xz coefficient) [valor_atual]: 
Enter F (yz coefficient) [valor_atual]: 
Enter G (x coefficient) [valor_atual]: 
Enter H (y coefficient) [valor_atual]: 
Enter I (z coefficient) [valor_atual]: 
Enter J (constant) [valor_atual]: 

--- Bounding Box ---
Enter bbox min X [valor_atual]: 
Enter bbox min Y [valor_atual]: 
Enter bbox min Z [valor_atual]: 
Enter bbox max X [valor_atual]: 
Enter bbox max Y [valor_atual]: 
Enter bbox max Z [valor_atual]: 

Enter material index (0-9) [valor_atual]: 
```

**Dica**: Pressione **Enter** sem digitar nada para manter o valor atual.

## Exemplos Práticos

### Exemplo 1: Criar uma Esfera

Equação: `x² + y² + z² = r²`

Para uma esfera de raio 1.5:

1. Pressione **Alt+5** (para editar Quadric 4, vazia)
2. Entre com os valores:
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

### Exemplo 2: Criar um Cone

Equação: `x² + y² - z² = 0`

1. Pressione **Alt+6**
2. Entre com os valores:
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

### Exemplo 3: Criar um Paraboloide Hiperbólico (Sela)

Equação: `z = x² - y²` → Rearranjar: `x² - y² - z = 0`

1. Pressione **Alt+7**
2. Entre com os valores:
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

## Materiais Disponíveis

Ao escolher o material index, use:

| Índice | Material | Descrição |
|--------|----------|-----------|
| 0 | White Diffuse | Branco difuso (paredes) |
| 1 | Red Diffuse | Vermelho difuso |
| 2 | Green Diffuse | Verde difuso |
| 3 | Chrome Metal | Cromo metálico (espelhado) |
| 4 | Gold Metal | Ouro metálico |
| 5 | Warm Light | Luz emissiva quente |
| 6 | Clear Glass | Vidro transparente |
| 7 | Blue Glossy | Azul brilhante |
| 8 | Rough White | Branco áspero |
| 9 | Bronze | Bronze metálico |

## Dicas Importantes

### 1. Bounding Box é Essencial
- Sempre defina uma bounding box adequada
- Para superfícies limitadas (esferas, elipsoides), a bbox deve englobar toda a forma
- Para superfícies ilimitadas (cilindros, cones, paraboloides), a bbox define os limites visíveis

### 2. Verificar a Equação
Antes de entrar com os coeficientes:
1. Escreva a equação desejada
2. Reorganize para o formato: `Ax² + By² + Cz² + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0`
3. Identifique cada coeficiente

### 3. Testar Incrementalmente
- Comece com formas simples (esfera, cilindro)
- Teste a visualização antes de criar formas complexas
- Use **Ctrl+L** para verificar os valores após editar

### 4. Visualizar as Mudanças
Após editar uma quádrica:
- A cena será automaticamente reiniciada
- Use os controles de câmera (Botão direito + WASD) para ver a nova forma
- Ajuste a câmera com Q/E (subir/descer)

### 5. Debugging
Se a quádrica não aparecer:
- ✓ Verifique se a bounding box está correta
- ✓ Confirme que a câmera está posicionada para ver a forma
- ✓ Use **Ctrl+L** para verificar se os coeficientes foram salvos corretamente
- ✓ Tente aumentar a bounding box temporariamente

## Conversão de Equações Comuns

### Esfera
- Forma padrão: `x² + y² + z² = r²`
- Coeficientes: A=1, B=1, C=1, J=-r²

### Elipsoide
- Forma padrão: `x²/a² + y²/b² + z²/c² = 1`
- Multiplique: `(1/a²)x² + (1/b²)y² + (1/c²)z² - 1 = 0`
- Coeficientes: A=1/a², B=1/b², C=1/c², J=-1

### Cilindro (eixo Z)
- Forma padrão: `x² + y² = r²`
- Coeficientes: A=1, B=1, C=0, J=-r²

### Cone (eixo Z)
- Forma padrão: `x² + y² = z²`
- Rearranjar: `x² + y² - z² = 0`
- Coeficientes: A=1, B=1, C=-1, J=0

### Paraboloide
- Forma padrão: `z = x² + y²`
- Rearranjar: `x² + y² - z = 0`
- Coeficientes: A=1, B=1, I=-1

### Hiperboloide (1 folha)
- Forma padrão: `x²/a² + y²/b² - z²/c² = 1`
- Coeficientes: A=1/a², B=1/b², C=-1/c², J=-1

### Hiperboloide (2 folhas)
- Forma padrão: `z²/c² - x²/a² - y²/b² = 1`
- Coeficientes: A=-1/a², B=-1/b², C=1/c², J=-1

## Controles Completos

| Tecla | Ação |
|-------|------|
| **Botão direito + WASD** | Mover câmera |
| **Q / E** | Subir / descer |
| **Shift** | Mover mais rápido |
| **R** | Recarregar shaders |
| **T** | Alternar tonemapper |
| **+ / -** | Ajustar exposição |
| **↑ / ↓** | Ajustar max bounces |
| **F** | Toggle depth of field |
| **Ctrl+Q** | Menu editor de quádricas |
| **Ctrl+L** | Listar todas as quádricas |
| **Alt+[1-8]** | Editar quádrica N |
| **ESC** | Sair |

## Salvando suas Quádricas

Atualmente, as quádricas são configuradas em tempo de execução e não são salvas em arquivo. Para tornar suas configurações permanentes:

1. Após criar a quádrica desejada, use **Ctrl+L** para ver os coeficientes
2. Copie os valores
3. (Opcional) Adicione-os ao código em `Main.cpp` na função `InitializeDefaultQuadrics()`

## Limitações

- Máximo de 8 quádricas simultâneas (configurável em `MAX_QUADRICS`)
- As quádricas são sempre centradas ou posicionadas via termos lineares G, H, I
- Para transformações mais complexas (rotação), modifique os coeficientes D, E, F
