# Preparação — 20/10/2026

- Repositório confirmado: `Borbofruto/Mixer-Modified-Blender-` (hífen final).
- Pasta local: `D:\gabri\Documents\GitHub repositories\Mixer-Modified-Blender-`.
- Base: tag `v5.2.2`, commit `d13f752e3b9c4f8c261cda552b1021f8bcc0382c`.
- Histórico original preservado; `upstream` oficial bloqueado para push localmente.
- Base e primeira implementação publicadas em `main`; histórico de commits preservado.
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
- Compilação completa Release e instalação local: concluídas.
- Inicialização `Smoke`: passou (`MIXER_SMOKE_OK 5.2.2 LTS`).
- Integração no executável com janela inicializada: passou (`MIXER_NODE_LAYOUT_API_OK`).
  Cobre snap com e sem ímã, agrupamento, configuração entre vistas, proteção de
  árvores vinculadas e persistência após salvar/reabrir.
- O teste de transformações exige janela: `--background` deixa a escala da interface
  em zero. Isso foi observado e o teste agora rejeita esse contexto explicitamente.
- Cliques em sockets e aparência das guias durante o arraste: validação visual pendente.
- Executável: `build/windows/bin/Release/blender.exe`.
- Logs locais em `build/setup`; arquivos de teste `.blend` em `build/layout-tests`.

A primeira implementação é descrita em [NODE_LAYOUT_TOOLS.md](NODE_LAYOUT_TOOLS.md).
