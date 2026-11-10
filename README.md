# Folder Size — correção para Windows 11 (popups no Explorer)

Reimplementação do cliente **`FolderSize.exe`** do [Folder Size for Windows 2.6](https://sourceforge.net/projects/foldersize/) (GPLv2, Brian Oraas / `brio1337`). O serviço `FolderSizeSvc`, o painel `FolderSize.cpl` e a coluna para XP **não** são alterados — só o executável da bandeja/popups.

## Sobre o projeto original

| Item | Detalhe |
|------|---------|
| Licença | [GPLv2](LICENSE) |
| Última release oficial | 2.6 (2013) |
| Fonte publicada | Apenas [FolderSize-2.0-src.zip](https://sourceforge.net/projects/foldersize/files/foldersize/2.0/) — **não** há fonte da 2.6 no SourceForge |
| Modo Vista+ | `FolderSize.exe` abre popups `FolderSizeShellWindow` ao lado do Explorer |

No Windows 11 o modo coluna do Explorer (XP) não existe; o fluxo é **serviço + cliente popup**.

## O bug

**Sintoma:** ao abrir uma **segunda** (ou terceira) janela do Explorer (`Win+E`), o popup de tamanhos **não aparece** ou não acompanha — só a primeira janela costuma funcionar.

**Causa (cliente 2.6):** o rastreio depende principalmente de `DShellWindowsEvents` (`WindowRegistered` / `WindowRevoked`) e de associação frágil entre janelas novas e popups. No Win11 isso fica incompleto.

**Causa extra encontrada neste patch:** ao enumerar janelas via `IShellWindows`, `IShellBrowser::GetWindow` devolve um HWND **interno** do controlador web, **não** o topo `CabinetWClass`. Todos os explorers eram descartados no filtro. A correção usa `IWebBrowser2::get_HWND` + `GetAncestor(..., GA_ROOT)` para obter o frame correto.

Relacionado (upstream, ainda aberto): [bug #35](https://sourceforge.net/p/foldersize/bugs/35/) — popup não volta após fechar.

## O que este repositório faz

```mermaid
flowchart LR
  Explorer[CabinetWClass]
  Client[FolderSize.exe patch]
  Pipe["Named pipe FolderSize"]
  Svc[FolderSizeSvc]
  Explorer --> Client
  Client -->|IShellWindows + polling 750ms| Explorer
  Client -->|Shell hook HSHELL_*| Explorer
  Client --> Pipe
  Pipe --> Svc
```

- **Polling** (~750 ms) de instâncias `IShellWindows` / `_NewEnum`
- **`RegisterShellHookWindow`** para `HSHELL_WINDOWCREATED`, `DESTROYED`, `ACTIVATED`
- Um popup **`FolderSizeShellWindow` por HWND** do Explorer
- Listagem da pasta atual via `\\.\pipe\FolderSize` (protocolo compatível com o serviço 2.6)
- Pastas virtuais (ex.: **Início**) mostram aviso; pastas em disco listam tamanhos do cache do serviço

Código do patch: [`client/src/`](client/src/)  
Referência do upstream 2.0 (GPL): [`upstream-2.0/`](upstream-2.0/)  
**Pacote pronto para instalar (binários GPLv2):** [`redist/`](redist/)

## Instalação (sem baixar o SourceForge)

Tudo que você precisa está neste repositório — **não é obrigatório** baixar o MSI original.

### Opção A — ZIP da Release (recomendado)

1. Baixe [FolderSize-Win11-Patch-2.6.1.0.zip](https://github.com/alanspindler/foldersize-win11-fix/releases/latest) (ou o asset mais recente).
2. Extraia e abra PowerShell **como Administrador** na pasta.
3. Execute:

   ```powershell
   .\install.ps1
   ```

   Para iniciar com o Windows: `.\install.ps1 -StartWithWindows`

### Opção B — clone do GitHub

```powershell
git clone https://github.com/alanspindler/foldersize-win11-fix.git
cd foldersize-win11-fix
# PowerShell como Administrador:
.\packaging\install.ps1
```

O script copia os arquivos de [`redist/`](redist/) para `C:\Program Files\FolderSize`, registra o serviço **FolderSize**, inicia o cliente e configura o applet do Painel de Controle.

### Só atualizar o cliente (já tem o Folder Size instalado)

Substitua `C:\Program Files\FolderSize\FolderSize.exe` pelo [`redist/FolderSize.exe`](redist/FolderSize.exe) (patch) e reinicie o cliente pela bandeja — **sem** elevar o processo (admin não vê o Explorer do usuário).

### Desinstalar

```powershell
.\packaging\uninstall.ps1   # como Administrador
```

## Posso compilar e distribuir? (GPLv2)

**Sim**, desde que você respeite a [GPLv2](LICENSE):

- Mantenha a licença e o aviso de copyright ([NOTICE](NOTICE)).
- Disponibilize o **código-fonte** das partes que você modificar (aqui: `client/` no GitHub).
- Se redistribuir binários, qualquer pessoa que receba os binários pode pedir o código correspondente.

**O que está neste pacote**

| Arquivo | Origem |
|---------|--------|
| `FolderSize.exe` | **Recompilado** — patch Win11 (`client/src`) |
| `FolderSizeSvc.exe`, `FolderSize.cpl`, `FolderSizeColumn.dll` | Binários **2.6 x64** do projeto original (GPLv2, sem alteração), republicados neste repo para instalação completa |

O autor original não publicou o fonte da 2.6; incluímos o fonte **2.0** em `upstream-2.0/` como referência GPL e o fonte novo do cliente. Isso **não** substitui o projeto oficial nem implica endosso do autor Brian Oraas — é um **fork comunitário** para Windows 11.

## Compilar

Requisitos: **Visual Studio 2022 Build Tools** (C++), **CMake**, **Ninja**.

```bat
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cd foldersize-win11-fix
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Saída: `build/FolderSize.exe` (versão de recurso **2.6.1.0**). Atualize `redist/` com:

```powershell
.\packaging\build-redist.ps1 -Zip   # gera também o ZIP para Release
```

## Teste rápido (Windows 11)

1. Bandeja → *Show Folder Size popups* marcado.
2. `Win+E` → um popup ao lado.
3. `Win+E` de novo → **segundo** popup.
4. Feche uma janela do Explorer → popup some.
5. Navegue para `C:\` ou Downloads → tamanhos de subpastas (via serviço).

## Licença e atribuição

Este trabalho é um **derivado** do Folder Size for Windows (**GPLv2**). O código novo neste repositório é distribuído sob a mesma licença — veja [LICENSE](LICENSE). Melhorias devem permanecer livres conforme a GPL.

O projeto original não é mantido desde 2013; este repositório **não** é afiliado ao autor Brian Oraas — é um patch comunitário para Windows 11.

**Não confundir** com o produto comercial [MindGems “Folder Size” 5.x](https://www.mindgems.com/products/Folder-Size/) (outro software).
