# Organização no editor de nós

Primeira implementação sobre Blender 5.2.2 LTS. As ferramentas ficam no editor
compartilhado por Shader Nodes, Geometry Nodes e outros tipos de árvores.

## Coordenadas por clique

Abra o painel lateral com `N`, na aba **Node**. Em **Connection Coordinates**,
clique no círculo de um socket ou em um Reroute para consultar X e Y. Não há
rótulos de coordenadas sobre o grafo nem lista de conexões.

Um socket pertence a um nó. Um Reroute é o nó intermediário usado como ponto
em uma ligação. A interface identifica esses casos separadamente. O Blender
permite Reroutes desconectados; esta alteração não os remove.

As coordenadas são absolutas dentro da árvore editada, em unidades de nó,
independentes de zoom, pan e escala da interface. São somente leitura. O clique
preserva o gesto padrão de arrastar uma ligação e não altera as flags de seleção
de sockets usadas pelos operadores de conexão.

A inspeção guarda identificadores de árvore, nó e socket no estado temporário do
editor. Cada consulta resolve esses identificadores novamente; apagar o socket,
apagar o nó ou mudar para outra árvore faz o painel deixar de mostrar a seleção
anterior. A inspeção não é salva como ponteiro no `.blend`.

## Grade e layout

Em **Layout**, ajuste **Grid Spacing** entre 4 e 200 unidades, incluindo valores
fracionários. A grade desenhada, o snap de movimento e o snap de redimensionamento
usam o mesmo passo. Colagem com snap e normalização ao criar/separar grupos
também recebem o espaçamento configurado. Arquivos antigos sem esse campo continuam usando 20 unidades.
As opções são salvas no `.blend`. Com Socket Grid desligado, o passo é local ao
editor. Com ele ligado, o modo e seu passo pertencem à árvore: duas vistas da mesma
árvore usam a mesma geometria de sockets. Guias continuam sendo uma opção de cada
editor. Ativar o modo exige uma árvore editável; dados vinculados somente para
leitura podem ser inspecionados, mas seu layout não é reescrito.

**Socket Grid** acrescenta espaço entre linhas de controles de nós expandidos
para posicionar os centros dos sockets em múltiplos do passo, em relação à origem
do nó. A largura desenhada também é arredondada para cima até um múltiplo do passo.
A largura solicitada pelo nó continua preservada como mínimo. Não se altera o
valor persistente de largura a cada desenho.

Use **Snap Selected to Grid** para encaixar explicitamente os nós selecionados,
com o operador de transformação e o undo existentes. Com a origem do nó na grade
e Socket Grid ligado, as colunas de sockets de nós expandidos também ficam nela.
Ligar a opção sozinho não move os nós de um grafo existente.

## Guias

**Smart Guides** procura bordas, centros e sockets de nós visíveis no começo do
arraste. Com **Snap** ligado, encaixa a seleção quando está a até 8 pixels de um
alinhamento. O alinhamento com um vizinho tem prioridade sobre a grade nesse eixo.
Sem Snap, a guia apenas indica alinhamentos já próximos, com tolerância visual
inferior a um pixel.

As referências são ordenadas e guardadas no início do movimento para evitar
comparações entre todos os sockets a cada evento. Nós selecionados e seus filhos
não viram alvos da própria seleção. Frames são excluídos como alvos, pois seus
limites podem mudar durante o movimento de um descendente.

Uma seleção múltipla recebe uma correção comum durante o arraste com guias,
preservando a distribuição relativa. A ação explícita Snap Selected to Grid
encaixa cada origem. Guias respeitam as restrições de eixo, não atuam sobre
entrada numérica ou transformações executadas por script, e são removidas ao
concluir ou cancelar o movimento. O botão geral de overlays controla seu desenho.

## Limites e validação

- O layout na grade atua em nós expandidos. Nós recolhidos mantêm o desenho compacto.
- Sockets de múltiplas entradas têm o centro na grade; o espaçamento entre as
  terminações de links dentro do mesmo socket continua sendo o do Blender.
- As coordenadas de sockets dependem do layout calculado pelo editor aberto;
  não constituem uma propriedade geométrica disponível sem desenhar a árvore.
- O teste de API cobre duas vistas compartilhando o layout de uma árvore e
  vistas independentes quando o modo está desligado. A validação visual de
  painéis recolhidos, nós altos e múltiplas entradas ainda precisa ser executada.

Os testes C++ em `mixer/tests` verificam posições negativas, valores inválidos,
preservação da largura mínima, linhas sem sobreposição, passos fracionários,
escalas de interface de 1 a 2 e escolha determinística de referências.
Podem ser compilados sem compilar o Blender completo:

```powershell
cmake -S mixer/tests -B build/layout-tests -G "Visual Studio 17 2022" -A x64
cmake --build build/layout-tests --config Release
ctest --test-dir build/layout-tests -C Release --output-on-failure
```

`node_layout_api_test.py` verifica as propriedades, limites e gravação/reabertura
no executável compilado. Ele ainda não foi executado enquanto o executável completo
não estiver disponível. Essa distinção também está no registro de preparação.
