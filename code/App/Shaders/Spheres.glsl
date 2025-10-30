#version 460 core

layout(rgba32f, binding = 0) uniform writeonly image2D outputImage;

layout(local_size_x = 16, local_size_y = 16) in;

// Estrutura para representar uma esfera
struct Sphere {
    vec3 center;
    float radius;
    vec3 color;
};

// Função para calcular interseção raio-esfera
float intersectSphere(vec3 rayOrigin, vec3 rayDir, Sphere sphere) {
    vec3 oc = rayOrigin - sphere.center;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(oc, rayDir);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4.0 * a * c;
    
    if (discriminant < 0.0) {
        return -1.0;
    }
    
    return (-b - sqrt(discriminant)) / (2.0 * a);
}

// Função para calcular a normal da esfera
vec3 getSphereNormal(vec3 point, Sphere sphere) {
    return normalize(point - sphere.center);
}

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);

    if (pixelCoord.x >= imageSize(outputImage).x || pixelCoord.y >= imageSize(outputImage).y)
        return;

    ivec2 texSize = imageSize(outputImage);
    vec2 uv = (vec2(pixelCoord) - vec2(texSize) * 0.5) / float(texSize.y);

    // Configuração da câmera
    vec3 rayOrigin = vec3(0.0, 0.0, 3.0);
    vec3 rayDir = normalize(vec3(uv, -1.0));

    // Definir várias esferas com cores diferentes
    const int numSpheres = 7;
    Sphere spheres[numSpheres];
    
    // Esfera central grande - vermelha
    spheres[0].center = vec3(0.0, 0.0, 0.0);
    spheres[0].radius = 0.5;
    spheres[0].color = vec3(1.0, 0.2, 0.2);
    
    // Esfera à esquerda - verde
    spheres[1].center = vec3(-1.2, 0.0, -0.5);
    spheres[1].radius = 0.4;
    spheres[1].color = vec3(0.2, 1.0, 0.2);
    
    // Esfera à direita - azul
    spheres[2].center = vec3(1.2, 0.0, -0.5);
    spheres[2].radius = 0.4;
    spheres[2].color = vec3(0.2, 0.2, 1.0);
    
    // Esfera acima - amarela
    spheres[3].center = vec3(0.0, 0.8, -0.3);
    spheres[3].radius = 0.3;
    spheres[3].color = vec3(1.0, 1.0, 0.2);
    
    // Esfera abaixo - magenta
    spheres[4].center = vec3(0.0, -0.8, -0.3);
    spheres[4].radius = 0.3;
    spheres[4].color = vec3(1.0, 0.2, 1.0);
    
    // Esfera pequena - ciano
    spheres[5].center = vec3(0.6, 0.6, 0.5);
    spheres[5].radius = 0.25;
    spheres[5].color = vec3(0.2, 1.0, 1.0);
    
    // Esfera pequena - laranja
    spheres[6].center = vec3(-0.6, -0.6, 0.5);
    spheres[6].radius = 0.25;
    spheres[6].color = vec3(1.0, 0.6, 0.2);

    // Cor de fundo (gradiente)
    vec3 backgroundColor = mix(vec3(0.1, 0.1, 0.2), vec3(0.5, 0.7, 1.0), uv.y * 0.5 + 0.5);
    
    // Direção da luz
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));
    
    // Ray tracing
    vec3 color = backgroundColor;
    float closestT = 1e10;
    int hitSphere = -1;
    
    // Encontrar a esfera mais próxima
    for (int i = 0; i < numSpheres; i++) {
        float t = intersectSphere(rayOrigin, rayDir, spheres[i]);
        if (t > 0.0 && t < closestT) {
            closestT = t;
            hitSphere = i;
        }
    }
    
    // Se acertou uma esfera, calcular iluminação
    if (hitSphere >= 0) {
        vec3 hitPoint = rayOrigin + rayDir * closestT;
        vec3 normal = getSphereNormal(hitPoint, spheres[hitSphere]);
        
        // Iluminação difusa
        float diffuse = max(dot(normal, lightDir), 0.0);
        
        // Iluminação ambiente
        float ambient = 0.3;
        
        // Iluminação especular
        vec3 viewDir = normalize(rayOrigin - hitPoint);
        vec3 reflectDir = reflect(-lightDir, normal);
        float specular = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * 0.5;
        
        // Cor final com iluminação
        color = spheres[hitSphere].color * (ambient + diffuse) + vec3(specular);
        
        // Adicionar um pouco de reflexão do ambiente
        color += backgroundColor * 0.1 * (1.0 - diffuse);
    }
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    imageStore(outputImage, pixelCoord, vec4(color, 1.0));
}
