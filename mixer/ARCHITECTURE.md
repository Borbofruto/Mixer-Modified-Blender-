# Mapa inicial do código

Este é um mapa para localizar mudanças. Não significa que todos os módulos já foram
auditados. A implementação de cada solicitação começa pelo fluxo concreto afetado.

| Responsabilidade | Local principal |
| --- | --- |
| Menus, painéis e atalhos em Python | `scripts/startup/bl_ui`, `scripts/startup/bl_operators`, `scripts/presets/keyconfig` |
| Integração da API Python | `source/blender/python` |
| Definição e exposição de propriedades (RNA) | `source/blender/makesrna` |
| Estruturas persistidas no arquivo (DNA) | `source/blender/makesdna` |
| Operadores e ferramentas dos editores | `source/blender/editors` |
| Eventos, janelas e ciclo dos operadores | `source/blender/windowmanager` |
| Operações de domínio e dados da cena | `source/blender/blenkernel` |
| Dependências e avaliação da cena | `source/blender/depsgraph` |
| Malhas e algoritmos geométricos | `source/blender/bmesh`, `source/blender/geometry` |
| Modificadores e nós | `source/blender/modifiers`, `source/blender/nodes` |
| Desenho da viewport e abstração gráfica | `source/blender/draw`, `source/blender/gpu` |
| Renderização e Cycles | `source/blender/render`, `intern/cycles` |
| Leitura, escrita e migração de arquivos | `source/blender/blenloader` |
| Compilação e dependências externas | `CMakeLists.txt`, `build_files`, `extern`, `lib` |

Por exemplo, um menu em `scripts/startup/bl_ui/space_view3d.py` pode chamar um
operador registrado em `source/blender/editors/mesh/mesh_ops.cc`. Propriedades de
malha expostas à interface e ao Python são definidas em
`source/blender/makesrna/intern/rna_mesh.cc`. O depsgraph controla quais dados precisam
ser reavaliados após uma mudança; desenhar novamente a interface não substitui isso.

## Critérios para cada solicitação

1. Descrever o comportamento atual, o comportamento esperado e um caso reproduzível.
2. Verificar primeiro se a API pública, um operador existente ou um script interno resolve.
3. Alterar C/C++ quando houver uma limitação concreta da API ou mudança de comportamento interno.
4. Seguir o fluxo existente de contexto, operadores, dados, notificações e avaliação.
5. Verificar modos de edição, seleção/contexto inválido, cancelamento, undo/redo,
   gravação e reabertura de arquivos, conforme a mudança exigir.
6. Adicionar validação que reproduza o comportamento afetado e compilar o que foi alterado.

Alterações no DNA ou no formato `.blend` exigem avaliar migração e compatibilidade
antes de mudar estruturas. O estado editável original e o estado avaliado pelo
depsgraph têm responsabilidades diferentes; não devem ser usados como se fossem
uma única cópia mutável da cena.

Preferir mudanças pequenas e commits por funcionalidade. A organização modular
existente deve ser mantida quando atender ao problema; não criar uma camada genérica
de plugins ou substituir subsistemas antes de haver uma necessidade demonstrada.

## Licença e identidade

Preservar `COPYING`, `doc/license`, avisos SPDX e créditos originais. O README oficial
declara GPL versão 3 para o conjunto, com licenças compatíveis em arquivos individuais.
Ao distribuir executáveis modificados, disponibilizar o código-fonte correspondente
sob os termos aplicáveis da GPL. Mixer deve ser identificado como versão modificada,
sem sugerir que é uma distribuição oficial do Blender.

Referências: [README da base](https://github.com/blender/blender/blob/v5.2.2/README.md),
[licença](https://www.blender.org/about/license/),
[documentação de desenvolvimento](https://developer.blender.org/docs/).
