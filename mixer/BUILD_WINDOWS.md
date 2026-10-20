# Compilação no Windows

## Base e ferramentas

- Blender `v5.2.2`, commit `d13f752e3b9c4f8c261cda552b1021f8bcc0382c`.
- Visual Studio 2022, ferramentas de C++ para desktop x64 e Windows SDK.
- CMake 3.21 ou superior; o script encontra a cópia do Visual Studio ou a do PATH.
- Git e Git LFS.
- Bibliotecas pré-compiladas oficiais, no commit de `lib/windows_x64` registrado pelo Blender.
  Elas incluem o Python embarcado: não é necessário substituir o Python do computador.

Pasta de trabalho de Gabriel:
`D:\gabri\Documents\GitHub repositories\Mixer-Modified-Blender-`.
Código, bibliotecas e saídas de compilação ficam dentro dela. O diretório `build/`
é ignorado pelo Git. Não enviar bibliotecas ou executáveis compilados como código-fonte.

## Executar

Abra o PowerShell na raiz do repositório e execute cada etapa separadamente:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\mixer\windows.ps1 -Action Dependencies
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\mixer\windows.ps1 -Action Configure
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\mixer\windows.ps1 -Action Build -Jobs 2
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\mixer\windows.ps1 -Action Smoke
```

O script usa `subst M:` temporariamente para contornar os espaços no caminho,
sem copiar o projeto. A unidade é liberada ao terminar. Se M: estiver ocupada,
use, por exemplo, `-Drive N` em **todas** as etapas. O cache do CMake guarda caminhos
absolutos: mantenha a mesma letra entre configurações e compilações.
Execute uma etapa por vez; não remova a unidade enquanto uma compilação estiver ativa.
Se o processo for encerrado à força, confira `subst` antes de liberar o alias manualmente.

O executável instalado é `build\windows\bin\Release\blender.exe`.
O teste Smoke abre esse executável em segundo plano com configurações de fábrica e
verifica a importação de `bpy` e a versão. A interface, os drivers da GPU e as funções
de edição precisam de verificações próprias; esse teste não cobre esses comportamentos.

O alvo `INSTALL` instala os arquivos na pasta de build do projeto. Não instala o
Blender em Program Files e não altera associações de arquivos do Windows.
A configuração inicial usa as opções padrão do Blender, sem exigir SDKs opcionais
para recompilar kernels CUDA/OptiX. Isso não equivale ao pacote oficial de distribuição.

## Obter uma nova cópia

Os objetos LFS originais são hospedados em `projects.blender.org`, e não no
espelho GitHub. Faça o clone sem tentar obtê-los do GitHub:

```powershell
$env:GIT_LFS_SKIP_SMUDGE = '1'
git clone https://github.com/Borbofruto/Mixer-Modified-Blender-.git
Remove-Item Env:\GIT_LFS_SKIP_SMUDGE
Set-Location .\Mixer-Modified-Blender-
git remote add upstream https://github.com/blender/blender.git
git remote set-url --push upstream DISABLED
```

Depois execute Dependencies, Configure, Build e Smoke. Dependencies baixa os
assets da árvore atual do servidor LFS oficial e as bibliotecas Windows do submódulo,
sem mover a branch Mixer nem atualizar o código para outra versão do Blender.
Os dados LFS de `tests/**` são excluídos desse download inicial. Antes de executar
suítes que dependem deles, baixe-os explicitamente do mesmo endpoint oficial.

Os assets herdados continuam referenciados por seus hashes LFS originais. Este
repositório não é uma cópia independente de todo o armazenamento LFS do Blender.
Novos assets binários do Mixer precisarão ser publicados no LFS do próprio projeto;
o script de dependências deverá então distinguir os assets novos dos herdados.
A preparação inicial só modifica arquivos de texto.

## Manutenção

`origin` é o repositório Mixer; `upstream` é o Blender oficial e fica bloqueado para push.
A tag base deve permanecer preservada. Atualizações do Blender devem ser revisadas em
uma branch própria, seguidas de configuração, compilação e testes; não execute
`make update` automaticamente sobre o fork, pois ele também pode atualizar código.

Referências oficiais:
[Windows](https://developer.blender.org/docs/handbook/building_blender/windows/),
[opções de compilação](https://developer.blender.org/docs/handbook/building_blender/options/),
[código da base](https://github.com/blender/blender/tree/v5.2.2).
