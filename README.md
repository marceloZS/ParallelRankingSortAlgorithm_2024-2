# ParallelRankingSortAlgorithm_2024-2

El ordenamiento eficiente por ranking es un tópico crucial en el área de ciencias de la computación.

El objetivo de este proyecto es llevar a cabo la tarea de implementar en paralelo el algoritmo de ordenamiento por ranking.

## Integrantes
- Marcelo Zuloeta : 202110397
- Juan Leandro :
- Fernando Choque :


## Contenido

El proyecto contiene lo siguiente
### PRAM TEÓRICO

Input: d[1...n]
Output: ranking de d(i,j,k) almacenado en b' en el proceso (i,j)

1. Gossip:
   forall (i, j) pardo
       a[i, j] = gather a[1, j], ..., a[P, j] from all processes in column j

2. Broadcast:
   forall (i, j) pardo
       a'[i, j] = broadcast a[i, j] to all processes in row i

3. Sort:
   forall (i, j) pardo
       sort a'[i, j]

4. Local Ranking:
   forall (i, j) pardo
       b[i, j] = rank elements in a'[i, j]

5. Reduce:
   forall (i, j) pardo
       b'[i, j] = reduce sum of b[i, j] across all columns to process (i, 0)


### El código paralelizable utilizando una implementación de alto performance con el estándar Message Passaging Interface - MPI

### Registro del desarrollo del código en por lo menos 3 pasos (versiones beta).


### Medición del tiempo de ejecución, y la velocidad del algoritmo, representado en gráficas.
Los resultados serán comparados con la complejidad teórica en paralelo, para distintos números de
procesos y tamaños del problema. 

Se utilizará un tamaño adecuado del array para lograr tiempos de ejecución medibles
Se comparará la escalabilidad del software con Quicksort en paralelo (MPI/OMP)
