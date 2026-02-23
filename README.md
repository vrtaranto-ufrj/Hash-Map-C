# Hash-Map-C
Hash map simples implementado em C, com open addressing e redimensionamento.

## O que é um hash map
Um hash map é uma estrutura de dados que associa chaves a valores. Em média, as operações de inserção, busca e remoção são $O(1)$, assumindo uma função de hash bem distribuída e baixa taxa de colisão.

## Função de hash (FNV-1a)
Esta implementação usa a função de hash FNV-1a. Ela é simples, rápida e gera boa distribuição para chaves de texto. Referência: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function

## Open addressing
Open addressing resolve colisões armazenando os elementos no próprio array e fazendo sondagem (probing) quando ocorre colisão. Eu migrei da abordagem com lista encadeada para open addressing porque queria implementar essa lógica e o redimensionamento dinâmico. Outro motivo é que, com lista encadeada, eu sempre precisava dar `malloc` para cada elemento da lista. Hoje ainda faço isso porque copio as chaves, mas no futuro planejo criar uma pool para as chaves. A localidade de cache também pesou nessa escolha.

### Comparação rápida
| Aspecto | Open addressing | Lista encadeada (chaining) |
| --- | --- | --- |
| Memória extra | Menor (um array) | Maior (nós e ponteiros) |
| Localidade de cache | Melhor | Pior |
| Deleção | Requer tombstones | Remoção direta do nó |
| Performance em alta carga | Piora mais rápido | Tende a degradar mais suavemente |

## Redimensionamento
No `add_hashmap`, a carga é verificada antes de inserir. Quando a ocupação passa de `RESIZE_THRESHOLD` (70%), a tabela dobra de tamanho e todos os itens são reinseridos.

## Testes
Para compilar e rodar os testes localmente:

```bash
make test
./test_hashmap
```

Se quiser compilar tudo (main e testes):

```bash
make
```

## TODO
As ideias e tarefas pendentes estão nos Issues do repositório.
