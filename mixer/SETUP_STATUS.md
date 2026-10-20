# Preparação — 20/10/2026

- Repositório confirmado: `Borbofruto/Mixer-Modified-Blender-` (hífen final).
- Pasta local: `D:\gabri\Documents\GitHub repositories\Mixer-Modified-Blender-`.
- Base: tag `v5.2.2`, commit `d13f752e3b9c4f8c261cda552b1021f8bcc0382c`.
- Histórico original preservado; `upstream` oficial bloqueado para push localmente.
- Base publicada na main em `bb0625abb6fec2648e5f0991b5ee5284753f41a5`.
- Git LFS 3.7.1, Visual Studio 2022 17.14.42, MSVC 19.44.35229,
  Windows SDK 10.0.26100.0 e CMake 3.31.6 disponíveis.
- Assets LFS necessários à compilação obtidos do servidor oficial do Blender.
- Submódulo Windows fixado em `60d6e96b917568278d400a4024c98da0fb777338`.
- Bibliotecas de Release baixadas; submódulo limpo. Pacotes de Debug e símbolos opcionais.
- CMake do Blender: configuração e geração concluídas.
- Geradores `makesdna` e `makesrna`: compilados; geração DNA/RNA concluída.
- Módulo `bf_rna`: compilado com as propriedades novas.
- Módulo `bf_editor_space_node`: compilado.
- Módulo `bf_editor_transform`: compilado.
- Testes matemáticos C++: compilados com /W4 /WX; CTest passou (1/1).
- Sintaxe do Python da interface e do teste de API: validada.
- Sintaxe do script PowerShell: validada; `git diff --check` sem erros.
- Python embarcado 3.13.13: executado.
- Executável completo em compilação; teste de API e interação visual ainda pendentes.

A primeira implementação é descrita em [NODE_LAYOUT_TOOLS.md](NODE_LAYOUT_TOOLS.md).
